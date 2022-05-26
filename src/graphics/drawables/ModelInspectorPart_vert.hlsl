#include "SceneTransform_inc.hlsl"
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

int DISPLAYED_COLOR_BITS() { return 0x00F00000; }
int DISPLAYED_COLOR_OFFSET() { return 20; }
int DISPLAYED_COLOR(int drawMode) { return (drawMode & DISPLAYED_COLOR_BITS()) >> DISPLAYED_COLOR_OFFSET(); }


// Drawcall data
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




float4 uvSpaceClipPos(float2 uv) {
    float2 eyeUV = uv - float2(_uvCenterX, _uvCenterY);
    return orthoClipFromEyeSpace(_projection.aspectRatio(),  2.0f * _uvScale
    , 0.0f, 2.0f, float3( eyeUV, 1.0f));
}

struct VertexShaderOutput {
    float3 EyePos : EPOS;
    float3 Normal   : NORMAL;
    float2 Texcoord  : TEXCOORD;
    float4 TriPos : TBPOS;
    float4 Position : SV_Position;
};

VertexShaderOutput main_uvspace(uint vidx : SV_VertexID) {
    uint tidx = vidx / 2;
    uint tvidx = vidx % 2;

    float2 texcoord = float2(float(tidx), float(tvidx));

    VertexShaderOutput OUT;

    OUT.Position = uvSpaceClipPos(texcoord);
    OUT.EyePos = OUT.Position;

    OUT.Normal = float3(0.0, 0.0, 1.0);
    OUT.Texcoord = texcoord;
    OUT.TriPos = float4(0, 0, 0, 0);

    return OUT;
}

VertexShaderOutput transformAndPackOut(float3 position, float3 normal, float2 texcoord, float4 trianglePos) {
    VertexShaderOutput OUT;
    // Transform
    Transform _model = node_getWorldTransform(_nodeID);
    position = worldFromObjectSpace(_model, position);
    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);

    normal = worldFromObjectSpaceDir(_model, normal);
    normal = eyeFromWorldSpaceDir(_view, normal);

    OUT.Position = clipPos;
    OUT.EyePos = eyePosition;
    OUT.Normal = normal;
    OUT.Texcoord = texcoord;
    OUT.TriPos = trianglePos;

    if (_drawMode & RENDER_UV_SPACE_BIT()) {
        OUT.Position = uvSpaceClipPos(texcoord);
    }
    else if (_drawMode & MAKE_UVMESH_MAP_BIT()) {
        float2 uv = 2 * texcoord - float2(1.0, 1.0);
        OUT.Position = orthoClipFromEyeSpace(1.0f, 2.0f, 0.0f, 2.0f,
            float3(uv, 1.0f));
        OUT.Position.y = -OUT.Position.y;
    }
    return OUT;
}


VertexShaderOutput transformAndPackOutSprite(int sid, float spriteSize, float3 position, float3 normal, float2 texcoord, float4 trianglePos) {
    VertexShaderOutput OUT;
    // Transform
    Transform _model = node_getWorldTransform(_nodeID);
    position = worldFromObjectSpace(_model, position);
    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);
    float eyeLinearDepth = -eyePosition.z;

    normal = worldFromObjectSpaceDir(_model, normal);
    normal = eyeFromWorldSpaceDir(_view, normal);

    OUT.Position = clipPos;
    OUT.EyePos = eyePosition;
    OUT.Normal = normal;
    OUT.Texcoord = texcoord;
    OUT.TriPos = trianglePos;

    // Apply scaling to the sprite coord and offset pointcloud pos
    float2 sprite = float2(((sid == 1) ? 3.0 : -1.0), ((sid == 2) ? 3.0 : -1.0));
    const float2 invRes = float2(1.0 / _viewport.z, 1.0 / _viewport.w);
    float2 spriteOffset = invRes.xy * spriteSize * sprite;
    OUT.TriPos.yz = sprite;
    // fixed sprite pixel size in depth or perspective correct 3d size ?
    OUT.Position.xy += spriteOffset * eyeLinearDepth;

    if (_drawMode & RENDER_UV_SPACE_BIT()) {
        OUT.Position = uvSpaceClipPos(texcoord);

        OUT.Position.xy += spriteOffset;
    }

    return OUT;
}

