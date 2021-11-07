struct UBO
{
	float4x4 model;
	float4x4 view;
	float4x4 projection;
};
cbuffer ubo : register(b0) { UBO ubo; };

Texture2D textureInput : register(t1);
SamplerState samplerInput : register(s1);

float4 main([[vk::location(0)]] float2 inUV : TEXCOORD0) : SV_TARGET
{
	return float4(inUV, 0.0, 1.0);
}

