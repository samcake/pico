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

cbuffer UniformBlock1 : register(b1) {
    int   _nodeID;
    int  _width;
    int  _height;
    float _spacing;
}



// Heightmap
//
StructuredBuffer<float>  heightmap_buffer : register(t1);

float getHeight(float x, float z) {

    return heightmap_buffer[0];
}

struct VertexPosColor
{
    float3 Position : POSITION;
    //  float3 Normal : NORMAL;
    float4 Color : COLOR;
};

struct VertexShaderOutput
{
    float4 Color    : COLOR;
    float4 Position : SV_Position;
};

VertexShaderOutput main(uint ivid : SV_VertexID)
{
    VertexShaderOutput OUT;

    const int numVertPerWidth = 2 * (_width + 1);
    const int numVertPerStrip = numVertPerWidth + 1;
    int strip_id = ivid / numVertPerStrip;
    int strip_odd = strip_id % 2;
    int strip_vert_id = ivid % numVertPerStrip;
    
    strip_vert_id = (strip_vert_id == numVertPerWidth ? numVertPerWidth - 1 : strip_vert_id);
    int svid = strip_vert_id / 2;
    int ovid = strip_vert_id % 2;
    svid = (strip_odd ? _width - svid : svid);

    const float3 offset = float3(_width, 0.0, _height) * 0.5f;
    float3 position = float3((svid - offset.x), 0.0, (strip_id + ovid - offset.z)) * _spacing;
    float3 color = float3(float(strip_vert_id) / float(numVertPerStrip), float(strip_id) / float(_height), 1.0);

    position.y = heightmap_buffer[(strip_id + ovid) * _width + svid];//getHeight(position.x, position.z);

    Transform _model = node_getWorldTransform(_nodeID);

    position = worldFromObjectSpace(_model, position);
    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);

    OUT.Position = clipPos;
    OUT.Color = float4(color, 1.0f);
 
    return OUT;
}

