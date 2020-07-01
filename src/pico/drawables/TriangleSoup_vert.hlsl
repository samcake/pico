#define mat43 float4x3

struct View {
    float4 _right_upX;
    float4 _upYZ_backXY;
    float4 _backZ_eye;

    float3 right() { return _right_upX.xyz; }
    float3 up() { return float3(_right_upX.w, _upYZ_backXY.xy); }
    float3 back() { return float3(_upYZ_backXY.zw, _backZ_eye.x); }
    float3 eye() { return _backZ_eye.yzw; }
};

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
    } else {
        return clipFromEyeSpace(proj.aspectRatio(), proj.sensorHeight(), proj.focal(), proj.persFar(), eyePos);
    }
}

cbuffer UniformBlock0 : register(b0) {
    //float4x3 _view;
    View _view;
    Projection _projection;
    float4 _viewport;
};


float3 eyeFromWorldSpace(float3 right, float3 up, float3 back, float3 eye, float3 worldPos) {
    // TranslationInv
    float3 eyeCenteredPos = worldPos - eye;
    return eyeCenteredPos;
    // RotationInv
    return float3(dot(right, eyeCenteredPos), dot(up, eyeCenteredPos), dot(back, eyeCenteredPos));
}

float3 eyeFromWorldSpace(View view, float3 worldPos) {
    // TranslationInv
    float3 eyeCenteredPos = worldPos - view.eye();
    //  return eyeCenteredPos;
      // RotationInv
    return float3(dot(view.right(), eyeCenteredPos), dot(view.up(), eyeCenteredPos), dot(view.back(), eyeCenteredPos));
}

struct Transform {
    float4 _right_upX;
    float4 _upYZ_backXY;
    float4 _backZ_ori;

    float3 right() { return _right_upX.xyz; }
    float3 up() { return float3(_right_upX.w, _upYZ_backXY.xy); }
    float3 back() { return float3(_upYZ_backXY.zw, _backZ_ori.x); }

    float3 invX() { return float3(_right_upX.x, _right_upX.w, _upYZ_backXY.z); }
    float3 invY() { return float3(_right_upX.y, _upYZ_backXY.x, _upYZ_backXY.w); }
    float3 invZ() { return float3(_right_upX.z, _upYZ_backXY.y, _backZ_ori.x); }

    float3 ori() { return _backZ_ori.yzw; }
};

cbuffer UniformBlock1 : register(b1) {
    Transform _model;
}

float3 worldFromObjectSpace(Transform model, float3 objPos) {
    float3 rotatedPos = float3(dot(model.invX(), objPos), dot(model.invY(), objPos), dot(model.invZ(), objPos));
    // TranslationInv
    return  rotatedPos + model.ori();
}

/*struct VertexPosColor
{
    float3 Position : POSITION;
    //  float3 Normal : NORMAL;
    float4 Color : COLOR;
};*/

struct PointPosColor {
    float x;
    float y;
    float z;
    uint  color;
};

StructuredBuffer<PointPosColor>  BufferIn : register(t0);

struct VertexShaderOutput
{
    float4 Color    : COLOR;
    float2 sprite   : SPRITE;
    float4 Position : SV_Position;
};

VertexShaderOutput main(uint vidT : SV_VertexID)
{
    VertexShaderOutput OUT;
    uint vid = vidT / 3;
    uint sid = vidT - vid * 3;

    uint color = BufferIn[vid].color;
    const float INT8_TO_NF = 1.0 / 255.0;
    float r = INT8_TO_NF * (float)((color >> 0) & 0xFF);
    float g = INT8_TO_NF * (float)((color >> 8) & 0xFF);
    float b = INT8_TO_NF * (float)((color >> 16) & 0xFF);
    float3 position = float3(BufferIn[vid].x, BufferIn[vid].y, BufferIn[vid].z);

    position = worldFromObjectSpace(_model, position);
    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);

    // make the sprite
    OUT.sprite = float2(((sid == 1) ? 3.0 : -1.0), ((sid == 2) ? -3.0 : 1.0));

    const float spriteSize = 1.5;
    float2 invRes = float2(1.0 / _viewport.z, 1.0 / _viewport.w);
    clipPos.xy += invRes.xy * OUT.sprite * spriteSize * clipPos.w;// pixel size or 3d size? 

    OUT.Position = clipPos;
    OUT.Color = float4(r, g, b, 1.0f);

    return OUT;
}
