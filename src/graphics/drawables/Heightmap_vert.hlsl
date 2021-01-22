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
    int  _map_width;
    int  _map_height;
    float _map_spacing;
    int  _mesh_width;
    int  _mesh_height;
    float _mesh_spacing;
}



// Heightmap
//
StructuredBuffer<float>  heightmap_buffer : register(t1);

float3 getHeight(float x, float y) {
    float3 res;
    // map to heightmap base region
    float xf = x + (0.5 * _map_width * _map_spacing);
    float yf = y + (0.5 * _map_height * _map_spacing);

    float xtf = xf / float(_map_width * _map_spacing);
    float ytf = yf / float(_map_height * _map_spacing);

    int xt, yt;
    res.x = modf(xtf, xt);
    if (xtf < 0) {
        res.x = 1.0f + res.x;
        xt -= 1;
    }
    res.y = modf(ytf, yt);
    if (ytf < 0) {
        res.y = 1.0f + res.y;
        yt -= 1;
    }

    int xi = int(res.x * _map_width);
    int yi = int(res.y * _map_height);

    if (!((xt == 0) && (yt == 0))) { // clamp height to heightmap region
        xi = _map_width + 1;
        yi = _map_height + 1 ;
    }
    res.z = heightmap_buffer[xi + yi * _map_width];


    return res;
}


float4 evalTiledPosition(float2 p) {
    float4 res;
    // map to heightmap base region
    float xf = p.x + (0.5 * _map_width * _map_spacing);
    float yf = p.y + (0.5 * _map_height * _map_spacing);

    float xtf = xf / float(_map_width * _map_spacing);
    float ytf = yf / float(_map_height * _map_spacing);

    int xt, yt;
    res.x = modf(xtf, xt);
    if (xtf < 0) {
        res.x = 1.0f + res.x;
        xt -= 1;
    }
    res.y = modf(ytf, yt);
    if (ytf < 0) {
        res.y = 1.0f + res.y;
        yt -= 1;
    }

    res.z = xt;
    res.w = yt;

    /*if (!((xt == 0) && (yt == 0))) { // clamp height to heightmap region
    }*/

    return res;
}

float fetchHeight(int2 p) {
   return 0.2 * heightmap_buffer[p.x + p.y * _map_width];
   // return 10.0 * sin(p.x * 0.1) * cos(p.y * 0.2);
}

float getHeight(float2 p) {
    int2 pi = int2(int(p.x * _map_width), int(p.y * _map_height));
    return fetchHeight(pi);
}

float3 getNormal(float2 p) {
    int2 pi = int2(int(p.x * _map_width), int(p.y * _map_height));

    float h0 = fetchHeight(pi);
    float hx = fetchHeight(int2(pi.x + 1, pi.y));
    float hy = fetchHeight(int2(pi.x, pi.y + 1));
    float hxb = fetchHeight(int2(pi.x - 1, pi.y));
    float hyb = fetchHeight(int2(pi.x, pi.y - 1));

    float3 vx = float3( _map_spacing, hx - h0, 0.0);
    float3 vz = float3(0.0, hy - h0,  _map_spacing);

    return normalize(cross(vz, vx));
}

float3 getSmoothNormal(float2 p) {
    p *= float2(_map_width, _map_height);
    int2 pi00;
    float2 fp = modf(p, pi00);

    int2 pi11 = int2((pi00.x + 1) % _map_width, (pi00.y + 1) % _map_height);
    int2 pi10 = int2(pi11.x, pi00.y);
    int2 pi01 = int2(pi00.x, pi11.y);

    float h00 = fetchHeight(pi00);
    float h11 = fetchHeight(pi11);
    float h10 = fetchHeight(pi10);
    float h01 = fetchHeight(pi01);


    float3 vx0 = float3(_map_spacing, h10 - h00, 0.0);
    float3 vz0 = float3(0.0, h01 - h00, _map_spacing);
 
    float3 vx1 = float3(_map_spacing, h11 - h10, 0.0);
    float3 vz1 = float3(0.0, h11 - h01, _map_spacing);
 
    return normalize(cross(lerp(vz0, vz1, fp.x), lerp(vx0, vx1, fp.y)));
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
    float3 WPos : WPOS;
    float3 Normal   : SPRITE;
    float4 Position : SV_Position;
};

VertexShaderOutput main(uint ivid : SV_VertexID)
{
    VertexShaderOutput OUT;

    const int numVertPerWidth = 2 * (_mesh_width + 1);
    const int numVertPerStrip = numVertPerWidth + 1;
    int strip_id = ivid / numVertPerStrip;
    int strip_odd = strip_id % 2;
    int strip_vert_id = ivid % numVertPerStrip;
    
    strip_vert_id = (strip_vert_id == numVertPerWidth ? numVertPerWidth - 1 : strip_vert_id);
    int svid = strip_vert_id / 2;
    int ovid = strip_vert_id % 2;
    svid = (strip_odd ? _mesh_width - svid : svid);

    const float3 offset = float3(_mesh_width, 0.0, _mesh_height) * 0.5f;
    float3 position = float3((svid - offset.x), 0.0, (strip_id + ovid - offset.z)) * _mesh_spacing;

    float4 tilePos = evalTiledPosition(position.xz);
    if (!(tilePos.z == 0.0f && tilePos.w == 0.0f)) {
        tilePos.xy = float2(-1.0f, -1.0f);
    }
    position.y = getHeight(tilePos.xy);
    float3 normal = getSmoothNormal(tilePos.xy);

    float3 color = float3(tilePos.x, 0.0, tilePos.y);
 //   float3 color = dot(normal, normalize(float3(-1.0f, 1.0f, -1.0f)));
   // float3 color = normal;

    Transform _model = node_getWorldTransform(_nodeID);

    position = worldFromObjectSpace(_model, position);
    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);

    OUT.Position = clipPos;
    OUT.WPos = position;
    OUT.Color = float4(color, 1.0f);
    OUT.Normal = normal;

    return OUT;
}

