#version 450

layout(location = 0) in vec3 vViewNormal;
layout(location = 1) in vec4 vScreenPos;

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

layout(location = 0) out vec4 outColor;

#define AIR_IOR 1.0f

float GetSceneDepth(in vec2 uv)
{
	return texture(texDepth, uv).r;
}

vec2 ComputeBufferUVDistortion(
	in vec3 viewNormal, // normal in view space
	in float MaterialIOR,
	in float SceneDepth, 	// 
	in float InvTanHalfFov, // 1 / tan(fov)
	in float ScreenRatioXY) // screen width / height
{
	vec2 ViewportUVDistortion = viewNormal.xy * (MaterialIOR - AIR_IOR);
	vec2 BufferUVDistortion = ViewportUVDistortion;// * FullResolutionDistortionPixelSize;

	// InvTanHalfFov only apply a correction for the distortion to be the same in screen space space whatever the FoV is (to make it consistent accross player setup).
	// However without taking depth into account, the distortion will actually be stronger the further away the camera is from the distortion surface.
	// So when zoomed-in the distortion will be higher than expected.
	// To fix this, a scale of 100/SurfaceDepth would be a good approximation to make the distortion properly scaled when the surface is zoomed in and/or further away (with distortion authored at 1meter being the reference strength)

	// Fix for Fov and aspect.
	vec2 FovFix = vec2(InvTanHalfFov, ScreenRatioXY * InvTanHalfFov);
	//A fudge factor scale to bring values close to what they would have been under usual circumstances prior to this change.
	const float OffsetFudgeFactor = 0.00023;
	BufferUVDistortion *= 100.0f/SceneDepth * vec2(OffsetFudgeFactor, -OffsetFudgeFactor) * FovFix;

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
	BufferUVDistortion *= saturate((Z - ZCompare) * InvWidth + Bias);

	//Scale up for better precision in low/subtle refractions at the expense of artefacts at higher refraction.
	// static const half DistortionScaleBias = 4.0f;
	// BufferUVDistortion *= DistortionScaleBias;
}

void main()
{
	// material distortion offset
	vec3 Normal = normalize(vNormal); // world
	float MaterialIOR = 1.5f;
	float SceneDepth = vScreenPos.w;
	float RefractionDepthBias = 0.0;
	// Prevent silhouettes from geometry that is in front of distortion from being seen in the distortion 
	vec2 NDC = vScreenPos.xy / vScreenPos.w;
	vec2 ScreenUV = NDC * vec2(0.5f, 0.5f) + vec2(0.5f, 0.5f);

	// Compute UV distortion
	vec2 BufferUVDistortion = ComputeBufferUVDistortion(Normal, MaterialIOR, SceneDepth, InvTanHalfFov, ScreenRatioXY);

	// Sample depth at distortion offset
	float DistortSceneDepth = GetSceneDepth(ScreenUV + BufferUVDistortion);

	// Post process UV distortion according to depth
	PostProcessUVDistortion(SceneDepth, DistortSceneDepth, BufferUVDistortion, RefractionDepthBias);

	// store positive and negative offsets separately
	vec2 PosOffset = max(BufferUVDistortion,0);
	vec2 NegOffset = abs(min(BufferUVDistortion,0));

	// output positives in R|G channels and negatives in B|A channels
	OutColor = vec4(PosOffset.x, PosOffset.y, NegOffset.x, NegOffset.y);
}