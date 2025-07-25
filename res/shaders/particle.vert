#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

void main(){
    gl_PointSize = 5.0;
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}