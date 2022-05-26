
#include "SceneTransform_inc.hlsl"

cbuffer UniformBlock1 : register(b1) {
    int   currentSampleIndex;
    float numSamples;
    float sx;
    float sy;
}

struct VertexShaderOutput
{
    float4 Coords : TEXCOORD;
    float3 wdir : WPOS;
    float4 Position : SV_Position;
};

VertexShaderOutput main(uint ivid : SV_VertexID)
{
    VertexShaderOutput OUT;

    const int num_tris = 1;
    uint vid = ivid % (3 * num_tris);
    uint instance = ivid / (3 * num_tris);
    uint tvid = vid % 3;
    uint tid = vid / 3;
    

    float2 uv = float2(((tvid == 1) ? 2.0 : 0.0), ((tvid == 2) ? 2.0 : 0.0));
    float2 ndc = uv * 2.0 - 1.0;

    float4 coords = float4(ndc, uv);
    float4 clipPos = _projection.clipPosPersAt(ndc.x, ndc.y, 1.0);

    float3 eyeDir = eyeFromClipSpace(_projection, ndc.xy);
    float3 worldDir = worldFromEyeSpaceDir(_view, eyeDir);

    OUT.wdir = worldDir;
    OUT.Position = clipPos;
    OUT.Coords = coords;
 
    return OUT;
}

