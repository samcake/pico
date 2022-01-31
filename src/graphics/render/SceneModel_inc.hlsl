//
// SceneModel API
//

#include "Mesh_inc.hlsl"
#include "Part_inc.hlsl"
#include "Material_inc.hlsl"

StructuredBuffer<Material>  material_array : register(t9);

Texture2DArray material_textures : register(t10);

SamplerState uSampler0[2] : register(s0);

SamplerState materialMapSampler() { return uSampler0[0]; }


StructuredBuffer<Part>  part_array : register(t1);