VertexShaderOutput transformAndPackOutSplat(int sid, float splatSize, float3 tangent, float3 cotangent, float3 position, float3 normal, float2 texcoord, float4 trianglePos) {
    VertexShaderOutput OUT;
    float2 sprite = float2(((sid == 1) ? 3.0 : -1.0), ((sid == 2) ? 3.0 : -1.0));

    if (!(_drawMode & RENDER_UV_SPACE_BIT())) {
        position = position + tangent * sprite.x * splatSize - cotangent * sprite.y * splatSize;
    }


    // Transform
    Transform _model = node_getWorldTransform(_nodeID);
    position = worldFromObjectSpace(_model, position);
    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);
    float eyeLinearDepth = -eyePosition.z;

    normal = worldFromObjectSpaceDir(_model, normal);
    normal = eyeFromWorldSpaceDir(_view, normal);

    OUT.Position = clipPos;
    OUT.EyePos = eyePosition;
    OUT.Normal = normal;
    OUT.Texcoord = texcoord;
    OUT.TriPos = trianglePos;
    OUT.TriPos.yz = sprite;


    if (_drawMode & RENDER_UV_SPACE_BIT()) {
        OUT.Position = uvSpaceClipPos(texcoord);

        // Apply scaling to the sprite coord and offset pointcloud pos
        const float2 invRes = float2(1.0 / _viewport.z, 1.0 / _viewport.w);
        float2 spriteOffset = invRes.xy * splatSize * sprite;
        OUT.Position.xy += spriteOffset;
    }

    return OUT;
}


VertexShaderOutput main(uint vidx : SV_VertexID) {
    VertexShaderOutput OUT;

    // Which part ?
    Part p = part_array[_partID];

    // Fetch the part data
    uint pvidx = vidx;
    uint tidx = pvidx / 3;
    uint tvidx = pvidx % 3;

    // draw edges ?
    if (_drawMode & DRAW_EDGE_LINE_BIT()) {
        uint eidx = vidx / 2;
        int evidx = vidx % 2;
        Edge edge = edge_array[eidx];
        pvidx = edge[evidx]; // x or y are vertex indices of the edge
        tidx = mesh_edge_triangle0(edge);
        if (!(_drawMode & RENDER_WIREFRAME_BIT()) && !mesh_edge_isAttribSeam(edge)) {
            return OUT;
        }
    }


    // Fetch mesh and face data
    uint3 faceIndices = mesh_fetchFaceIndices(tidx, p.indexOffset);
    FaceVerts faceVerts = mesh_fetchFaceVerts(faceIndices, p.vertexOffset);
    FacePositions facePositions = mesh_unpackFacePositions(faceVerts);


    if (_drawMode & DRAW_EDGE_LINE_BIT()) {
        tvidx = ((pvidx == faceIndices[0]) ? 0 : (
                    (pvidx == faceIndices[1]) ? 1 :
                        2 ));
    }

    float3 position = facePositions[tvidx];
    float3 normal = unpackNormalFrom32I(faceVerts.n[tvidx]);
    float2 texcoord = mesh_fetchVertexTexcoord(faceIndices[tvidx], p.attribOffset);

    // Barycenter tweak 
    float3 barycenter = (facePositions[0] + facePositions[1] + facePositions[2]) / 3.0f;
    position += 0.0f * (barycenter - position);

    float4 trianglePos = float4(tvidx == 0, tvidx == 1, tvidx == 2, float(tidx + 1));
    if (_drawMode & DRAW_EDGE_LINE_BIT()) {
        trianglePos.w = -trianglePos.w;
    }

    return transformAndPackOut(position, normal, texcoord, trianglePos);
}


VertexShaderOutput main_connectivity(uint vid : SV_VertexID) {
    VertexShaderOutput OUT;
    // Which part ?
    Part p = part_array[_partID];

    // Fetch the part data
    uint pvidx = vid;

    uint tidx = pvidx / 3;
    uint tvidx = pvidx % 3;
    
    uint neighbor_tidx = tidx % (_numInspectedTriangles);
    tidx = tidx / (_numInspectedTriangles);

    if (_inspectedTriangle == -1) {
        return OUT;
    }

    tidx = _inspectedTriangle;
    uint triangleIdx = 0;


    int triangleRing[32];

    int ringLength = mesh_triangle_gatherTriangleRing(triangleRing, tidx, p.edgeOffset);
    if (neighbor_tidx > 0 && neighbor_tidx <= ringLength) {
        triangleIdx = triangleRing[neighbor_tidx - 1];
    }
    else if (neighbor_tidx == 0) {
        triangleIdx = tidx;
    }


    // Fetch the actual face vertex indices
    uint3 faceIndices = mesh_fetchFaceIndices(triangleIdx, p.indexOffset);
    int vidx = faceIndices[tvidx];
   // float4 trianglePos = float4(tvidx == 0, tvidx == 1, tvidx == 2, float(triangleIdx + 1));
    float4 trianglePos = float4(float(ringLength + 1), tvidx == 1, tvidx == 2, float(neighbor_tidx));
    float3 position = mesh_fetchVertexPosition(vidx, p.vertexOffset);
    float3 normal = mesh_fetchVertexNormal(vidx, p.vertexOffset);
    float2 texcoord = mesh_fetchVertexTexcoord(vidx, p.attribOffset);

    return transformAndPackOut(position, normal, texcoord, trianglePos);
}


