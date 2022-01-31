#include "SceneTransform_inc.hlsl"
#include "SceneModel_inc.hlsl"

#include "Color_inc.hlsl"
#include "Paint_inc.hlsl"
#include "Shading_inc.hlsl"
#include "Surface_inc.hlsl"


int SHOW_UVMESH_OUTSIDE_BIT() { return 0x00000001; }
int SHOW_UVMESH_FACES_BIT() { return 0x00000002; }
int SHOW_UVMESH_EDGES_BIT() { return 0x00000004; }
int SHOW_UVMESH_FACES_ID_BIT() { return 0x00000008; }

int SHOW_UV_GRID_BIT() { return 0x00000010; }
int MASK_OUTSIDE_UV_BIT() { return 0x00000020; }
int LINEAR_SAMPLER_BIT() { return 0x00000040; }
int LIGHT_SHADING_BIT() { return 0x00000080; }

int MAKE_UVMESH_MAP_BIT() { return 0x00000100; }
int RENDER_UV_SPACE_BIT() { return 0x00000200; }
int DRAW_EDGE_LINE_BIT() { return 0x00000400; }
int RENDER_WIREFRAME_BIT() { return 0x00000800; }

int INSPECTED_MAP_BITS() { return 0x000F0000; }
int INSPECTED_MAP_OFFSET() { return 16; }
int INSPECTED_MAP(int drawMode) { return (drawMode & INSPECTED_MAP_BITS()) >> INSPECTED_MAP_OFFSET(); }

int DISPLAYED_COLOR_BITS() { return 0x00F00000; }
int DISPLAYED_COLOR_OFFSET() { return 20; }
int DISPLAYED_COLOR(int drawMode) { return (drawMode & DISPLAYED_COLOR_BITS()) >> DISPLAYED_COLOR_OFFSET(); }

// 
// UVMesh map API
//
Texture2D uvmesh_map : register(t11);
float uvmesh_isOutside(float4 uvmesh_texel) {
    return float(uvmesh_texel.w == 0.0f);
}
float uvmesh_isEdge(float4 uvmesh_texel) {
    return float(uvmesh_texel.w < 0.0f);
}
float uvmesh_isFace(float4 uvmesh_texel) {
    return float(uvmesh_texel.w > 0.0f);
}
int uvmesh_triangle(float4 uvmesh_texel) {
    return int(abs(uvmesh_texel.w)) - 1;
}

//
// Computed map API
//
Texture2D computed_map : register(t12);

//
// Main
// 


cbuffer UniformBlock1 : register(b1) {
    int _nodeID;
    int _partID;

    int _numNodes;
    int _numParts;
    int _numMaterials;
    int _numEdges;
    int _drawMode;
    int _inspectedTriangle;
    int _numInspectedTriangles;


    float _uvCenterX;
    float _uvCenterY;
    float _uvScale;
    float _colorMapBlend;

    float _kernelRadius;
    int _inspectedTexelX;
    int _inspectedTexelY;
}

struct PixelShaderInput{
    float3 EyePos : EPOS;
    float3 Normal   : NORMAL;
    float2 Texcoord : TEXCOORD;
    float4 TriPos : TBPOS;
};

// Compute tangent space and final normal
float3 computeNormal(float3 surfNormal, float3 normalMap, float3 eyePos, float2 uv) {
    float3 q0 = ddx(eyePos);
    float3 q1 = ddy(eyePos);
    float2 st0 = ddx(uv);
    float2 st1 = ddy(uv);

    float3 S = normalize(q0 * st1.y - q1 * st0.y);
    float3 T = normalize(-q0 * st1.x + q1 * st0.x);
    float3 normal = surfNormal;
/*
    normalMap.xy = 1.0 * normalMap.xy;
    float3x3 tsn = float3x3(S, T, normal);
    normal = mul(tsn, normalMap.xyz);
    normal = normalize(normal);
  */  return normal;
}

// Map Color / Computed blend
float colorMapBlend(float2 uv) {
    float sqrt2 = 1.4142135;
    return dot(0.5 * float2(sqrt2, sqrt2), uv) < (_colorMapBlend * sqrt2);
}

