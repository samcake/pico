#include "Color_inc.hlsl"
#include "Hexagon_inc.hlsl"

struct PixelShaderInput
{
    float4 coords : TEXCOORD;
};

float3 eval_hex_from_ortho(float2 p, int l, out float3 hc)
{
    float scale = pow(INV_SQRT_3, float(l));
    float r = 1.0 * scale; // radius of the hexagon

    if (l & 0x1) p = p.yx;

    hc = hex_ortho_to_cube(p, r); // position converted to the cube coord
    float3 hi = hex_round(hc); // hexagon coordinate at the 2d position

    return hi;
}

float2 eval_sd_d_hex_from_ortho(float2 p, int l, int3 p0 = int3(0,0,0))
{
    float3 hc;
    float3 hi = eval_hex_from_ortho(p, l, hc);

    int d = hex_dist(hi, p0); // distance in hexagons to p0

    float3 hl = (hc - hi);  // local coord aka cube coord offset at the hexagon center
    float3 hil = hex_pointy_to_flat(hl); // local coordinate aligned inside the hexagon
    float s_id = hex_signed_distance(hil); // get signed distance to the hexagon

    return float2(s_id, d);
}

float4 main_hexamap(PixelShaderInput IN) : SV_Target
{
    float2 p = IN.coords.xy; // position in 2d
    float2 p0 = IN.coords.zw; // position of eye in 2d
  
    int l = 0;
    int l_step = 2;
    l = 0 + l_step * l;

    float3 hp0c;
    float3 hp0 = eval_hex_from_ortho(p0, l, hp0c);
    float3 hp1 = eval_hex_from_ortho(p0, l + l_step, hp0c);

    float2 sd0 = eval_sd_d_hex_from_ortho(p, l - l_step);
    float2 sd1 = eval_sd_d_hex_from_ortho(p, l, hp0);
    float2 sd2 = eval_sd_d_hex_from_ortho(p, l + l_step);


    float3 color = color_rainbow(sd1.y, 6.0);

    float a0 = clamp(1.0 - abs(sd0.x) * 30.0, 0, 1);
    float a1 = clamp(1.0 - abs(sd1.x) * 20.0, 0, 1);
    float a2 = clamp(1.0 - abs(sd2.x) * 20.0, 0, 1);

    color = lerp(color, 0.9, a0);
    color = lerp(color, 0.95, a1);
    color = lerp(color, 0.9, a2);

    float eye_sd = clamp(1.0 - distance(p, p0), 0, 1);
    color = lerp(color, 1.0, eye_sd * eye_sd * eye_sd * eye_sd);

   // color = lerp(color, float3(0.2, 1.0, 0.2), a2 * (hi.x == 0) * (hi.y == 0) * (hi.z == 0));
    
    
    return float4(color, 1.0);
}