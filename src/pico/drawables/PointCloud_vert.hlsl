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


cbuffer UniformBlock0 : register(b0) {
    //float4x3 _view;
    View _view;
    float4 _projection;
    float4 _viewport;
};

float4 clipFromEyeSpace(float focal, float sensorHeight, float aspectRatio, float pfar, float3 eyePos) {

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
float4 clipFromEyeSpace(float4 proj, float3 eyePos) {
    return clipFromEyeSpace(proj.x, proj.y, proj.z, proj.w, eyePos);
}


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
    float pointSize : PSIZE;
};

VertexShaderOutput main(VertexPosColor IN)
{
    VertexShaderOutput OUT;

    float3 position = IN.Position;

    position = worldFromObjectSpace(_model, position);
    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);

    OUT.Position = clipPos;
    OUT.Color = float4(IN.Color.xyz, 1.0f);
    OUT.pointSize = 0.5;

    return OUT;
}
