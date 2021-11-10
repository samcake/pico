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
cbuffer UniformBlock0 : register(b10) {
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

float3 objectFromWorldSpaceDir(Transform model, float3 worldDir) {
    return rotateTo(model, worldDir);
}


//
// Transform Tree
//
StructuredBuffer<Transform>  tree_transforms : register(t20);

Transform node_getTransform(int nodeID) {
    return tree_transforms[2 * nodeID];
}
Transform node_getWorldTransform(int nodeID) {
    return tree_transforms[2 * nodeID + 1];
}


Buffer<uint4> timerBuffer : register(t21);


cbuffer UniformBlock1 : register(b1) {
    int   currentSampleIndex;
    float numSamples;
    float sx;
    float sy;
}

static const float SQRT_3 = sqrt(3.0);
static const float HALF_SQRT_3 = SQRT_3 * 0.5;

static const float2 HEX_VERTS[7] = {
    float2(0, 1),
    float2(-HALF_SQRT_3, 0.5),
    float2(-HALF_SQRT_3, -0.5),
    float2(0,-1),
    float2(HALF_SQRT_3, -0.5),
    float2(HALF_SQRT_3, 0.5),
    float2(0, 0),
};

static const float2x3 HEX_TO_2D = float2x3(
     HALF_SQRT_3, 0,    -HALF_SQRT_3,
     -0.5,        1,    -0.5
);

static const int3 hex_direction_vectors[6] = {
    int3(+1, 0, -1), int3( 0, +1, -1), int3(-1, +1, 0),
    int3(-1, 0, +1), int3( 0, -1, +1), int3(+1, -1, 0)
};

int3 hex_dir(uint dir, int ring = 1) {
    return hex_direction_vectors[uint(dir) % 6] * ring;
}

int3 hex_add(int3 a, int3 b) {
    return a + b;
}

int3 hex_neighbor(int3 h, int dir) {
    return h + hex_dir(dir);
}

uint2 hex_spiral_polar(uint i) {
    i -= 1; // forget about ring 0
    // compute the ring
    uint r = uint(floor(0.5 * (1.0 + sqrt(1.0 + float(i) * 4.0 / 3.0))));
    // sum at ring r aka index of the first index in the ring
    uint r0 = (r * (r - 1) / 2) * 6;
    // index in the ring
    uint ri = (i - r0);
    
    return int2(r, ri);
}

uint3 hex_add_polar(int3 h, uint2 pol) {
    // find which side / direction
    uint d = pol.y / pol.x;
    // and index on the ring side
    uint rsi = pol.y % pol.x;

    h = hex_dir(d, pol.x);
    h += hex_dir(d + 2, rsi);
    return h;
}

static const float PHI = 0.5 * (1.0 + sqrt(5.0));

static const float3 icosahedron_verts[12] = {
    float3(0, +1, +PHI),
    float3(0, +1, -PHI),
    float3(0, -1, +PHI),
    float3(0, -1, -PHI),
    float3(+1, +PHI, 0),
    float3(+1, -PHI, 0),
    float3(-1, +PHI, 0),
    float3(-1, -PHI, 0),
    float3(+PHI, 0, +1),
    float3(-PHI, 0, +1),
    float3(+PHI, 0, -1),
    float3(-PHI, 0, -1)
};

static const uint3 icosahedron_indices[20] = {
    uint3(0, 2, 8),
    uint3(0, 8, 4),
    uint3(0, 4, 6),
    uint3(0, 6, 9),
    uint3(0, 9, 2),

    uint3(3, 10, 5),
    uint3(3, 5, 7),
    uint3(3, 7, 11),
    uint3(3, 11, 1),
    uint3(3, 1, 10),

    uint3(2, 5, 8),
    uint3(8, 5, 10),
    uint3(8, 10, 4),
    uint3(4, 10, 1),
    uint3(4, 1, 6),
    
    uint3(6, 1, 11),
    uint3(6, 11, 9),
    uint3(9, 11, 7),
    uint3(7, 9, 2),
    uint3(7, 2, 5)
};


struct VertexShaderOutput
{
    float4 Coords : TEXCOORD;
    float4 Position : SV_Position;
};

VertexShaderOutput main_hex(uint ivid : SV_VertexID)
{
    VertexShaderOutput OUT;

    const int num_tris = 6;
    uint vid = ivid % (3 * num_tris);
    uint instance = ivid / (3 * num_tris);
    uint tvid = vid % 3;
    uint tid = vid / 3;

    uint vertIdx = (tvid ? (tvid + tid - 1) % 6 : 6);
    float3 position = float3(HEX_VERTS[vertIdx], 0.0);

    float4 coords = float4(position.xy, instance, sy);
 
    int3 h = 0;
    
    
    const int num_faces = 20;
    int face_id = instance % num_faces; 
    int i = instance / num_faces;
    if ((i > 0)) //  && (i < 19))
    {
        uint2 pol = hex_spiral_polar(i);
        h = hex_add_polar(h, pol);
        coords.zw = float2(pol);
        position.xy += mul(HEX_TO_2D, float3(h));
    }    
    position.xy *= 0.08;
    position = position.xzy;
    
    
    //position.y = face_id * 2.0;
    
    
    // BUild face base to transform hex
    uint3 faceIdxs = icosahedron_indices[face_id];
    float3 faceOri = (icosahedron_verts[faceIdxs.x]
    + icosahedron_verts[faceIdxs.y]
    + icosahedron_verts[faceIdxs.z]) /3.0;
  //  faceOri = icosahedron_verts[face_id];
    
    float3 faceNor = normalize(faceOri);
    float3 faceUp = normalize(icosahedron_verts[faceIdxs.x] - faceOri);
    float3 faceTan = normalize(icosahedron_verts[faceIdxs.z] - icosahedron_verts[faceIdxs.y]);
    
    position = faceOri * 1.0
    +faceTan * position.x + faceUp * position.z;
    
    float3 color = float3(1.0, 1.0, 1.0);
    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);

    OUT.Position = clipPos;
    OUT.Coords = coords;
 
    return OUT;
}

