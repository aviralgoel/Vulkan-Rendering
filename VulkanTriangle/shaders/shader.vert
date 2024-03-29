#version 450


layout(location = 0) in vec3 in_Postion;
layout(location = 1) in vec3 in_Color;
layout(location = 2) in vec2 in_TexCoords;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

layout(binding=0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

void main() {
    // gl_Position and gl_VertexIndex are built in
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(in_Postion, 1.0);  // (x,y,z, 1) homegenous
    fragColor = in_Color;
	fragTexCoord = in_TexCoords;
}