struct PixelShaderInput {
    float4 Color    : COLOR;
    float3 Normal   : NORMAL;
};

float4 main(PixelShaderInput IN) : SV_Target {
    const float3 globalD = normalize(float3(0.0f, 1.0f, 0.0f));
    const float globalI = 0.3f;
    const float3 lightD = normalize(float3(-1.0f, -1.0f, -1.0f));
    const float lightI = 0.7f;

    float NDotL = clamp(dot(IN.Normal, -lightD), 0.0f, 1.0f);
    float NDotG = clamp(dot(IN.Normal, -globalD), 0.0f, 1.0f);
    float3 color = (NDotL * lightI + NDotG * globalI) * IN.Color.xyz;

    return float4(color, 1.0);
}
