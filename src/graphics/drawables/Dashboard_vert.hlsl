#include "SceneTransform_inc.hlsl"
#include "Viewport_inc.hlsl"


cbuffer UniformBlock1 : register(b1) {
    int   currentSampleIndex;
    float numSamples;
    float sx;
    float sy;
}

struct VertexShaderOutput
{
    float4 Coords : TEXCOORD;
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
    
    uint sampleIdx = (instance - currentSampleIndex) % (int)numSamples;

    float3 position = float3(0.0, 0.0, 0.0);
    float3 color = float3(1.0, 1.0, 1.0);


    position.xy = float2(((tvid == 1) ? 2.0 : 0.0), ((tvid == 2) ? 2.0 : 0.0));

    float4 coords = float4(position.xy, sx, sy);

    uint4 tsAB = timerBuffer[sampleIdx];
    uint2 ts0 = tsAB.xy;
    uint2 ts1 = tsAB.zw;
    float  highDelta = 0.0; // (ts1.y - ts0.y) * 1000.0;
    float time = highDelta  + (ts1.x - ts0.x) / 10000;
    
    float width = (1.0 / numSamples);

    position.x *= width * 0.5;
    position.x += 1.0 - instance * 2 * width;

    position.y *= 0.5 * sy * time * 0.001;

    position.z -= 2.0;

    if (instance == 0)
    {
        position.xy = (coords.xy * 2 - float2(1.0, 1.0)) * float2(1.0, 0.1);
        //coords.zw = position.xy;
        coords.w = -1;
        position.z -= 0.1;   
    }
    
    float3 eyePosition = position;
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);

    OUT.Position = clipPos;
    OUT.Coords = coords;
 
    return OUT;
}

