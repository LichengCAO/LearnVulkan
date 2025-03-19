#version 450

layout(location = 0) in vec3 vViewNormal;
layout(location = 1) in vec4 vScreenPos;
layout(location = 2) in vec3 vNormalWorld;
layout(location = 3) in vec3 vPosWorld;

layout(set = 0, binding = 0) uniform CameraInformation
{
    mat4 view;
    mat4 proj;
    vec4 eye;
} cameraInfo;
layout(set = 2, binding = 0) uniform CameraViewInformation
{
	mat4 normalView; // inverse transpose of view matrix
	float InvTanHalfFov;
	float ScreenRatioXY;
} cameraView;

// layout(set = 3, binding = 0) uniform sampler2D texAlbedo;
// layout(set = 3, binding = 1) uniform sampler2D texPos;
// layout(set = 3, binding = 2) uniform sampler2D texNormal;
layout(set = 3, binding = 3) uniform sampler2D texDepth;

layout(set = 4, binding = 0) uniform SimpleMaterial
{
	float roughness;	
} material;

layout(location = 0) out vec4 outColor;

#define AIR_IOR 1.0f

float GetSceneDepth(in vec2 uv)
{
	uv = clamp(uv, vec2(0.0f, 0.0f), vec2(1.0f, 1.0f));
	return texture(texDepth, uv).r;
}

vec3 TransformDirectionFromWorldToView(in vec3 dir)
{
	return (cameraInfo.view * vec4(dir, 0.0f)).rgb;
}

vec2 ComputeUVDistortion(
	in float refractionMaterialDepth,
	in float SceneDepth,
	in float materialIOR,
	in vec3 N, // world
	in vec3 I // world
)
{
	vec3 wo = normalize(refract(I, N, 1.0f / materialIOR));
	float cosTheta = clamp(dot(N, I), 0.0f, 1.0f);
	vec3 vWo = TransformDirectionFromWorldToView(wo);
	const float OffsetFudgeFactor = 0.00023;
	return vWo.xy * 100.f / SceneDepth * vec2(OffsetFudgeFactor, -OffsetFudgeFactor);// * cosTheta;
}

vec2 ComputeBufferUVDistortion(
	in vec3 Normal, // normal in view space
	in float MaterialIOR,
	in float SceneDepth, 	// 
	in float InvTanHalfFov, // 1 / tan(0.5 * fovy)
	in float ScreenRatioXY) // screen width / height
{

	vec2 ViewportUVDistortion = -TransformDirectionFromWorldToView(Normal).xy * (MaterialIOR - AIR_IOR); // UE use the positive one which I think is wrong...
	vec2 BufferUVDistortion = ViewportUVDistortion;// * FullResolutionDistortionPixelSize;
	BufferUVDistortion *= 1.f;
	// InvTanHalfFov only apply a correction for the distortion to be the same in screen space space whatever the FoV is (to make it consistent accross player setup).
	// However without taking depth into account, the distortion will actually be stronger the further away the camera is from the distortion surface.
	// So when zoomed-in the distortion will be higher than expected.
	// To fix this, a scale of 100/SurfaceDepth would be a good approximation to make the distortion properly scaled when the surface is zoomed in and/or further away (with distortion authored at 1meter being the reference strength)

	// Fix for Fov and aspect.
	vec2 FovFix = vec2(InvTanHalfFov/ ScreenRatioXY , InvTanHalfFov);
	//A fudge factor scale to bring values close to what they would have been under usual circumstances prior to this change.
	const float OffsetFudgeFactor = 0.00023;
	BufferUVDistortion *= 100.0f / SceneDepth * vec2(OffsetFudgeFactor, -OffsetFudgeFactor) * FovFix;

	return BufferUVDistortion;
}

void PostProcessUVDistortion(
	in float SceneDepth,
	in float DistortSceneDepth,	
	in float RefractionDepthBias,
	inout vec2 BufferUVDistortion)
{
	// Soft thresholding 
	float Bias = -RefractionDepthBias;
	float Range = clamp(abs(Bias * 0.5f), 0, 50);
	float Z = DistortSceneDepth;
	float ZCompare = SceneDepth; // screen pos.w
	float InvWidth = 1.0f / max(1.0f, Range);
	BufferUVDistortion *= clamp((Z - ZCompare) * InvWidth + Bias, 0.0f, 1.0f);

	//Scale up for better precision in low/subtle refractions at the expense of artefacts at higher refraction.
	// static const half DistortionScaleBias = 4.0f;
	// BufferUVDistortion *= DistortionScaleBias;
}

float RoughnessToVariance(in float roughness)
{
	float a11 = clamp(pow(roughness, 1.1f), 0.0f, 0.999f);
	return a11 / (1.0f - a11);
}

void main()
{
	// material distortion offset
	vec3 Normal = normalize(vViewNormal); // world
	float MaterialIOR = 1.51f;
	float SceneDepth = vScreenPos.w;
	float RefractionDepthBias = 0.5;
	float roughness = material.roughness;
	// Prevent silhouettes from geometry that is in front of distortion from being seen in the distortion 
	vec2 NDC = vScreenPos.xy / vScreenPos.w;
	vec2 ScreenUV = NDC * vec2(0.5f, 0.5f) + vec2(0.5f, 0.5f);
	vec3 I = normalize(cameraInfo.eye.rgb - vPosWorld);

	// Compute UV distortion
	vec2 BufferUVDistortion = ComputeBufferUVDistortion(normalize(vNormalWorld), MaterialIOR, SceneDepth, cameraView.InvTanHalfFov, cameraView.ScreenRatioXY);
	//vec2 BufferUVDistortion = ComputeUVDistortion(1.0, SceneDepth, MaterialIOR, normalize(vNormalWorld), I);

	// Sample depth at distortion offset
	float DistortSceneDepth = GetSceneDepth(ScreenUV + BufferUVDistortion);

	// Post process UV distortion according to depth
	if (SceneDepth > DistortSceneDepth)BufferUVDistortion = vec2(0.0f);
	outColor = vec4(BufferUVDistortion, RoughnessToVariance(roughness), 1.0);
}