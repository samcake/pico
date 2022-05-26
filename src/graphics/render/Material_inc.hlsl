//
// Material API
// 
#ifndef MATERIAL_INC
#define MATERIAL_INC

struct Material {
    float4 color;
    float metallic;
    float roughness;
    float spareA;
    float spareB;
    float4 emissive;
    uint4 textures;
};

#endif