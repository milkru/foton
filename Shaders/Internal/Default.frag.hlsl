struct UBO
{
	float4x4 model;
	float4x4 view;
	float4x4 projection;
};
cbuffer ubo : register(b0) { UBO ubo; };

Texture2D textureInput : register(t1);
SamplerState samplerInput : register(s2);

float4 main([[vk::location(0)]] float2 inUV : TEXCOORD0) : SV_TARGET
{
	float4 fragCol = textureInput.Sample(samplerInput, inUV);
	return fragCol;
}