int samplerIdx() {
    return (_drawMode & LINEAR_SAMPLER_BIT()) != 0;
}

// Drawgrid
float3 drawGrid(float2 uv, float3 color) {
    float tex_width, tex_height, tex_elements;
    material_textures.GetDimensions(tex_width, tex_height, tex_elements);

    float3 texcoord = float3(uv.x * tex_width, uv.y * tex_height, 1.0f);

    float3 grid = paintStripe(texcoord, tex_width * 0.1f, 1.5f);
    color = lerp(color, float3(1.0, 0.0, 0.0), grid.x);
    color = lerp(color, float3(0.0, 1.0, 0.0), grid.y);

    grid = paintStripe(texcoord, tex_width * 0.01f, 0.5f);
    color = lerp(color, float3(0.8, 0.8, 1.8), max(grid.x, grid.y));

    return color;
}

// Tool / Debug Coloring from UVMesh map
float3 drawUVMeshTool(float2 uv, float3 color, int numIndices) {
    float4 uvmesh = uvmesh_map.Sample(uSampler0[0], uv.xy);

    // uvmesh faces
    if (_drawMode & SHOW_UVMESH_FACES_BIT()) {
        color = lerp(color, uvmesh.xyz, uvmesh_isFace(uvmesh));
    }

    if (_drawMode & SHOW_UVMESH_FACES_ID_BIT()) {
        color = lerp(color, color_rainbow(float(uvmesh_triangle(uvmesh)), float(numIndices/ 3)), 0.5f * (1- uvmesh_isOutside(uvmesh)));
    }

    // uvmesh edge texels
    if (_drawMode & SHOW_UVMESH_EDGES_BIT()) {
        color = lerp(color, uvmesh.xyz, uvmesh_isEdge(uvmesh));
    }

    // uvmesh outside
    if (_drawMode & SHOW_UVMESH_OUTSIDE_BIT()) {
        color = lerp(color, float3(1.0f, 0.0f, 0.0f), 0.6f * uvmesh_isOutside(uvmesh));
    }

    // Add uv stripes
    if (_drawMode & SHOW_UV_GRID_BIT()) {
        color = drawGrid(uv, color);
    }

    return color;
}

