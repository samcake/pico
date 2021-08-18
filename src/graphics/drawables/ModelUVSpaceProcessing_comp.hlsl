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



//
// Color API
// 

float3 colorRGBfromHSV(const float3 hsv) {
    const float4 k = float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
    float3 f = frac(float3(hsv.x + k.x, hsv.x + k.y, hsv.x + k.z)) * 6.0f;
    float3 p = abs(float3(f.x - k.w, f.y - k.w, f.z - k.w));
    p = clamp(float3(p.x - k.x, p.y - k.x, p.z - k.z), float3(0, 0, 0), float3(1.0, 1.0, 1.0));
    return lerp(k.xxx, p, hsv.yyy) * hsv.z;
}

float3 rainbowRGB(float n, float d = 1.0f) {
    return colorRGBfromHSV(float3((n / d) * (5.0 / 6.0), 3.0, 1.0));
}

//
// Model
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


//
// Mesh API
//
Buffer<uint>  index_array : register(t2);

typedef float4 Vertex;
StructuredBuffer<Vertex>  vertex_array : register(t3);

typedef float4 VertexAttrib;
StructuredBuffer<VertexAttrib>  vertex_attrib_array : register(t4);

uint mesh_fetchVectorIndex(int vertNum, int indexOffset) {
    return indexOffset + vertNum;
}

uint3 mesh_fetchFaceIndices(int faceNum, int indexOffset) {
    // Fetch Face indices
    int faceIndexBase = indexOffset + faceNum * 3;
    return uint3(index_array[faceIndexBase], index_array[faceIndexBase + 1], index_array[faceIndexBase + 2]);
}

float3 unpackNormalFrom32I(uint n) {
    int nx = int(n & 0x3FF);
    int ny = int((n >> 10) & 0x3FF);
    int nz = int((n >> 20) & 0x3FF);
    //   nx = nx + (nx > 511 ? -1024 : 0);
   //    ny = ny + (ny > 511 ? -1024 : 0); 
     //  nz = nz + (nz > 511 ? -1024 : 0); 
    nx = nx - 511;
    ny = ny - 511;
    nz = nz - 511;

    return normalize(float3(float(nx), float(ny), float(nz)));
}

float3 mesh_fetchVertexPosition(uint vid, int vertexOffset) {
    return vertex_array[vertexOffset + vid].xyz;
}

float3 mesh_fetchVertexNormal(uint vid, int vertexOffset) {
    return unpackNormalFrom32I(asuint(vertex_array[vertexOffset + vid].w));
}

float2 mesh_fetchVertexTexcoord(uint aid, int attribOffset) {
    return (attribOffset == -1 ? float2(0.5, 0.5) : vertex_attrib_array[attribOffset + aid].xy);
}

typedef float3x3 FacePositions;

struct FaceVerts {
    FacePositions v;
    uint3 n;
};

FaceVerts mesh_fetchFaceVerts(uint3 fvid, int vertexOffset) {
    // Fetch Face vertices
    FaceVerts face;
    for (int i = 0; i < 3; i++) {
        uint vi = vertexOffset + fvid[i];
        Vertex v = vertex_array[vi];
        face.v[i] = v.xyz;
        face.n[i] = asuint(v.w);
    }
    return face;
}

FacePositions mesh_fetchFacePositions(uint3 fvid, int vertexOffset) {
    // Fetch Face positions
    FacePositions face;
    for (int i = 0; i < 3; i++) {
        face[i] = vertex_array[vertexOffset + fvid[i]].xyz;
    }
    return face;
}
FacePositions mesh_unpackFacePositions(FaceVerts fv) {
    return fv.v;
}
float3 mesh_interpolateVertexPos(FaceVerts fv, float3 baryPos) {
    return baryPos.x * fv.v[0].xyz + baryPos.y * fv.v[1].xyz + baryPos.z * fv.v[2].xyz;
}
float3 mesh_interpolateVertexPos(FacePositions fp, float3 baryPos) {
    return baryPos.x * fp[0].xyz + baryPos.y * fp[1].xyz + baryPos.z * fp[2].xyz;
}

