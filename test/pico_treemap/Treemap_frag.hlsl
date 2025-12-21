
struct PixelShaderInput
{
    float4 coords : TEXCOORD;
};

float4 mainPixel(PixelShaderInput IN) : SV_Target
{
    float maxCoord = max(IN.coords.x, IN.coords.y);
    if (maxCoord > 1.0) discard;

    float3 color = lerp(
                       lerp(float3(1, 0, 0), float3(0, 1, 0), IN.coords.x),
                       lerp(float3(0, 0, 1), float3(1, 1, 0), IN.coords.x),
                       IN.coords.y);


    return float4(color, 1);
}
