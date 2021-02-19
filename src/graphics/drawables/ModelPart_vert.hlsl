#define mat43 float4x3

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

Buffer<uint>  index_array : register(t2);

struct Vertex {
    float x;
    float y;
    float z;
    uint n;
};

StructuredBuffer<Vertex>  vertex_array : register(t3);

struct VertexAttrib {
    float x;
    float y;
    float z;
    float w;
};

StructuredBuffer<VertexAttrib>  vertex_attrib_array : register(t4);

uint3 fetchFaceIndices(int indexOffset, int faceNum) {
    // Fetch Face indices
    int faceIndexBase = indexOffset + faceNum * 3;
    return uint3(index_array[faceIndexBase], index_array[faceIndexBase + 1], index_array[faceIndexBase + 2]);
}

struct Face {
    float3 v[3];
    uint3 n;
};

Face fetchFaceVerts(uint3 fvid, int vertexOffset) {
    // Fetch Face vertices
    Face face;
    for (int i = 0; i < 3; i++) {
        uint vi = vertexOffset + fvid[i];
        Vertex v = vertex_array[vi];
        face.v[i] = float3(v.x, v.y, v.z);
        face.n[i] = v.n;
    }

    return face;
}

Face fetchFaceAttribs(uint3 fvid, int attribOffset) {
    // Fetch Face vertices
    Face face;
    for (int i = 0; i < 3; i++) {
        uint vi = attribOffset + fvid[i];
        face.v[i] = float4(vertex_attrib_array[vi].x, vertex_attrib_array[vi].y, vertex_attrib_array[vi].z, vertex_attrib_array[vi].w);
    }

    return face;
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

    uint3 faceIndices = fetchFaceIndices(p.indexOffset, tidx);
    Face faceVerts = fetchFaceVerts(faceIndices, p.vertexOffset);

    float2 texcoord = float2(0.5, 0.5);
    if (p.attribOffset != uint( -1)) {
        VertexAttrib va = vertex_attrib_array[faceIndices[tvidx] + p.attribOffset];
        texcoord = float2(va.x, va.y);
    }

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
    VertexShaderOutput OUT;
    OUT.Position = clipPos;
    OUT.EyePos = eyePosition;
    OUT.Normal = normal;
    OUT.Texcoord = texcoord;
    
    return OUT;
}

