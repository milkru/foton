#version 450

layout (location = 0) in vec2 inUV;

struct whaaa
{
	float x;
	int y[2];
};

layout (binding = 0) uniform UBO
{
	whaaa w[2][3];
	float y;
} ubo[2];

layout (binding = 1) uniform sampler2D inputTexture;

layout (binding = 2) uniform sampler2D inputTexture2;

layout (binding = 3) uniform sampler2D inputTexture3;

layout (location = 0) out vec4 outColor;

void main()
{
	// TODO: Default shader should be a logo of FOTON made from code!
	vec4 fragCol = texture(inputTexture, inUV);
	vec4 fragCol2 = texture(inputTexture2, inUV);
	outColor = vec4(inUV, 1.0, 1.0);
	//outColor = vec4(0, 0, 0, 1);
	outColor = fragCol + fragCol2;
}