typedef float3x3 FaceNormals;

FaceNormals mesh_unpackFaceNormals(FaceVerts fv) {
    FaceNormals fns;
    fns[0] = unpackNormalFrom32I(fv.n[0]);
    fns[1] = unpackNormalFrom32I(fv.n[1]);
    fns[2] = unpackNormalFrom32I(fv.n[2]);
    return fns;
}

float3 mesh_interpolateVertexNormal(FaceNormals fns, float3 baryPos) {
    return normalize(baryPos.x * fns[0] + baryPos.y * fns[1] + baryPos.z * fns[2]);
}

typedef float3x4 FaceAttribs;
typedef float3x2 FaceTexcoords;

FaceAttribs mesh_fetchFaceAttribs(uint3 fvid, int attribOffset) {
    // Fetch Face attribs
    FaceAttribs face;
    for (int i = 0; i < 3; i++) {
        face[i] = vertex_attrib_array[attribOffset + fvid[i]].xyzw;
    }
    return face;
}
FaceTexcoords mesh_fetchFaceTexcoords(uint3 fvid, int attribOffset) {
    // Fetch Face attribs
    FaceTexcoords face;
    for (int i = 0; i < 3; i++) {
        face[i] = vertex_attrib_array[attribOffset + fvid[i]].xy;
    }
    return face;
}

float2 mesh_interpolateVertexTexcoord(FaceTexcoords ft, float3 baryPos) {
    return baryPos.x * ft[0].xy + baryPos.y * ft[1].xy + baryPos.z * ft[2].xy;
}

typedef int4 Edge; // x and y are vertex indices, z and w are triangle indices in index buffer
StructuredBuffer<Edge>  edge_array : register(t5);

bool mesh_edge_isAttribSeam(int4 edge) {
    return edge.w < 0;
}
int mesh_edge_triangle0(int4 edge) {
    return edge.z;
}
int mesh_edge_triangle1(int4 edge) {
    return (edge.w < 0 ? -edge.w - 1 : edge.w);
}

bool mesh_edge_isOutline(int4 edge) { // no c1 continuity neighbor face in the mesh
    return edge.z == mesh_edge_triangle1(edge);
}

typedef int4 FaceEdge; // x, y and z are the 3 edge indices of the face, allowing to retreive neighbor faces
StructuredBuffer<FaceEdge>  face_array : register(t6);


uint3 mesh_triangle_getAdjacentTriangles(uint triangleId, int edgeOffset) {
    int3 faceEdges = face_array[triangleId].xyz;

    uint3 triangles;
    for (int e = 0; e < 3; ++e) {
        Edge edge = edge_array[faceEdges[e] + edgeOffset];
        triangles[e] = ((edge.z == triangleId) ? mesh_edge_triangle1(edge) : mesh_edge_triangle0(edge));
    }
    return triangles;
}

int2 mesh_triangle_nextTriangleAroundFromAdjacent(int dir, uint pivotTriangle, uint fromTriangle, int edgeOffset) {

    uint3 pivotTriAdj = mesh_triangle_getAdjacentTriangles(pivotTriangle, edgeOffset);

    int adjEdgeId = (pivotTriAdj.x == fromTriangle ? 0 :
        (pivotTriAdj.y == fromTriangle ? 1 : 2));

    int nextEdgeId = (adjEdgeId + dir) % 3;

    int nextTri = pivotTriAdj[nextEdgeId];

    return int2(nextTri, nextEdgeId);
}

int mesh_triangle_detectAdjTriangle(uint nextRingTri, uint3 adjTriangles) {
    int detectAdjTriangle = -1 + 1 * (adjTriangles.x == nextRingTri) + 2 * (adjTriangles.y == nextRingTri) + 3 * (adjTriangles.z == nextRingTri);
    return detectAdjTriangle;
}

