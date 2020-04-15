struct PixelShaderInput
{
    float4 Color    : COLOR;
};

float4 main(PixelShaderInput IN) : SV_Target
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}