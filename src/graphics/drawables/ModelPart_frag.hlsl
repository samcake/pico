
struct Material {
    float4 color;
    float metallic;
    float roughness;
    float spareA;
    float spareB;
};

StructuredBuffer<Material>  material_array : register(t4);


struct PixelShaderInput{
    float3 Normal   : NORMAL;
    float Material  : COLOR;
};

float4 main(PixelShaderInput IN) : SV_Target{
    Material m = material_array[int(IN.Material)];

    const float globalD = normalize(float3(0.0f, 1.0f, 0.0f));
    const float globalI = 0.3f;
    const float3 lightD = normalize(float3(-1.0f, -1.0f, -1.0f));
    const float lightI = 0.7f;

    float NDotL = clamp(dot(IN.Normal, -lightD), 0.0f, 1.0f);
    float NDotG = clamp(dot(IN.Normal, -globalD), 0.0f, 1.0f);
   // float3 color = (NDotL * lightI + NDotG * globalI) * IN.Color.xyz;
    float3 color = (NDotL * lightI + NDotG * globalI) * m.color;
  //  color = 0.5 * (IN.Normal + float3(1.0f, 1.0f, 1.0f));

    return float4(color, 1.0);
}