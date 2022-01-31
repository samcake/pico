#include "SceneTransform_inc.hlsl"

cbuffer UniformBlock1 : register(b1) {
    int _nodeID;
    int _flags;
    int _spareA;
    int _spareB;
}
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
    const int box_num_edges = 12;
    const int num_edges = (_flags & SHOW_LOCAL_BOUND) * box_num_edges + (_flags & SHOW_WORLD_BOUND) * box_num_edges;

    uint vid = ivid % (2 * num_edges);
    uint itemid = ivid / (2 * num_edges);
    uint svid = vid % 2;
    uint lid = vid / 2;

    float3 position = float3(0.0, 0.0, 0.0);
    float3 color = float3(1.0, 1.0, 1.0);

    ItemInfo _item = item_getInfo(itemid);
    Transform _model = node_getWorldTransform(_item.nodeID);
    Box _box = drawable_getLocalBox(_item.drawableID);

    if ((_flags & SHOW_LOCAL_BOUND) && lid < (box_num_edges)) {
        //lid -= 0;
        int2 edge = _box.getEdge(lid);
        position = _box.getCorner(edge[svid]);
        color = float3(float(lid < 4), float(lid >= 4 && lid < 8), float(lid >= 8));
        position = worldFromObjectSpace(_model, position);
    }
    else {
        Box _world_box = worldFromObjectSpace(_model, _box);
    
        lid -= box_num_edges;
        int2 edge = _box.getEdge(lid);
        position = _world_box.getCorner(edge[svid]);
        color = float3(float(lid < 4), float(lid >= 4 && lid < 8), float(lid >= 8));
    }

    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);

    OUT.Position = clipPos;
    OUT.Color = float4(color, 1.0f);
 
    return OUT;
}

