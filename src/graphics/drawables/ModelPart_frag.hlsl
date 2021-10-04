

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
    float3 x0 = (value - offset) / (period)-normalizedWidth * 0.5;
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



// Drawgrid
float3 drawGrid(float2 uv, float3 color) {
    float tex_width, tex_height, tex_elements;
    material_textures.GetDimensions(tex_width, tex_height, tex_elements);

    float3 texcoord = float3(uv.x * tex_width, uv.y * tex_height, 1.0f);

    float3 grid = paintStripe(texcoord, tex_width * 0.1f, 1.5f);
    color = lerp(color, float3(1.0, 0.0, 0.0), grid.x);
    color = lerp(color, float3(0.0, 1.0, 0.0), grid.y);

    grid = paintStripe(texcoord, tex_width * 0.01f, 0.5f);
    color = lerp(color, float3(0.8, 0.8, 1.8), max(grid.x, grid.y));

    return color;
}

// Technique to compute tangent space in fragment shader by
// Christian Schüler
// http://www.thetenthplanet.de/archives/1180
// Compute tangent space and final normal
float3x3 computeTangentSpace(float3 surfNormal, float3 viewVec, float2 uv) {
    float3 dp1 = ddx(viewVec);
    float3 dp2 = ddy(viewVec);
    float2 duv1 = ddx(uv);
    float2 duv2 = ddy(uv);

    float3 dp2perp = cross(dp2, surfNormal); // V ^ N ~= T
    float3 dp1perp = cross(surfNormal, dp1); // N ^ U ~= B
    float3 T = (dp2perp * duv1.x + dp1perp * duv2.x);
    float3 B = (dp2perp * duv1.y + dp1perp * duv2.y);

    float invmax = rsqrt(max(dot(T, T), dot(B, B)));
    T *= invmax;
    B *= invmax;
    return float3x3(T,B, surfNormal);
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
    Material m = material_array[matIdx];

    // Normal and Normal map
    float3 surfNormal = normalize(IN.Normal);
    float3 mapNor = float4(0.0, 0.0, 0.0, 0.0);
    float3 normal = surfNormal;
    if (m.textures.y != -1) {
        mapNor = float3(material_textures.Sample(uSampler0, float3(IN.Texcoord.xy, m.textures.y)).xyz);
        float3 mapN = (mapNor * 2.0 - 1.0);
        // assume surfNormal, the inter­po­lat­ed ver­tex nor­mal and 
        // V, the view vec­tor (ver­tex to eye)
        //   and then -V => eye to vertex
        float3 mV = -IN.EyePos.xyz;
        float3x3 tbn = computeTangentSpace(surfNormal, mV, IN.Texcoord.xy);
        normal = (mul(mapN, tbn));
        normal = normalize(normal);
        normal = normal.xyz; // i m puzzled by the fact that we don't need to swizzle the result here ? whatever it works
    }

    float4 rmaoMap = float4(0.0, 0.0, 0.0, 0.0);
    if (m.textures.z != -1) {
        rmaoMap = material_textures.Sample(uSampler0, float3(IN.Texcoord.xy, m.textures.z));
    }

    // with albedo from property or from texture
    float3 albedo = m.color;
    if (m.textures.x != -1) {
        albedo = material_textures.Sample(uSampler0, float3(IN.Texcoord.xy, m.textures.x)).xyz;
    }

    float3 baseColor = float3(0.7, 0.7, 0.7);
    switch (DISPLAYED_COLOR(_drawMode)) {
    case 0: baseColor = albedo; break;
    case 1: baseColor = normal; break;
    case 2: baseColor = surfNormal; break;
    case 3: baseColor = mapNor; break;
    case 4: baseColor = rmaoMap; break;
    case 5: ; break;
    }

    baseColor = drawGrid(IN.Texcoord.xy, baseColor);

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