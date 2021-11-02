


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

float sdHexagonGrid(in float2 p, in float s, in float r)
{
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
    float maxCoord = max(IN.coords.x, IN.coords.y);
    if (maxCoord > 1.0) discard;

    float3 color = float3(IN.coords.xy, 0);
    
    if (IN.coords.w == -1)
    {
        float2 p = IN.coords.xy * 2 - float2(1, 1);
        p *= 3.0;
        float2 pc = p;
        
        color = float3(frac(pc), 0);
        color = 0.2;
        
        float isoscale = 0.5;
        float hexscale = 1.0;
        
        float a = clamp(1.0 - abs(sdHexagonGrid(p, isoscale, hexscale)) * 50.0, 0, 1);

        color = lerp(color, float3(0.9, 0.9, 0.9), a);

    }
    
    return float4(color, 1.0);
}