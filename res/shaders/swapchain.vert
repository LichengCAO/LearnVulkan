#version 450

layout(location = 0) out vec2 fragTexCoord;

vec2 uvs[4] = {
	{0.0f, 0.0f},
	{1.0f, 0.0f},
	{1.0f, 1.0f},
	{0.0f, 1.0f}
};

vec2 positions[4] = {
    {-1.0f, -1.0f},
    { 1.0f, -1.0f},
    { 1.0f,  1.0f},
    {-1.0f,  1.0f}
};

int indices[6] = {
    2, 1, 0,
    0, 3, 2
};

void main(){
    int index = indices[gl_VertexIndex];
    gl_Position = vec4(positions[index], 0.0f, 1.0f);
    fragTexCoord = uvs[index];
}