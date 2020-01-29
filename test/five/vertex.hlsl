
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


float4 clipFromEyeSpace(float focal, float sensorHeight, float aspectRatio, float far, float3 eyePos) {
    
   // float foc = focal* (eyePos.x < 0.0 ? 1.0 : 1.5);
    float foc = focal;
    float4 clipPos;
    clipPos.w = foc - eyePos.z;
    clipPos.z = (-eyePos.z) *far / (far - foc);
    clipPos.x = eyePos.x * foc / (0.5 * sensorHeight * aspectRatio);
    clipPos.y = eyePos.y * foc / (0.5 * sensorHeight);
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
    return float3( dot(right, eyeCenteredPos), dot(up, eyeCenteredPos), dot(back, eyeCenteredPos) );
}

float3 eyeFromWorldSpace(View view, float3 worldPos) {
    // TranslationInv
    float3 eyeCenteredPos = worldPos - view.eye();
  //  return eyeCenteredPos;
    // RotationInv
    return float3(dot(view.right(), eyeCenteredPos), dot(view.up(), eyeCenteredPos), dot(view.back(), eyeCenteredPos));
 }


struct VertexPosColor
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
};

struct VertexShaderOutput
{
    float4 Color    : COLOR;
    float4 Position : SV_Position;
};

VertexShaderOutput mainVertex(VertexPosColor IN)
{
    VertexShaderOutput OUT;

    float3 position = IN.Position * 0.075f;

    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);

    OUT.Position = clipPos;
    OUT.Color = float4(IN.Color.xyz, 1.0f);

    return OUT;
}
