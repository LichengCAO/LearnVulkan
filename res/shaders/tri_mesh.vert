#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;

layout (location = 0) out vec3 outColor;

//push constants block
layout( push_constant ) uniform constants
{
	vec4 data;
	mat4 render_matrix;
	vec2 d2;
	vec3 d3;
	vec3 d4;
} PushConstants;

void main()
{
	gl_Position = PushConstants.render_matrix * vec4(vPosition, 1.0f);
	outColor = vColor;
}