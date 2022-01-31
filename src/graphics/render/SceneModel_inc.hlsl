//
// SceneModel API
//

#include "Mesh_inc.hlsl"
#include "Material_inc.hlsl"

StructuredBuffer<Material>  material_array : register(t9);

Texture2DArray material_textures : register(t10);

SamplerState uSampler0[2] : register(s0);

SamplerState materialMapSampler() { return uSampler0[0]; }

//
// Model & Parts
// 

struct Part {
    uint numIndices;
    uint indexOffset;
    uint vertexOffset;
    uint attribOffset;
    uint material;
    uint numEdges;
    uint edgeOffset;
    uint spareC;
};

StructuredBuffer<Part>  part_array : register(t1);