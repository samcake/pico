#include "Sky_inc.hlsl"
#include "Color_inc.hlsl"

struct PixelShaderInput
{
    float4 coords : TEXCOORD;
    float3 wdir : WPOS;
};

float4 main(PixelShaderInput IN) : SV_Target
{
    float3 dir = normalize(IN.wdir);

    //float3 color = color_rgbFromDir(dir);

    float3 color = SkyColor(dir);
    return float4(color, 1.0);
    return float4(IN.coords.zw, 0.0, 1.0);

}