#include "SceneTransform_inc.hlsl"

cbuffer UniformBlock1 : register(b1) {
    int   _nodeID;
    float sx;
    float sy;
    float sz;
}



struct VertexPosColor
{
    float3 Position : POSITION;
    //  float3 Normal : NORMAL;
    float4 Color : COLOR;
};

struct VertexShaderOutput
{
    float4 Color    : COLOR;
    float4 Position : SV_Position;
};

VertexShaderOutput main(uint ivid : SV_VertexID)
{
    VertexShaderOutput OUT;

    const int box_num_tris = 6 * 2;
    const int num_tris = box_num_tris;

    uint vid = ivid % (3 * num_tris);
    uint instance = ivid / (3 * num_tris);
    uint tvid = vid % 3;
    uint tid = vid / 3;

    float3 position = float3(0.0, 0.0, 0.0);
    float3 color = float3(1.0, 1.0, 1.0);

    Transform _model = node_getWorldTransform(_nodeID);
    Transform _view = cam_view();
    Projection _projection = cam_projection();
    
    Box _box;
    _box._center = float3(0.0, 0.0, 0.0);
    _box._size = float3(sx, sy,sz);

    if (tid < (box_num_tris)) {
        int3 tri = _box.getTriangle(tid);
        position = _box.getCorner(tri[tvid]);
        color = float3(float(tid < 4), float(tid >= 4 && tid < 8), float(tid >= 8));
    }

    position = worldFromObjectSpace(_model, position);
    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);

    OUT.Position = clipPos;
    OUT.Color = float4(color, 1.0f);
 
    return OUT;
}

