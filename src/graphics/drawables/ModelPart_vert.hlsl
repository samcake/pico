#include "SceneTransform_inc.hlsl"

//
// Model
//
struct Part {
    uint numIndices;
    uint indexOffset;
    uint vertexOffset;
    uint attribOffset;
    uint material;
    uint spareA;
    uint spareB;
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

FacePositions mesh_unpackFacePositions(FaceVerts fv) {
    return fv.v;
}

typedef float3x3 FaceNormals;

FaceNormals mesh_unpackFaceNormals(FaceVerts fv) {
    FaceNormals fns;
    fns[0] = unpackNormalFrom32I(fv.n[0]);
    fns[1] = unpackNormalFrom32I(fv.n[1]);
    fns[2] = unpackNormalFrom32I(fv.n[2]);
    return fns;
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

// Triangle Attribute Intepolation
float3 mesh_interpolateVertexPos(FaceVerts fv, float3 baryPos) {
    return baryPos.x * fv.v[0].xyz + baryPos.y * fv.v[1].xyz + baryPos.z * fv.v[2].xyz;
}
float3 mesh_interpolateVertexPos(FacePositions fp, float3 baryPos) {
    return baryPos.x * fp[0].xyz + baryPos.y * fp[1].xyz + baryPos.z * fp[2].xyz;
}
float3 mesh_interpolateVertexNormal(FaceNormals fns, float3 baryPos) {
    return normalize(baryPos.x * fns[0] + baryPos.y * fns[1] + baryPos.z * fns[2]);
}
float2 mesh_interpolateVertexTexcoord(FaceTexcoords ft, float3 baryPos) {
    return baryPos.x * ft[0].xy + baryPos.y * ft[1].xy + baryPos.z * ft[2].xy;
}





cbuffer UniformBlock1 : register(b1) {
    int _nodeID;
    int _partID;
    int _numNodes;
    int _numParts;
    int _numMaterials;
}

struct VertexShaderOutput
{
    float3 EyePos : EPOS;
    float3 Normal   : NORMAL;
    float2 Texcoord  : TEXCOORD;
    float4 Position : SV_Position;
};

VertexShaderOutput main(uint vidx : SV_VertexID) {

    // Fetch the part data
    Part p = part_array[_partID];

    uint tidx = vidx / 3;
    uint tvidx = vidx % 3;

    uint3 faceIndices = mesh_fetchFaceIndices(tidx, p.indexOffset);
    FaceVerts faceVerts = mesh_fetchFaceVerts(faceIndices, p.vertexOffset);

    float2 texcoord = mesh_fetchVertexTexcoord(faceIndices[tvidx], p.attribOffset);
    

    // Generate normal
    float3 faceEdge0 = faceVerts.v[1].xyz - faceVerts.v[0].xyz;
    float3 faceEdge1 = faceVerts.v[2].xyz - faceVerts.v[0].xyz;
    float3 normal = normalize(cross(faceEdge0, faceEdge1));

    normal = unpackNormalFrom32I(faceVerts.n[tvidx]);
  //  normal = float3(0.0, 1.0, 0.0);

    // Barycenter 
    float3 barycenter = (faceVerts.v[0].xyz + faceVerts.v[1].xyz + faceVerts.v[2].xyz) / 3.0f;

    // Transform
    float3 position = faceVerts.v[tvidx].xyz;
    position += 0.0f * (barycenter - position);

    Transform _model = node_getWorldTransform(_nodeID);

    position = worldFromObjectSpace(_model, position);
    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);

    normal = worldFromObjectSpaceDir(_model, normal);

  /*  uint color = asuint(p.material);
    const float INT8_TO_NF = 1.0 / 255.0;
    float r = INT8_TO_NF * (float)((color >> 0) & 0xFF);
    float g = INT8_TO_NF * (float)((color >> 8) & 0xFF);
    float b = INT8_TO_NF * (float)((color >> 16) & 0xFF);
*/

  //  float3 camera_pos = worldFromEyeSpace(_view, float3(0, 0, 0));
    float3 camera_pos = _view.col_w();

    VertexShaderOutput OUT;
    OUT.Position = clipPos;
  //  OUT.EyePos = eyePosition;
    OUT.EyePos = camera_pos - position;
    OUT.Normal = normal;
    OUT.Texcoord = texcoord;
    
    return OUT;
}

