#include "SceneTransform_inc.hlsl"


cbuffer UniformBlock1 : register(b1) {
    int4   _instance;
    float _spriteSize;
    float _perspectiveSpriteX;
    float _perspectiveDepth;
    float _showPerspectiveDepthPlane;
}

/*struct VertexPosColor
{
    float3 Position : POSITION;
    //  float3 Normal : NORMAL;
    float4 Color : COLOR;
};*/

struct PointPosColor {
    float x;
    float y;
    float z;
    uint  color;
};

StructuredBuffer<PointPosColor>  BufferIn : register(t1);

struct VertexShaderOutput
{
    float4 Color    : COLOR;
    float2 sprite   : SPRITE;
    float4 Position : SV_Position;
};

VertexShaderOutput main(uint vidT : SV_VertexID)
{
    VertexShaderOutput OUT;
    uint vid = vidT / 3;
    uint sid = vidT - vid * 3;

    uint color = BufferIn[vid].color;
    const float INT8_TO_NF = 1.0 / 255.0;
    float r = INT8_TO_NF * (float)((color >> 0) & 0xFF);
    float g = INT8_TO_NF * (float)((color >> 8) & 0xFF);
    float b = INT8_TO_NF * (float)((color >> 16) & 0xFF);
    float3 position = float3(BufferIn[vid].x, BufferIn[vid].y, BufferIn[vid].z);

    Transform _model = node_getWorldTransform(_instance.x);

    position = worldFromObjectSpace(_model, position);
    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);
    float eyeLinearDepth = -eyePosition.z;

    // make the sprite offsetting the vert from the pointcloud pos in clip space
    // offset is expressed in ndc with spriteSize expressed in pixels
    const float2 invRes = float2(1.0 / _viewport.z, 1.0 / _viewport.w);
    float2 spriteOffset = invRes.xy * _spriteSize;

    // fixed sprite pixel size in depth or perspective correct 3d size ?
    const float isPerspective = float((0.5 * (clipPos.x / clipPos.w) + 0.5) <= _perspectiveSpriteX);
    spriteOffset *= (isPerspective > 0.0 ? _perspectiveDepth : eyeLinearDepth);

    // Apply scaling to the sprite coord and offset pointcloud pos
    OUT.sprite = float2(((sid == 1) ? 3.0 : -1.0), ((sid == 2) ? 3.0 : -1.0));
    spriteOffset *= OUT.sprite;

    clipPos.xy += spriteOffset;
    OUT.Position = clipPos;

    OUT.Color = float4(r, g, b, 1.0f);

    // For debug, represent the points close to the '_perspectiveDepth' with a control color
    float distanceToPerspectiveDepth = (eyeLinearDepth - _perspectiveDepth);
    float perspectivePlaneTolerance = _showPerspectiveDepthPlane * 0.01 * _perspectiveDepth;
    if ((distanceToPerspectiveDepth * distanceToPerspectiveDepth) < (perspectivePlaneTolerance * perspectivePlaneTolerance)) {
        OUT.Color = float4(1.0, 1.0, 1.0, 1.0);
    }

    return OUT;
}



//
// Initial version using Input Assembly
// Not used anymore
//

cbuffer UniformBlock1 : register(b1) {
    Transform _model;
};

struct VertexPosColor0 {
    float3 Position : POSITION;
    //  float3 Normal : NORMAL;
    float4 Color : COLOR;
};

struct VertexShaderOutput0 {
    float4 Color    : COLOR;
    float4 Position : SV_Position;
};

VertexShaderOutput0 main0(VertexPosColor0 IN) {
    VertexShaderOutput0 OUT;

    float3 position = IN.Position;

    position = worldFromObjectSpace(_model, position);
    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);

    OUT.Position = clipPos;
    OUT.Color = float4(IN.Color.xyz, 1.0f);

    return OUT;
}