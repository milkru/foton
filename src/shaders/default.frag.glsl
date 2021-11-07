#version 450

layout (location = 0) in vec2 inUV;

layout (binding = 0) uniform UBO
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout (binding = 1) uniform sampler2D inputTexture;

layout (location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(inUV, 0.0, 1.0);
}