static const uint numKernelSamples = 16;
static float2 kernelDistribution_PoissonDisk[numKernelSamples] =
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
static float2 kernelDistribution_Grid[numKernelSamples] =
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

VertexShaderOutput main_kernelSamples(uint vid : SV_VertexID) {
    VertexShaderOutput OUT;
    // Which part ?
    Part p = part_array[_partID];

    // Which inspected texel
    uint tex_width, tex_height;
    uvmesh_map.GetDimensions(tex_width, tex_height);
    int2 texPos = int2(_inspectedTexelX, _inspectedTexelY);
    float2 texUV = float2((texPos.x + 0.5f) / float(tex_width), (texPos.y + 0.5f) / float(tex_height));
    // fetch the UVmesh
    float4 k_uvmesh = uvmesh_map.Load(int3(texPos, 0));

    uint tidx = uvmesh_triangle(k_uvmesh);
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

    float4 trianglePos = float4(numKernelSamples, 0, 0, 0);


    int pid = vid / 3;
    int sid = pid - 1;
    int sprite_vid = vid % 3;

    if (pid == 0) {
        trianglePos.x = 0;      
        return transformAndPackOutSplat(sprite_vid, _kernelRadius, tangent, cotangent, position, normal, texcoord, trianglePos);
    }

    // Collect ring of triangles
    int tested_triangles[32];
    int numTestedTriangles = mesh_triangle_gatherTriangleRingSorted(tested_triangles, tidx, p.edgeOffset);


    uint ks_tid = tidx;
    uint3 ks_indices = faceIndices;
    float3x3  ks_positions = facePositions;
    float numIntersects = 0;


    uint i = sid;

    {
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
        //  for (int tid = 0; tid < 100; tid++) {
        for (int tid = 0; tid < numTestedTriangles; tid++) {
            uint3 ti_indices = mesh_fetchFaceIndices(tested_triangles[tid], p.indexOffset);
            FacePositions ti_positions = mesh_fetchFacePositions(ti_indices, p.vertexOffset);
            float3 ti_intersection = triangle_intersect(ks_rayOrigin, ks_rayDir, ti_positions[0], ti_positions[1], ti_positions[2]);
            if ((ti_intersection.x > -1) && (ti_intersection.x < ks_intersection.x)) {
                ks_intersection = ti_intersection;
                ks_indices = ti_indices;
                ks_positions = ti_positions;
                ks_tid = tested_triangles[tid];
            }
        }
        ks_intersection.x = (ks_intersection.x == 100000 ? -1 : ks_intersection.x);
#endif


       float2 ks_uv = texcoord;
        if ((ks_intersection.x > -1)) {
            float3x2 ks_texcoords = mesh_fetchFaceTexcoords(ks_indices, p.attribOffset);

            // A triangle and bary pos found, from this let's go fetch the signal value and accumulate
            float3 ks_barypos = float3(1.0 - ks_intersection.y - ks_intersection.z, ks_intersection.yz);
            texcoord = mesh_interpolateVertexTexcoord(ks_texcoords, ks_barypos);
            position = ks_rayOrigin;
            position = mesh_interpolateVertexPos(ks_positions, ks_barypos);
        }

    }

    trianglePos = float4(float(numKernelSamples), 0, 0, float(sid));

    return transformAndPackOutSprite(sprite_vid, 5.0, position, normal, texcoord, trianglePos);
}

VertexShaderOutput main_uvmesh_point(uint vidx : SV_VertexID) {
    uint tex_width, tex_height;
    uvmesh_map.GetDimensions(tex_width, tex_height);
    int2 texPos = int2(vidx % tex_width, vidx / tex_height);

    float2 texUV = float2((texPos.x + 0.5f) / float(tex_width), (texPos.y + 0.5f) / float(tex_height));

    // fetch the UVmesh
    float4 k_uvmesh = uvmesh_map.Load(int3(texPos, 0));

    // Find part / triangle idx
    // Fetch the part data
    Part p;
    uint pvidx = 0;
    // Valid part idx means per part drawcall
    if (_partID != uint(-1)) {
        p = part_array[_partID];
        pvidx = vidx;
    }

    uint tidx = uvmesh_triangle(k_uvmesh);
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

    return transformAndPackOut(position, float3(0.0, 0.0, 1.0), texcoord, k_uvmesh);
}



