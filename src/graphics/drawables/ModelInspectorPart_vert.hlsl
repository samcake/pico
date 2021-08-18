#define mat43 float4x3

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
// Transform API
//
struct Transform {
    float4 _right_upX;
    float4 _upYZ_backXY;
    float4 _backZ_ori;

    float3 row_x() { return float3(_right_upX.x, _right_upX.w, _upYZ_backXY.z); }
    float3 row_y() { return float3(_right_upX.y, _upYZ_backXY.x, _upYZ_backXY.w); }
    float3 row_z() { return float3(_right_upX.z, _upYZ_backXY.y, _backZ_ori.x); }

    float3 col_x() { return _right_upX.xyz; }
    float3 col_y() { return float3(_right_upX.w, _upYZ_backXY.xy); }
    float3 col_z() { return float3(_upYZ_backXY.zw, _backZ_ori.x); }
    float3 col_w() { return _backZ_ori.yzw; }
};

float3 rotateFrom(const Transform mat, const float3 d) {
    return float3(dot(mat.row_x(), d), dot(mat.row_y(), d), dot(mat.row_z(), d));
}
float3 rotateTo(const Transform mat, const float3 d) {
    return float3(dot(mat.col_x(), d), dot(mat.col_y(), d), dot(mat.col_z(), d));
}

float3 transformTo(const Transform mat, const float3 p) {
    return rotateTo(mat, p - mat.col_w());
}
float3 transformFrom(const Transform mat, const float3 p) {
    return rotateFrom(mat, p) + mat.col_w();
}

//
// Projection API
// 
struct Projection {
    float4 _aspectRatio_sensorHeight_focal_far;
    float4 _ortho_enabled_height_near_far;

    float aspectRatio() { return (_aspectRatio_sensorHeight_focal_far.x); }
    float sensorHeight() { return (_aspectRatio_sensorHeight_focal_far.y); }
    float focal() { return (_aspectRatio_sensorHeight_focal_far.z); }
    float persFar() { return (_aspectRatio_sensorHeight_focal_far.w); }

    bool isOrtho() { return (_ortho_enabled_height_near_far.x > 0.0); }
    float orthoHeight() { return (_ortho_enabled_height_near_far.y); }
    float orthoNear() { return (_ortho_enabled_height_near_far.z); }
    float orthoFar() { return (_ortho_enabled_height_near_far.w); }
};


float4 clipFromEyeSpace(float aspectRatio, float sensorHeight, float focal, float pfar, float3 eyePos) {
    float ez = -eyePos.z;
    float pnear = focal;
    // Infinite far inverted Z
    // float b = 0.0f; //lim at far  infinite of  pnear / (pnear- pfar);;
    // float a = pnear; // lim at far infinite of - pfar * pnear / (pnear - pfar);
    float4 clipPos;
    clipPos.w = ez;
    clipPos.z = pnear;
    // float depthBuffer = z/w = a * (1/ez) + b; 
    clipPos.x = eyePos.x * pnear * 2.0 / (sensorHeight * aspectRatio);
    clipPos.y = eyePos.y * pnear * 2.0 / (sensorHeight);
    return clipPos;
}

float4 orthoClipFromEyeSpace(float aspectRatio, float sensorHeight, float pnear, float pfar, const float3 eyePos) {
    float4 clipPos;
    clipPos.w = 1.0f;
    clipPos.z = (pfar - (-eyePos.z)) / (pfar - pnear);
    clipPos.x = eyePos.x * 2.0f / (sensorHeight * aspectRatio);
    clipPos.y = eyePos.y * 2.0f / (sensorHeight);
    return clipPos;
}

float4 clipFromEyeSpace(Projection proj, float3 eyePos) {
    if (proj.isOrtho()) {
        return orthoClipFromEyeSpace(proj.aspectRatio(), proj.orthoHeight(), proj.orthoNear(), proj.orthoFar(), eyePos);
    }
    else {
        return clipFromEyeSpace(proj.aspectRatio(), proj.sensorHeight(), proj.focal(), proj.persFar(), eyePos);
    }
}

// Camera buffer
cbuffer UniformBlock0 : register(b0) {
    //float4x3 _view;
    Transform _view;
    Projection _projection;
    float4 _viewport;
};


float3 eyeFromWorldSpace(Transform view, float3 worldPos) {
    return transformTo(view, worldPos);
}

float3 eyeFromWorldSpaceDir(Transform view, float3 worldDir) {
    return rotateTo(view, worldDir);
}

float3 worldFromObjectSpace(Transform model, float3 objPos) {
    return transformFrom(model, objPos);
}

float3 worldFromObjectSpaceDir(Transform model, float3 objDir) {
    return rotateFrom(model, objDir);
}

float3 objectFromWorldSpaceDir(Transform model, float3 worldDir) {
    return rotateTo(model, worldDir);
}


//
// Transform Tree
//
StructuredBuffer<Transform>  tree_transforms : register(t0);

Transform node_getTransform(int nodeID) {
    return tree_transforms[2 * nodeID];
}
Transform node_getWorldTransform(int nodeID) {
    return tree_transforms[2 * nodeID + 1];
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
            }
            else {
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



