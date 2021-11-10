
//
// Color API
// 

float3 colorRGBfromHSV(const float3 hsv)
{
    const float4 k = float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
    float3 f = frac(float3(hsv.x + k.x, hsv.x + k.y, hsv.x + k.z)) * 6.0f;
    float3 p = abs(float3(f.x - k.w, f.y - k.w, f.z - k.w));
    p = clamp(float3(p.x - k.x, p.y - k.x, p.z - k.z), float3(0, 0, 0), float3(1.0, 1.0, 1.0));
    return lerp(k.xxx, p, hsv.yyy) * hsv.z;
}

float3 rainbowRGB(float ratio, float sat = 0.89)
{
    return colorRGBfromHSV(float3((ratio), 0.78, sat));
}

struct PixelShaderInput
{
    float4 coords : TEXCOORD;
};

float sdHexagonH(in float2 p, in float r) {
    const float3 k = float3(-0.866025404, 0.5, 0.577350269);
    p = abs(p);
    p -= 2.0 * min(dot(k.xy, p), 0.0) * k.xy;
    p -= float2(clamp(p.x, -k.z * r, k.z * r), r);
    return length(p) * sign(p.y);
}
float sdHexagonV(in float2 p, in float r)
{
    const float3 k = float3(0.5, -0.866025404, 0.577350269);
    p = abs(p);
    p -= 2.0 * min(dot(k.xy, p), 0.0) * k.xy;
    p -= float2(r, clamp(p.y, -k.z * r, k.z * r));
    return length(p) * sign(p.x);
}

static const float SQRT_3 = sqrt(3.0);

float sdHexagonGrid(in float2 p, in float s)
{
    float r = 1.0;
    float h = 0.5 * s;
    float w = SQRT_3 * h;
    
    // scale
    p += float2(w, h);
    p /= float2(w * 2.0, h * 3.0);
    
    // offset
    float even = float(int(floor(p.y)) & 0x01);
    p.x += 0.5 * even;
    
    // repeat
    p = frac(p);
    
    // unscale
    p *= float2(w * 2.0, h * 3.0);
    p -= float2(w, h);
    
    return sdHexagonV(p, r * w);
}



float4 main(PixelShaderInput IN) : SV_Target
{
    float3 color = rainbowRGB(IN.coords.w / (6.0 * IN.coords.z), 1.0 - IN.coords.z / 20.0);
    float2 p = IN.coords.xy;
    float isoscale = 1.0;
        
    float a = clamp(1.0 - abs(sdHexagonGrid(p, isoscale)) * 50.0, 0, 1);

    color = lerp(color, 0.9, a);

    return float4(color, 1.0);
}