
//
// Color API
// 

float3 colorRGBfromHSV(const float3 hsv) {
    const float4 k = float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
    float3 f = frac(float3(hsv.x + k.x, hsv.x + k.y, hsv.x + k.z)) * 6.0f;
    float3 p = abs(float3(f.x - k.w, f.y - k.w, f.z - k.w));
    p = clamp(float3(p.x - k.x, p.y - k.x, p.z - k.z), float3(0, 0, 0), float3(1.0, 1.0, 1.0));
    return lerp(k.xxx, p, hsv.yyy) * hsv.z;
}

float3 rainbowRGB(float n, float d = 1.0f) {
    return colorRGBfromHSV(float3((n / d) * (5.0 / 6.0), 3.0, 1.0));
}

//
// Material API
// 
struct Material {
    float4 color;
    float metallic;
    float roughness;
    float spareA;
    float spareB;
    uint4 textures;
};

StructuredBuffer<Material>  material_array : register(t5);

Texture2D uTex0 : register(t0);
SamplerState uSampler0 : register(s0);

//
// Model & Parts
// 

struct Part {
    uint numIndices;
    uint indexOffset;
    uint vertexOffset;
    uint attribOffset;
    uint material;
    uint spareA;
    uint spareB;
    uint spareC;
};

StructuredBuffer<Part>  part_array : register(t1);

//
// Main
// 


cbuffer UniformBlock1 : register(b1) {
    int _nodeID;
    int _partID;
    int _numNodes;
    int _numParts;
    int _numMaterials;
}

struct PixelShaderInput{
    float3 Normal   : NORMAL;
    float2 Texcoord : TEXCOORD;
};

float4 main(PixelShaderInput IN) : SV_Target{
    int matIdx = part_array[_partID].material;// < 0.0 ? 0 : floor(IN.Material));
//    int matIdx = asint(IN.Material);// < 0.0 ? 0 : floor(IN.Material));
    Material m = material_array[matIdx];

    const float3 globalD = normalize(float3(0.0f, 1.0f, 0.0f));
    const float globalI = 0.3f;
    const float3 lightD = normalize(float3(-1.0f, -1.0f, 1.0f));
    const float lightI = 0.8f;

    float3 normal = normalize(IN.Normal);
    float NDotL = clamp(dot(normal, -lightD), 0.0f, 1.0f);
    float NDotG = clamp(dot(normal, -globalD), 0.0f, 1.0f);

    float3 shading = float3(1.0, 1.0, 1.0);
    shading = (NDotL * lightI + NDotG * globalI);
    
    float3 baseColor = float3(1.0, 1.0, 1.0);
    // with albedo from property or from texture
    baseColor = m.color;
    if (m.textures.x != -1) {
        baseColor = uTex0.Sample(uSampler0, IN.Texcoord.xy).xyz;
    }

  //  baseColor = 0.5 * (normal + float3(1.0, 1.0, 1.0));
  //  baseColor = normal;
  //  baseColor = rainbowRGB(IN.Material, float(_numMaterials));
  //  baseColor = rainbowRGB(_partID, float(_numParts));
  //  baseColor = rainbowRGB(_nodeID, float(_numNodes));
  //  baseColor = float3(IN.Texcoord.x, 0.0f * IN.Texcoord.y, 0.0f);

    
 
    float3 color = shading * baseColor;
    return float4(color, 1.0);
}