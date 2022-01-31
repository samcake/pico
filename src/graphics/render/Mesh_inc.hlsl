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


