
//
// Paint API
// 
float3 paintStripe(float3 value, float period, float stripe) {
    float3 normalizedWidth = fwidth(value);
    normalizedWidth /= (period);
    float half_stripe_width = 0.5 * stripe;
    float3 offset = float3(half_stripe_width, half_stripe_width, half_stripe_width);
    float stripe_over_period = stripe / period;
    float3 edge = float3(stripe_over_period, stripe_over_period, stripe_over_period);
    float3 x0 = (value - offset) / (period) - normalizedWidth * 0.5;
    float3 x1 = x0 + normalizedWidth;
    float3 balance = float3(1.0, 1.0, 1.0) - edge;
    float3 i0 = edge * floor(x0) + max(float3(0.0, 0.0, 0.0), frac(x0) - balance);
    float3 i1 = edge * floor(x1) + max(float3(0.0, 0.0, 0.0), frac(x1) - balance);
    float3 strip = (i1 - i0) / normalizedWidth;
    return clamp(strip, float3(0.0, 0.0, 0.0), float3(1.0, 1.0, 1.0));
}

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

StructuredBuffer<Material>  material_array : register(t5);

Texture2DArray uTex0 : register(t0);
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
    float3 EyePos : EPOS;
    float3 Normal   : NORMAL;
    float2 Texcoord : TEXCOORD;
};

float4 main(PixelShaderInput IN) : SV_Target{
    int matIdx = part_array[_partID].material;// < 0.0 ? 0 : floor(IN.Material));
//    int matIdx = asint(IN.Material);// < 0.0 ? 0 : floor(IN.Material));
    Material m = material_array[matIdx];

    float4 mapN = float4(0.0, 0.0, 0.0, 0.0);
    if (m.textures.y != -1) {
        mapN = float4(uTex0.Sample(uSampler0, float3(IN.Texcoord.xy, m.textures.y)).xyz * 2.0 - 1.0, 1.0);
    }
    float3 surf_normal = normalize(IN.Normal);


    float3 q0 = ddx(IN.EyePos.xyz);
    float3 q1 = ddy(IN.EyePos.xyz);
    float2 st0 = ddx(IN.Texcoord.xy);
    float2 st1 = ddy(IN.Texcoord.xy);

    float3 S = normalize(q0 * st1.y - q1 * st0.y);
    float3 T = normalize(-q0 * st1.x + q1 * st0.x);
    float3 normal = surf_normal;

    mapN.xy = 1.0 * mapN.xy;
    float3x3 tsn = float3x3(S, T, normal);
//    normal = mul( tsn, mapN.xyz);
 //   normal = normalize(normal);

    float4 rmaoMap = float4(0.0, 0.0, 0.0, 0.0);
    if (m.textures.z != -1) {
        rmaoMap = uTex0.Sample(uSampler0, float3(IN.Texcoord.xy, m.textures.z));
    }

    float3 baseColor = float3(1.0, 1.0, 1.0);
    // with albedo from property or from texture
    baseColor = m.color;
    if (m.textures.x != -1) {
        baseColor = uTex0.Sample(uSampler0, float3(IN.Texcoord.xy, m.textures.x)).xyz;
    }

    const float3 globalD = normalize(float3(0.0f, 1.0f, 0.0f));
    const float globalI = 0.3f;
    const float3 lightD = normalize(float3(-1.0f, -1.0f, 1.0f));
    const float lightI = 0.8f;

    float NDotL = clamp(dot(normal, -lightD), 0.0f, 1.0f);
    float NDotG = clamp(dot(normal, -globalD), 0.0f, 1.0f);

    float3 shading = float3(1.0, 1.0, 1.0);
  //  shading = (NDotL * lightI + NDotG * globalI);
    
  //  normal = T;

  //  baseColor = 0.5 * (normal + float3(1.0, 1.0, 1.0));
   // baseColor = normal;
  //  baseColor = rainbowRGB(IN.Material, float(_numMaterials));
  //  baseColor = rainbowRGB(_partID, float(_numParts));
  //  baseColor = rainbowRGB(_nodeID, float(_numNodes));
  //  baseColor = float3(IN.Texcoord.x, IN.Texcoord.y, 0.0f);
    
   //  baseColor = mapN;


    float3 emissiveColor = float3 (0.0, 0.0, 0.0);
    if (m.textures.w != -1) {
        emissiveColor = uTex0.Sample(uSampler0, float3(IN.Texcoord.xy, m.textures.w)).xyz;
    }
 
    float3 color = shading * baseColor + emissiveColor;

    // Add uv stripes
    float tex_width, tex_height, tex_elements;
    uTex0.GetDimensions(tex_width, tex_height, tex_elements);

    float3 texcoord = float3(IN.Texcoord.x * tex_width, IN.Texcoord.y * tex_height, 1.0f);

    float3 grid = paintStripe(texcoord, tex_width * 0.1f, 1.5f);
    color = lerp(color, float3(1.0, 0.0, 0.0), grid.x);
    color = lerp(color, float3(0.0, 1.0, 0.0), grid.y);

    grid = paintStripe(texcoord, tex_width * 0.01f, 0.5f);
    color = lerp(color, float3(0.8, 0.8, 1.8), max(grid.x, grid.y));

    //float2 normalizedWidth = fwidth(IN.Texcoord);
   // color = lerp(color, float3(normalizedWidth, 0.0), 1.0);

    return float4(color, 1.0);
}