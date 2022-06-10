#include "Color_inc.hlsl"
#include "Hexagon_inc.hlsl"

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

float sdHexagonGrid(in float2 p, in float s)
{
    float r = 1.0;
    float h = 0.5 * s;
    float w = SQRT_3 * h;
    
    // scale
    p += float2(w, h);
    p /= float2(w * 2.0, h * 3.0);
    
    // offset
    float even = float(int(floor(p.y)) & 0x01) ;
    p.x += 0.5 * even;
    
    // repeat
    p = frac(p);
    
    // unscale
    p *= float2(w * 2.0, h * 3.0);
    p -= float2(w, h);
    
    return sdHexagonV(p, r * w);
}

float3 hex_ortho_to_cube(float2 p, float radius = 1.0)
{
    p /= radius;
    float xc = 1.0 / SQRT_3;
    
    float q = xc * p.x + (-1.0 / 3.0) * p.y;
    float r = (2.0 / 3.0) * p.y;
    float s = -xc * p.x + (-1.0 / 3.0) * p.y;
    
    return float3(q, r, -q - r);
}




float4 main(PixelShaderInput IN) : SV_Target
{
    float ring = 1.0;

    float2 p = IN.coords.xz;
    
    float3 hf = hex_ortho_to_cube(p, ring);
    
    int3 hi = hex_round(hf);
    
    float3 hic = (hf - hi) ;
    float2 ip = mul(HEX_TO_2D, hic);
   
    float b0 = 2.0 / 3.0;
    float b1 = 0.0;
    float b2 = -1.0 / 3.0;
    float b3 = SQRT_3 / 3.0;
    float hic_q = ip.x * b0 + ip.y * b1;
    float hic_r = ip.x * b2 + ip.y * b3;
    float3 hic2 = float3(hic_q, hic_r, -hic_r - hic_q);
    hic2 = abs(hic2);
    float s_id =  1.0 - (hic2.x + hic2.y + hic2.z) * 6.0 / 7.0;
    
    
    
    float2 pb = hex_cube_to_ortho(hf, ring);
        
    //int d = hex_dist(hi, int3(0, 0, 0));
    int d = hex_dist(hi, int3(0, 0, 0));
    
   
  //  float3 color = color_rainbow(IN.coords.w / (6.0 * IN.coords.z), 1.0 - IN.coords.z / 20.0);
  //  float3 color = color_rainbow(float(d), 10);
  //  float3 color = color_rainbow(pb.y, 10);
  
   float3 color = color_rainbow(float(d), 6.0);
    
   // float3 color = color_rainbow(0.5 * hic.x * 3.0 / 2.0);
    
    
   // float3 color = color_rainbow(length(hic2));
   // float3 color = color_rainbow(s_id);
    //float3 color = color_rainbow(ip.x * xc);
    
   // float3 color = abs(round(hf) / 6.0);
  //  float3 color = 1 - frac(abs(hic));

  //  color = lerp(color, 0.5, check != 0);
    
  //  float a = clamp(1.0 - abs(sdHexagonGrid(p, ring)) * 50.0, 0, 1);
    float a = clamp(1.0 - abs(s_id) * 20.0, 0, 1);

    color = lerp(color, 0.9, a);
    
    color = lerp(color, float3(0.2, 1.0, 0.2), a * (hi.x == 0) * (hi.y == 0) * (hi.z == 0));
    
    
    return float4(color, 1.0);
}