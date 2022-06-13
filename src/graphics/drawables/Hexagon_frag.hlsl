#include "Color_inc.hlsl"
#include "Hexagon_inc.hlsl"
#include "SceneTransform_inc.hlsl"

struct PixelShaderInput
{
    float4 coords : TEXCOORD;
    float3 eyePos : EPOS;
};

static const float hex_size_0 = 10.0;

float eval_hex_level_size(int l)
{
    float scale = pow(INV_SQRT_3, float(l));
    float r = hex_size_0 * scale; // radius of the hexagon
    return r;
}

float3 eval_hex_from_ortho(float2 p, int l, out float3 hc)
{
    float r = eval_hex_level_size(l); // radius of the hexagon

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
  
    float eyeDepth = IN.eyePos.z;
    
 //   float pixelSize = cam_pixelSizeAt(_view.col_w().y);
    float pixelSize = cam_pixelSizeAt(eyeDepth);
    float eye_scale = 300.0 * pixelSize;
   // float pixelSize = cam_pixelSizeAt(1.0);
    
    int l_step = 2;
    int l = 0;
    float l_frac = 0.0;
    float l_scale = eval_hex_level_size(l);
    
    for (int i = 1; i < 15; i++) {
        float scale = eval_hex_level_size(l);
        if (eye_scale > scale)
        {
            l_frac = (eye_scale - scale) / (l_scale - scale);
            break;
        }
        l_scale = scale;
        l_frac = 1.0;
        l += l_step;
    }

    int l0 = l - l_step;
    int l1 = l;
    int l2 = l + l_step;
    
    float3 hp0c;
    float3 hp0 = eval_hex_from_ortho(p0, l0, hp0c);
    float3 hp1 = eval_hex_from_ortho(p0, l1, hp0c);
    float3 hp2 = eval_hex_from_ortho(p0, l2, hp0c);

    float2 sd0 = eval_sd_d_hex_from_ortho(p, l0, hp0);
    float2 sd1 = eval_sd_d_hex_from_ortho(p, l1, hp1);
    float2 sd2 = eval_sd_d_hex_from_ortho(p, l2, hp2);


    //float3 color = color_rainbow(sd1.y, 6.0);
   // float3 color = frac(sd1.y / 6.0);
    float3 color = 0.5;

    float a0 = step(0.2, clamp(1.0 - abs(sd0.x) * 30.0, 0, 1));
    float a1 = step(0.2, clamp(1.0 - abs(sd1.x) * 30.0, 0, 1));
    float a2 = step(0.2, clamp(1.0 - abs(sd2.x) * 30.0, 0, 1));

    
    float3 gridColor0 = color_rainbow(l0 / float(l_step), 6.0);
    float3 gridColor1 = color_rainbow(l1 / float(l_step), 6.0);
    float3 gridColor2 = color_rainbow(l2 / float(l_step), 6.0);
    
    color = lerp(color, gridColor0, a0 * l_frac);
    color = lerp(color, gridColor1, a1);
    color = lerp(color, gridColor2, a2 * (1 - l_frac));

    float eye_sd = clamp(1.0 - distance(p, p0), 0, 1);
   // color = lerp(color, 1.0, eye_sd * eye_sd * eye_sd * eye_sd);
    //float eye_scale = pow(INV_SQRT_3, float(l * 2));

    color = lerp(color, 1.0, (eye_scale - distance(p, p0)) > 0);
    
    color = lerp(color, float3( 0.7, 0.7, 1.0), a0 * 5.0 * (sd0.y == 0));
    color = lerp(color, float3( 0.7, 0.7, 1.0), a1 * 5.0 * (sd1.y == 0));
    color = lerp(color, float3(0.7, 0.7, 1.0), a2 * 5.0 * (sd2.y == 0));
    
    
    return float4(color, 1.0);
}