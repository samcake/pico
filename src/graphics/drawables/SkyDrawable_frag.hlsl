#include "Sky_inc.hlsl"

struct PixelShaderInput
{
    float4 coords : TEXCOORD;
    float3 wdir : WPOS;
};


float4 main(PixelShaderInput IN) : SV_Target
{
    float3 wdir = normalize(IN.wdir);


    return float4(abs(wdir), 1.0);
    return float4(IN.coords.zw, 0.0, 1.0);
}