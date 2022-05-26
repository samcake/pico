#include "SceneTransform_inc.hlsl"


cbuffer UniformBlock1 : register(b1) {
    int  _nodeID;
    int  _flags;
    int _spareA;
    int _spareB;
}

static const int SHOW_TRANSFORM = 0x00000001;
static const int SHOW_BRANCH = 0x00000002;
static const int SHOW_LOCAL_BOUND = 0x00000004;
static const int SHOW_WORLD_BOUND = 0x00000008;

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

    const int transform_num_edges = 3;
    const int node_num_edges = 1;
    const int num_edges = (_flags & SHOW_TRANSFORM) * transform_num_edges + (_flags & SHOW_BRANCH) * node_num_edges;

    uint vid = ivid % (2 * num_edges);
    uint nodeid = ivid / (2 * num_edges);
    uint svid = vid % 2;
    uint lid = vid / 2;

    float3 position = float3(0.0, 0.0, 0.0);
    float3 color = float3(1.0, 1.0, 1.0);

    Transform _model = node_getWorldTransform(nodeid);
    Transform _modelLocal = node_getTransform(nodeid);

    if ((_flags & SHOW_TRANSFORM) && lid < (transform_num_edges)) {
        position = float(svid) * float3(float(lid == 0), float(lid == 1), float(lid == 2));
        color = float3(float(lid == 0), float(lid == 1), float(lid == 2));
    }
    else {
        position = float(svid) * objectFromWorldSpaceDir(_modelLocal, -_modelLocal.col_w());
    }

    position = worldFromObjectSpace(_model, position);
    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);

    OUT.Position = clipPos;
    OUT.Color = float4(color, 1.0f);
 
    return OUT;
}

