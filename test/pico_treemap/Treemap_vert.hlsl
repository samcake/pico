#include "SceneTransform_inc.hlsl"
#include "Viewport_inc.hlsl"

Buffer<float4> rectBuffer : register(t0);

cbuffer PushUniform : register(b0)
{
    float viewportX;
    float viewportY;
    float sx;
    float sy;
}

struct VertexShaderOutput
{
    float4 coords : TEXCOORD;
    float4 position : SV_Position;
};

VertexShaderOutput mainVertex(uint ivid : SV_VertexID)
{
    VertexShaderOutput OUT;

    const int num_tris = 1;
    uint vid = ivid % (3 * num_tris);
    uint instance = ivid / (3 * num_tris);
    uint tvid = vid % 3;
    uint tid = vid / 3;
    
    
    float3 position = float3(0.0, 0.0, 0.0);
    float3 color = float3(1.0, 1.0, 1.0);

    position.xy = float2(((tvid == 1) ? 2.0 : 0.0), ((tvid == 2) ? 2.0 : 0.0));

    float4 rectVP = rectBuffer[instance];
    //float4 rect = rectVP /640.0;
    
   float4 rect = float4(
        -1.0 + 2.0 * rectVP.x / viewportX,
        -1.0 + 2.0 * rectVP.y / viewportY,
        2.0 * rectVP.z / viewportX,
        2.0 * rectVP.w / viewportY);

    float4 coords = float4(position.xy * rect.zw + rect.xy, 0, 1.0);

    OUT.position = coords;
    OUT.coords = float4(position.xy, 0.0f, 1.0f);

    return OUT;
}
