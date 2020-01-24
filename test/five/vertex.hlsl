/*struct ModelViewProjection
{
    matrix MVP;
};
*/
//ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

cbuffer UniformBlock0 : register(b0)
{
    //pico::vec3 _eye{ 0.0f };                float _focal { 0.036f };
    //pico::vec3 _right { 1.f, 0.f, 0.f};     float _sensorHeight { 0.056f };
    //pico::vec3 _up { 0.f, 1.f, 0.f };       float _aspectRatio { 16.f / 9.f };
    //pico::vec3 _back { 0.f, 0.f, -1.f };    float _far { 10.0f };

    float4 eye_focal;
    float4 right_sensorHeight;
    float4 up_aspectRatio;
    float4 back_far;
};

float3 eyeFromClipSpace(float focal, float sensorHeight, float aspectRatio, float2 clipPos) {
	return float3(clipPos.x*aspectRatio*sensorHeight * 0.5, clipPos.y * sensorHeight * 0.5, -focal);
}
float3 worldFromEyeSpaceDir(float3 right, float3 up, float3 eyeDir) {
	return eyeDir * eyeDir.x + up * eyeDir.y + cross(right, up) * eyeDir.z;
}
float3 worldFromEyeSpace(float3 eye, float3 right, float3 up, float3 eyePos) {
	return worldFromEyeSpaceDir(right, up, eyePos) + eye;
}
/*
float3 worldFromEyeSpaceDir(float3 right, float3 up, float3 eyeDir) {
	return eyeDir * eyePos.x + up * eyeDir.y + cross(right, up) * eyeDir.z;
}*/
float3 eyeFromWorldSpace(float3 eye, float3 right, float3 up, float3 worldPos) {
	float3 eyeCenteredPos = worldPos - eye;
    return float3( 
        dot(right, eyeCenteredPos),
        dot(up, eyeCenteredPos),
        dot(cross(right, up), eyeCenteredPos)
    );
}

float4 clipFromEyeSpace(float focal, float sensorHeight, float aspectRatio, float far, float3 eyePos) {
    
   // float foc = focal* (eyePos.x < 0.0 ? 1.0 : 1.5);
    float foc = focal;
    float w = foc - eyePos.z;
    float z = (-eyePos.z) *far / (far - foc);
    float x = eyePos.x * foc / (0.5 * sensorHeight * aspectRatio);
    float y = eyePos.y * foc / (0.5 * sensorHeight);
    return float4(x, y, z, w);
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
    float3 eyePosition = eyeFromWorldSpace(eye_focal.xyz, right_sensorHeight.xyz, up_aspectRatio.xyz, position);
  
    float4 clipPos = clipFromEyeSpace(eye_focal.w, right_sensorHeight.w, up_aspectRatio.w, back_far.w, eyePosition);
    OUT.Position = clipPos;
    OUT.Color = float4(IN.Color.xyz, 1.0f);

    return OUT;
}
