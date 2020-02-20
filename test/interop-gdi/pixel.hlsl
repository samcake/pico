struct PixelShaderInput
{
	float4 Color    : COLOR;
    float4 Texcoord : TEXCOORD;
};

Texture2D uTex0 : register(t0);
SamplerState uSampler0 : register(s0);

float4 mainPixel(PixelShaderInput input) : SV_TARGET
{
	return uTex0.Sample(uSampler0, input.Texcoord.xy);
    //return input.Color;
}