int mesh_triangle_gatherTriangleRing(out int ring[32], uint pivotTriangle, int edgeOffset) {
    const int MAX_RING_LENGTH = 32;
    uint3 adjFaces = mesh_triangle_getAdjacentTriangles(pivotTriangle, edgeOffset);

    int prevTri = pivotTriangle;
    int currentTri = adjFaces[0]; // find the other "ring" triangles from the last one
    ring[0] = currentTri;
    int ringLength = 1;

    int direction = +1;
    for (int rt = 1; rt < MAX_RING_LENGTH; ) {
        int2 next = mesh_triangle_nextTriangleAroundFromAdjacent(direction, currentTri, prevTri, edgeOffset);

        int detectAdjTriangle = mesh_triangle_detectAdjTriangle(next.x, adjFaces);
        prevTri = currentTri;
        currentTri = next.x;

        if (detectAdjTriangle == 0) {
            rt = MAX_RING_LENGTH;
        }
        else {
            // found the next triangle on the ring, not an adjTri
            ring[ringLength] = currentTri;
            ringLength++;
            rt++;

            // reached another adj triangle;
            if (detectAdjTriangle > 0) {
                // reset the search from the center triangle
                prevTri = pivotTriangle;
            }
        }
    }

    return ringLength;
}


int mesh_triangle_gatherTriangleRingSorted(out int ring[32], uint pivotTriangle, int edgeOffset) {
    const int MAX_RING_LENGTH = 32;
    uint3 adjFaces = mesh_triangle_getAdjacentTriangles(pivotTriangle, edgeOffset);
    ring[0] = pivotTriangle;
    ring[1] = adjFaces.x;
    ring[2] = adjFaces.y;
    ring[3] = adjFaces.z;
    int ringLength = 4;

    int prevTri = pivotTriangle;
    int currentTri = adjFaces[0]; // find the other "ring" triangles from the last one

    int direction = +1;
    for (int rt = 4; rt < MAX_RING_LENGTH; ) {
        int2 next = mesh_triangle_nextTriangleAroundFromAdjacent(direction, currentTri, prevTri, edgeOffset);

        int detectAdjTriangle = mesh_triangle_detectAdjTriangle(next.x, adjFaces);
        prevTri = currentTri;
        currentTri = next.x;

        if (detectAdjTriangle == 0) {
            rt = MAX_RING_LENGTH;
        }
        else {
            // reached another adj triangle;
            if (detectAdjTriangle > 0) {
                // reset the search from the center triangle
                prevTri = pivotTriangle;
            } else {
                // found the next triangle on the ring, not an adjTri
                ring[ringLength] = currentTri;
                ringLength++;
                rt++;
            }
        }
    }

    return ringLength;
}

//
// Triangle API
// 
// Triangle ray intersection by IQ https://iquilezles.org/
// triangle defined by vertices v0, v1 and  v2
float3 triangle_intersect(in float3 ro, in float3 rd, in float3 v0, in float3 v1, in float3 v2) {
    float3 v1v0 = v1 - v0;
    float3 v2v0 = v2 - v0;
    float3 rov0 = ro - v0;
    float3  n = cross(v1v0, v2v0);
    float3  q = cross(rov0, rd);
    float d = 1.0 / dot(rd, n);
    float u = d * dot(-q, v2v0);
    float v = d * dot(q, v1v0);
    float t = d * dot(-n, rov0);
    if (u < 0.0 || u>1.0 || v < 0.0 || (u + v)>1.0) t = -1.0;
    return float3(t, u, v);
}


//
// Material API
// 
struct Material {
    float4 color;
    float metallic;
    float roughness;
    float spareA;
    float spareB;
    float4 emissive;
    uint4 textures;
};

StructuredBuffer<Material>  material_array : register(t9);
Texture2DArray material_textures : register(t10);


SamplerState uSampler0[2] : register(s0);

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

// Read previous state from 
RWTexture2D<float4> out_buffer : register(u0);

[numthreads(32, 32, 1)]
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

[numthreads(32, 32, 1)]
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


[numthreads(32, 32, 1)]
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

