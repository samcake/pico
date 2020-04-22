struct PixelShaderInput
{
    float4 Color    : COLOR;
    float2 sprite : SPRITE;
};

float4 main(PixelShaderInput IN) : SV_Target
{
    float r2 = dot(IN.sprite, IN.sprite);

    if ((r2 >= 1.0)) discard;
    
    //return IN.Color;
    return float4(IN.Color.xyz, step(r2, 1.0));
}