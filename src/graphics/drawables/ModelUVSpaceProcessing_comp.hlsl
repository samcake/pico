#include "Color_inc.hlsl"
#include "SceneModel_inc.hlsl"
#include "MeshEdge_inc.hlsl"
#include "Triangle_inc.hlsl"

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


#define THREAD_GROUP_SIDE 4

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
// Kernel 
// 

cbuffer UniformBlock1 : register(b0) {
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



static const uint numKernelSamples = 16;
static const uint NUM_KERNEL_SAMPLES = 16;
static float2 kernelDistribution_PoissonDisk[NUM_KERNEL_SAMPLES] =
{   // This is a poisson distribution for the example
    float2(0.2770745f, 0.6951455f),
    float2(0.1874257f, -0.02561589f),
    float2(-0.3381929f, 0.8713168f),
    float2(0.5867746f, 0.1087471f),
    float2(-0.3078699f, 0.188545f),
    float2(0.7993396f, 0.4595091f),
    float2(-0.09242552f, 0.5260149f),
    float2(0.3657553f, -0.5329605f),
    float2(-0.3829718f, -0.2476171f),
    float2(-0.01085108f, -0.6966301f),
    float2(0.8404155f, -0.3543923f),
    float2(-0.5186161f, -0.7624033f),
    float2(-0.8135794f, 0.2328489f),
    float2(-0.784665f, -0.2434929f),
    float2(0.9920505f, 0.0855163f),
    float2(-0.687256f, 0.6711345f)
};
static float2 kernelDistribution_Grid[NUM_KERNEL_SAMPLES] =
{   // This is a regular grid distribution for the example
    float2(-0.75f, -0.75f),
    float2(-0.75f, -0.25f),
    float2(-0.75f, +0.25f),
    float2(-0.75f, +0.75f),
    float2(-0.25f, -0.75f),
    float2(-0.25f, -0.25f),
    float2(-0.25f, +0.25f),
    float2(-0.25f, +0.75f),
    float2(+0.25f, -0.75f),
    float2(+0.25f, -0.25f),
    float2(+0.25f, +0.25f),
    float2(+0.25f, +0.75f),
    float2(+0.75f, -0.75f),
    float2(+0.75f, -0.25f),
    float2(+0.75f, +0.25f),
    float2(+0.75f, +0.75f),
};

// Read previous state from 
RWTexture2D<float4> out_buffer : register(u0);

[numthreads(THREAD_GROUP_SIDE, THREAD_GROUP_SIDE, 1)]
void main_imageSpaceBlurBrutForce(uint3 DTid : SV_DispatchThreadID)
{
    int2 pixelCoord = DTid.xy;

    int matIdx = part_array[0].material;
    Material m = material_array[matIdx];
    int mapId = m.textures[INSPECTED_MAP(_drawMode)];
    if (mapId == -1) {
        return;
    }

    float3 color = float3(0,0,0);

    float tex_width, tex_height, tex_elements;
    material_textures.GetDimensions(tex_width, tex_height, tex_elements);

    int radius = floor(_kernelRadius);
    float numSamples = 0;
    int2 coord = pixelCoord;
    int minX = max(0, pixelCoord.x - radius);
    int maxX = min(int(tex_width) - 1, pixelCoord.x + radius);

    int minY = max(0, pixelCoord.y - radius);
    int maxY = min(int(tex_height) - 1, pixelCoord.y + radius);

    float maskOutside = (_drawMode & MASK_OUTSIDE_UV_BIT() ? 1.0 : 0.0);

    for (int i = minX; i <= maxX; ++i) {
        for (int j = minY; j <= maxY; ++j) {
            float3 texel = material_textures[uint3(i, j, mapId)].xyz;
            float4 uvmesh = uvmesh_map[uint2(i, j)];
                
            float mask = maskOutside * uvmesh_isOutside(uvmesh);
            texel = lerp(texel, float3(1.0, 0.0, 0.0), mask);
            color += texel;
            numSamples += 1.0;
        }
    }
    
    out_buffer[pixelCoord] = float4(color / numSamples, 1);
}

[numthreads(THREAD_GROUP_SIDE, THREAD_GROUP_SIDE, 1)]
void main_imageSpaceBlur(uint3 DTid : SV_DispatchThreadID)
{
    int2 pixelCoord = DTid.xy;

    int matIdx = part_array[0].material;
    Material m = material_array[matIdx];
    int mapId = m.textures[INSPECTED_MAP(_drawMode)];
    if (mapId == -1) {
        return;
    }

    float3 color = float3(0, 0, 0);

    float tex_width, tex_height, tex_elements;
    material_textures.GetDimensions(tex_width, tex_height, tex_elements);

    float radius = (_kernelRadius);
    float numSamples = 0;
    float2 coord = float2(pixelCoord);
    float minX = max(0, coord.x - radius);
    float maxX = min(tex_width, coord.x + radius);
    float minY = max(0, coord.y - radius);
    float maxY = min((tex_height) - 1.0, coord.y + radius);

    float maskOutside = (_drawMode & MASK_OUTSIDE_UV_BIT() ? 1.0 : 0.0);


    for (uint i = 0; i < numKernelSamples; i++) {
        float2 ks_coord = kernelDistribution_PoissonDisk[i] * _kernelRadius;
         //  float2 ks_coord = kernelDistribution_Grid[i] * _kernelRadius;
        ks_coord += coord + ks_coord;

        float weight = 1.0f;
        if (ks_coord.x < minX || ks_coord.x > maxX || ks_coord.y < minY || ks_coord.y > maxY) {
            weight = 0.0f;
        }


        float2 ks_uv = float2(ks_coord.x / tex_width, ks_coord.y / tex_height);
        float3 texel = material_textures.SampleLevel(uSampler0[1], float3(ks_uv , mapId), 0).xyz;
        
        float4 uvmesh = uvmesh_map.SampleLevel(uSampler0[0], ks_uv, 0);

        float mask = maskOutside * uvmesh_isOutside(uvmesh);
        texel = lerp(texel, float3(1.0, 0.0, 0.0), mask);
        color += texel * weight;

        numSamples += weight;
    }

    out_buffer[pixelCoord] = float4(color / numSamples, 1);
}


[numthreads(THREAD_GROUP_SIDE, THREAD_GROUP_SIDE, 1)]
void main_meshSpaceBlur(uint3 DTid : SV_DispatchThreadID)
{
    int2 pixelCoord = DTid.xy;

    Part p = part_array[0];

    int numTriangles = p.numIndices / 3;

    int matIdx = p.material;
    Material m = material_array[matIdx];
    int mapId = m.textures[INSPECTED_MAP(_drawMode)];
    if (mapId == -1) {
        return;
    }
    float tex_width, tex_height, tex_elements;
    material_textures.GetDimensions(tex_width, tex_height, tex_elements);

    float4 k_texel = material_textures[uint3(pixelCoord, mapId)];
    float4 k_uvmesh = uvmesh_map[uint2(pixelCoord)];

    if (uvmesh_isOutside(k_uvmesh)) {
        out_buffer[pixelCoord] = k_texel;
        return;
    }

    float maskOutside = (_drawMode & MASK_OUTSIDE_UV_BIT() ? 1.0 : 0.0);


    // Retreive pos
    uint tidx = uvmesh_triangle(k_uvmesh);

    // Collect ring of triangles
    int tested_triangles[32];
    int numTestedTriangles = mesh_triangle_gatherTriangleRingSorted(tested_triangles, tidx, p.edgeOffset);


    tidx = tested_triangles[0];
    if ((_inspectedTriangle != -1)) {
        bool test = false;
        for (int i = 0; i < numTestedTriangles; ++i) {
            test = test || (tested_triangles[i] == _inspectedTriangle);
        }
        if (!test) {
            out_buffer[pixelCoord] = k_texel;
            return;
        }
    }

    uint3 faceIndices = mesh_fetchFaceIndices(tidx, p.indexOffset);

    FaceVerts faceVerts = mesh_fetchFaceVerts(faceIndices, p.vertexOffset);
    FacePositions facePositions = mesh_unpackFacePositions(faceVerts);
    FaceNormals faceNormals = mesh_unpackFaceNormals(faceVerts);
    FaceTexcoords faceTexcoords = mesh_fetchFaceTexcoords(faceIndices, p.attribOffset);

    float3 position = mesh_interpolateVertexPos(facePositions, k_uvmesh.xyz);
    float3 normal = mesh_interpolateVertexNormal(faceNormals, k_uvmesh.xyz); // y axis
    float2 texcoord = mesh_interpolateVertexTexcoord(faceTexcoords, k_uvmesh.xyz);

    float3 faceEdge = normalize(facePositions[1] - facePositions[0]);
    float3 cotangent = normalize(cross(faceEdge, normal)); // z axis
    float3 tangent = normalize(cross(normal, cotangent)); // x axis

    float3 offsetPosition = position + normal * 0.1;
    float3 ks_rayDir = -normal;

    float3 accumulation = float3(0, 0, 0);

    uint ks_tid = tidx;
    uint3 ks_indices = faceIndices;
    float numIntersects = 0;
    for (uint i = 0; i < numKernelSamples; i++) {
        float2 ks_coord = kernelDistribution_PoissonDisk[i] * _kernelRadius;
     //   float2 ks_coord = kernelDistribution_Grid[i] * _kernelRadius;
        float3 ks_rayOrigin = offsetPosition + tangent * ks_coord.x + cotangent * ks_coord.y;

//#define TEST_SINGLE_TRIANGLE
#define TEST_MANY_TRIANGLES

#ifdef TEST_SINGLE_TRIANGLE
        float3 ks_intersection = triangle_intersect(ks_rayOrigin, ks_rayDir, facePositions[0], facePositions[1], facePositions[2]);
#endif


#ifdef TEST_MANY_TRIANGLES
        // Brut force approach is o(n2) and doesn't finish  before the windows gpu watchdog triggers... bad!
        float3 ks_intersection = float3(100000, 0, 0);
      //  for (int tid = 0; tid < 100 /*numTriangles*/; tid++) {
        for (int tid = 0; tid < numTestedTriangles; tid++) {
            uint3 ti_indices = mesh_fetchFaceIndices(tested_triangles[tid], p.indexOffset);
            FacePositions ti_positions = mesh_fetchFacePositions(ti_indices, p.vertexOffset);
            float3 ti_intersection = triangle_intersect(ks_rayOrigin, ks_rayDir, ti_positions[0], ti_positions[1], ti_positions[2]);
            if ((ti_intersection.x > -1) && (ti_intersection.x < ks_intersection.x)) {
                ks_intersection = ti_intersection;
                ks_indices = ti_indices;
                ks_tid = tested_triangles[tid];
            }
        }
        ks_intersection.x = (ks_intersection.x == 100000 ? -1 : ks_intersection.x);
#endif


        numIntersects += float(ks_intersection.x > -1);
    
        float3 ks_texel = float3(maskOutside, 0.0, 0.0);
        if ((ks_intersection.x > -1)) {
            float3x2 ks_texcoords = mesh_fetchFaceTexcoords(ks_indices, p.attribOffset);

            // A triangle and bary pos found, from this let's go fetch the signal value and accumulate
            float3 ks_barypos = float3(1.0 - ks_intersection.y - ks_intersection.z, ks_intersection.yz);
            float2 ks_uv = mesh_interpolateVertexTexcoord(ks_texcoords, ks_barypos);

           // ks_texel = material_textures[uint3(ks_uv.x * tex_width, ks_uv.y * tex_height, mapId)].xyz;
            ks_texel = material_textures.SampleLevel(uSampler0[1], float3(ks_uv.xy, mapId), 0).xyz;
            
        }
        accumulation += ks_texel;
    }


    float3 color = float3(1, 1, 1);
   // color = rainbowRGB(float(triangleId), float(part_array[0].numIndices / 3));
   // color = normal.xyz;
   // color = position;
   // color = tangent;
   // color = cotangent;
   // color = float3(texcoord.xy, 0 );
    //color = rainbowRGB(numIntersects, float(numKernelSamples));
    color = accumulation / float(numIntersects);

    out_buffer[pixelCoord] = float4(color, 1);
}

