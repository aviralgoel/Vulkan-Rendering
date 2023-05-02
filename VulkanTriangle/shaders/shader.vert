#version 450


layout(location = 0) in vec2 in_Postion;
layout(location = 1) in vec3 in_Color;

layout(location = 0) out vec3 fragColor;

void main() {
    // gl_Position and gl_VertexIndex are built in
    gl_Position = vec4(in_Postion, 0.0, 1.0);
    fragColor = in_Color;
}