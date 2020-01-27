struct CameraData {
    float4x3 _view;
    float4 _projection;
};

#define mat43 float4x3

cbuffer UniformBlock0 : register(b0) {
    mat43 _view;
    float4 _projection;
};


float4 clipFromEyeSpace(float focal, float sensorHeight, float aspectRatio, float far, float3 eyePos) {
    
   // float foc = focal* (eyePos.x < 0.0 ? 1.0 : 1.5);
    float foc = focal;
    float w = foc - eyePos.z;
    float z = (-eyePos.z) *far / (far - foc);
    float x = eyePos.x * foc / (0.5 * sensorHeight * aspectRatio);
    float y = eyePos.y * foc / (0.5 * sensorHeight);
    return float4(x, y, z, w);
}
float4 clipFromEyeSpace(float4 proj, float3 eyePos) {
    return clipFromEyeSpace(proj.x, proj.y, proj.z, proj.w, eyePos);
}


float3 eyeFromWorldSpace(float3 right, float3 up, float3 back, float3 eye, float3 worldPos) {
	float3 eyeCenteredPos = worldPos - eye;
    return float3( 
        dot(right, eyeCenteredPos),
        dot(up, eyeCenteredPos),
        dot(back, eyeCenteredPos)
    );
}

float3 eyeFromWorldSpace(mat43 view, float3 worldPos) {
 /*   float4 pos = float4(worldPos, -1.0f);

    return float3(
        dot(view._m00_m10_m20_m30, pos),
        dot(view._m01_m11_m21_m31, pos),
        dot(view._m02_m12_m22_m32, pos)
        );*/

    return eyeFromWorldSpace(view._m00_m01_m02, view._m10_m11_m12, view._m20_m21_m22, view._m30_m31_m32, worldPos);
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