float4 main(PixelShaderInput IN) : SV_Target{
    if (_drawMode & DRAW_EDGE_LINE_BIT()) {
        return float4(1.0, 1.0, 0.0, 1.0);
    }

    int numIndices = part_array[_partID].numIndices;

    int matIdx = part_array[_partID].material;// < 0.0 ? 0 : floor(IN.Material));
//    int matIdx = asint(IN.Material);// < 0.0 ? 0 : floor(IN.Material));
    Material m = material_array[matIdx];

    float4 mapNor = float4(0.0, 0.0, 0.0, 0.0);
    if (m.textures.y != -1) {
        mapNor = float4(material_textures.Sample(uSampler0[samplerIdx()], float3(IN.Texcoord.xy, m.textures.y)).xyz, 1.0);
    }

    float4 mapN = (mapNor *2.0 - 1.0);
    float3 surfNormal = normalize(IN.Normal);
    float3 normal = computeNormal(surfNormal, mapN, IN.EyePos.xyz, IN.Texcoord.xy);

    normal = worldFromEyeSpaceDir(_view, normal);

    float4 rmaoMap = float4(0.0, 0.0, 0.0, 0.0);
    if (m.textures.z != -1) {
        rmaoMap = material_textures.Sample(uSampler0[samplerIdx()], float3(IN.Texcoord.xy, m.textures.z));
    }

    float3 baseColor = float3(1.0, 1.0, 1.0);
    // with albedo from property or from texture
    baseColor = m.color;
    if (m.textures.x != -1) {
        baseColor = material_textures.Sample(uSampler0[samplerIdx()], float3(IN.Texcoord.xy, m.textures.x)).xyz;
    }

    switch (DISPLAYED_COLOR(_drawMode)) {
    case 0: break;
    case 1: baseColor = normal; break;
    case 2: baseColor = surfNormal; break;
    case 3: baseColor = mapNor; break;
    }

    const float3 globalD = normalize(float3(0.0f, 1.0f, 0.0f));
    const float globalI = 0.3f;
    const float3 lightD = normalize(float3(1.0f, -1.0f, -1.0f));
    const float lightI = 0.8f;

    float NDotL = clamp(dot(normal, -lightD), 0.0f, 1.0f);
    float NDotG = clamp(dot(normal, -globalD), 0.0f, 1.0f);

    float3 shading = float3(1.0, 1.0, 1.0);
    if ((LIGHT_SHADING_BIT() & _drawMode)) {
        shading = (NDotL * lightI + NDotG * globalI);
    }

  //  normal = T;

   // baseColor = 0.5 * (normal + float3(1.0, 1.0, 1.0));
   // baseColor = mapNor.xyz;
  //  baseColor = color_rainbow(IN.Material, float(_numMaterials));
  //  baseColor = color_rainbow(_partID, float(_numParts));
  //  baseColor = color_rainbow(_nodeID, float(_numNodes));
  //  baseColor = float3(IN.Texcoord.x, IN.Texcoord.y, 0.0f);
    
   //  baseColor = mapN;

    // Compute map instead of baseColor
    float4 compute = computed_map.Sample(uSampler0[samplerIdx()], IN.Texcoord.xy);
    baseColor = lerp(baseColor, compute.xyz, colorMapBlend(IN.Texcoord.xy));

    // Draw tools
    baseColor = drawUVMeshTool(IN.Texcoord.xy, baseColor, numIndices);

    float3 emissiveColor = float3 (0.0, 0.0, 0.0);
    if (m.textures.w != -1) {
        emissiveColor = material_textures.Sample(uSampler0[samplerIdx()], float3(IN.Texcoord.xy, m.textures.w)).xyz;
    }
 
    float3 color = shading * baseColor + emissiveColor;

    // Add uv stripes
    if (_drawMode & SHOW_UV_GRID_BIT()) {
        color = drawGrid(IN.Texcoord, color);
    }
    
    return float4(color, 1.0);
}

float4 main_connectivity(PixelShaderInput IN) : SV_Target{
    return float4(color_rainbow(IN.TriPos.w, IN.TriPos.x), 1.0);
}


float4 main_kernelSamples(PixelShaderInput IN) : SV_Target{
    float r2 = dot(IN.TriPos.yz, IN.TriPos.yz);
    if ((r2 >= 1.0)) discard;

    if (IN.TriPos.x > 0) {
        return float4(color_rainbow(IN.TriPos.w, IN.TriPos.x), 1.0);
    }

    float innerTest = float(r2 < 0.01);
    float outerTest = float(r2 > 0.9);

    float3 color = float3(0.5, 0.5, 0.5);
    color = lerp(color, float3(1, 0, 0), innerTest);
    color = lerp(color, float3(1,1,1), outerTest);

    if (innerTest + outerTest < 0.1) discard;

    return float4(color, innerTest + outerTest);
}

float4 main_uvmesh(PixelShaderInput IN) : SV_Target{
    return float4(IN.TriPos.xyzw);
}

float4 main_uvspace(PixelShaderInput IN) : SV_Target{
    
    float3 color = float3(0,0,0);

    int numIndices = part_array[_partID].numIndices;
    int matIdx = part_array[_partID].material;
    Material m = material_array[matIdx];
    int inspectedMapId = m.textures[INSPECTED_MAP(_drawMode)];

    if (inspectedMapId != -1) {
        color = material_textures.Sample(uSampler0[samplerIdx()], float3(IN.Texcoord.xy, inspectedMapId)).xyz;
    }

    // Compute map
    float4 compute = computed_map.Sample(uSampler0[samplerIdx()], IN.Texcoord.xy);
    color = lerp(color, compute.xyz, colorMapBlend(IN.Texcoord.xy));

    // Draw tools
    color = drawUVMeshTool(IN.Texcoord.xy, color, numIndices);

    return float4(color, 1.0);
}

float4 main_uvmesh_point(PixelShaderInput IN) : SV_Target{
    return float4(1.0, 1.0, 1.0, 1.0);
}
