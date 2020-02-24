struct PixelShaderInput
{
	float4 Color    : COLOR;
};

float4 mainPixel( PixelShaderInput IN ) : SV_Target
{
    return IN.Color;
}