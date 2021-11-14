#version 450

layout (location = 0) in vec2 inUV;

struct whaaa
{
	float x;
	float y;
};

layout (binding = 0) uniform UBO
{
	mat4 model;
	mat4 view;
	mat4 projection;
	whaaa whaa2;
} ubo;

layout (binding = 1) uniform sampler2D inputTexture;

layout (location = 0) out vec4 outColor;

void main()
{
	// TODO: Maybe default shader should be a logo of FOTON makde from code!
	vec4 fragPos = texture(inputTexture, inUV);
	outColor = vec4(inUV, 1.0, 1.0);
	outColor = fragPos;
}
