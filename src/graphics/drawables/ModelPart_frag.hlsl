
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
    float4 emissive;
    uint4 textures;
};

StructuredBuffer<Material>  material_array : register(t9);

Texture2DArray material_textures : register(t10);
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

// Compute tangent space and final normal
float3 computeNormal(float3 surfNormal, float3 normalMap, float3 eyePos, float2 uv) {
    float3 q0 = ddx(eyePos);
    float3 q1 = ddy(eyePos);
    float2 st0 = ddx(uv);
    float2 st1 = ddy(uv);

    float3 S = normalize(q0 * st1.y - q1 * st0.y);
    float3 T = normalize(-q0 * st1.x + q1 * st0.x);
    float3 normal = surfNormal;
    /*
        normalMap.xy = 1.0 * normalMap.xy;
        float3x3 tsn = float3x3(S, T, normal);
        normal = mul(tsn, normalMap.xyz);
        normal = normalize(normal);
      */  return normal;
}

//
// Main
// 
bool LIGHT_SHADING(int drawMode) { return (drawMode & 0x00000080) != 0; }

int DISPLAYED_COLOR_BITS() { return 0x0000000F; }
int DISPLAYED_COLOR_OFFSET() { return 0; }
int DISPLAYED_COLOR(int drawMode) { return (drawMode & DISPLAYED_COLOR_BITS()) >> DISPLAYED_COLOR_OFFSET(); }


cbuffer UniformBlock1 : register(b1) {
    int _nodeID;
    int _partID;
    int _numNodes;
    int _numParts;
    int _numMaterials;
    int _drawMode;
}

struct PixelShaderInput{
    float3 EyePos : EPOS;
    float3 Normal   : NORMAL;
    float2 Texcoord : TEXCOORD;
};

float4 main(PixelShaderInput IN) : SV_Target{
    int matIdx = part_array[_partID].material;// < 0.0 ? 0 : floor(IN.Material));
//    int matIdx = asint(IN.Material);// < 0.0 ? 0 : floor(IN.Material));
    Material m = material_array[matIdx];

    float4 mapNor = float4(0.0, 0.0, 0.0, 0.0);
    if (m.textures.y != -1) {
        mapNor = float4(material_textures.Sample(uSampler0, float3(IN.Texcoord.xy, m.textures.y)).xyz, 1.0);
    }

    float4 mapN = (mapNor * 2.0 - 1.0);
    float3 surfNormal = normalize(IN.Normal);
    float3 normal = computeNormal(surfNormal, mapN, IN.EyePos.xyz, IN.Texcoord.xy);
    //normal = worldFromEyeSpaceDir(_view, normal);

    float4 rmaoMap = float4(0.0, 0.0, 0.0, 0.0);
    if (m.textures.z != -1) {
        rmaoMap = material_textures.Sample(uSampler0, float3(IN.Texcoord.xy, m.textures.z));
    }

    // with albedo from property or from texture
    float3 albedo = m.color;
    if (m.textures.x != -1) {
        albedo = material_textures.Sample(uSampler0, float3(IN.Texcoord.xy, m.textures.x)).xyz;
    }

    float3 baseColor = float3(1.0, 1.0, 1.0);
    switch (DISPLAYED_COLOR(_drawMode)) {
    case 0: baseColor = albedo; break;
    case 1: baseColor = normal; break;
    case 2: baseColor = surfNormal; break;
    case 3: baseColor = mapNor; break;
    }

    const float3 globalD = normalize(float3(0.0f, 1.0f, 0.0f));
    const float globalI = 0.3f;
    const float3 lightD = normalize(float3(-1.0f, -1.0f, 1.0f));
    const float lightI = 0.8f;

    float NDotL = clamp(dot(normal, -lightD), 0.0f, 1.0f);
    float NDotG = clamp(dot(normal, -globalD), 0.0f, 1.0f);

    float3 shading = float3(1.0, 1.0, 1.0);
    if (LIGHT_SHADING(_drawMode)) {
        shading = (NDotL * lightI + NDotG * globalI);
    }

    float3 emissiveColor = float3 (0.0, 0.0, 0.0);
    if (m.textures.w != -1) {
        emissiveColor = material_textures.Sample(uSampler0, float3(IN.Texcoord.xy, m.textures.w)).xyz;
    }
 
    float3 color = shading * baseColor + emissiveColor;
    return float4(color, 1.0);
}