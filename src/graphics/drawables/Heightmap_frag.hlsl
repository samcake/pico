#include "Camera_inc.hlsl"


float3 getLightDir() {
    // the direction TO the light
    return -normalize(float3(1.f, -1.f, 0.f));
}

struct PixelShaderInput
{
    float4 Color    : COLOR;
    float3 WPos     : WPOS;
    float3 Normal   : SPRITE;
};

float3 SkyColor(const float3 dir) {
    float3 HORIZON_COLOR = float3(1.71, 1.31, 0.83);
    float3 SKY_COLOR = float3(0.4, 0.75, 1);
    float3 SUN_COLOR = float3(1.5, 1, 0.6);
    float3 SUN_DIRECTION = getLightDir();

    float mixWeight = sqrt(abs(dir.z));
    float3 sky = (1 - mixWeight) * HORIZON_COLOR + mixWeight * SKY_COLOR;
    float3 sun = 4 * SUN_COLOR * pow(max(dot(SUN_DIRECTION, dir), 0), 1500);
    return sky + sun;
}

/*
* This is done in eval shading specular diffuse
float3 colorDiffuse(float3 base_color, float metallic) {
    const float3 dielectricSpecular = float3(0.04, 0.04, 0.04);
    const float3 black = float3(0, 0, 0);
    float3 cdiff = lerp(base_color.rgb * (1 - dielectricSpecular.r), black, metallic);
    return cdiff;
}
*/

float3 colorF0(float3 base_color, float metallic) {
    const float3 dielectricSpecular = float3(0.04, 0.04, 0.04);
    float3 cdiff = lerp(dielectricSpecular, base_color.rgb, metallic);
    return cdiff;
}

float3 fresnel(float v_dot_h, float3 f0) {
    float base = (1.0 - v_dot_h);
    return f0 + (float3(1.0, 1.0, 1.0) - f0) * (base *base * base * base * base);
}

float evalSmithInvG1(float roughness4, float ndotd) {
    return ndotd + sqrt(roughness4 + ndotd * ndotd * (1.0 - roughness4));
}

float eval_GeometricOcclusion_NormalDistribution(float n_dot_l, float n_dot_v, float n_dot_h, float roughness2) {
    // See https://www.khronos.org/assets/uploads/developers/library/2017-web3d/glTF-2.0-Launch_Jun17.pdf
    // for details of equations, especially page 20
    float roughness4 = roughness2 * roughness2;

    // Distribution GGX
    float denom = (n_dot_h * n_dot_h * (roughness2 - 1.0) + 1.0);
    denom *= denom;

    // Geometric Occlusion factors G1(n,l) and G1(n,v)
    float smithInvG1NdotV = evalSmithInvG1(roughness4, n_dot_v);
    float smithInvG1NdotL = evalSmithInvG1(roughness4, n_dot_l);
    denom *= smithInvG1NdotV * smithInvG1NdotL;
    // Don't divide by PI as this is part of the light normalization factor
    float power = roughness4 / denom;
    return power;
}


float4 evalShading(float l_dot_h, float v_dot_h, float n_dot_l, float n_dot_v, float n_dot_h, float roughness2, float metallic, float3 f0) {
    // Incident angle attenuation
    float angleAttenuation = n_dot_l;

    // Specular Lighting
    float3 fresnelColor = fresnel(l_dot_h, f0);
    float power = eval_GeometricOcclusion_NormalDistribution(n_dot_l, n_dot_v, n_dot_h, roughness2);
    float3 specular = fresnelColor * power * angleAttenuation;

    // Diffuse Lighting
    // assuming the fresnel(v.h) term is equal to fresnelColor.x
    float diffuse = (1.0 - metallic) * angleAttenuation * (1.0 - fresnelColor.x);

    // We don't divided by PI, as the "normalized" equations state we should, because we decide, as Naty Hoffman, that
    // we wish to have a similar color as raw albedo on a perfectly diffuse surface perpendicularly lit
    // by a white light of intensity 1. But this is an arbitrary normalization of what light intensity "means".
    // (see http://blog.selfshadow.com/publications/s2013-shading-course/hoffman/s2013_pbs_physics_math_notes.pdf
    // page 23 paragraph "Punctual light sources")
    return float4(specular, diffuse);
}

float4 main(PixelShaderInput IN) : SV_Target
{
    float3 L = getLightDir();
//    float3 N = normalize(IN.Normal);
    float3 N = normalize(cross(ddy(IN.WPos), ddx(IN.WPos)));


    float3 E2F = IN.WPos - cam_view().col_w();
    float E2Flen = length(E2F);
    float3 V = -normalize(E2F);
    float3 H = normalize(V + L);


    float3 diff_color = SkyColor(N);
    float3 spec_color = SkyColor(reflect(V, N));


    float metallic = 0.0;
    float roughness = 0.01;
    float roughness2 = roughness * roughness;
    float3 base_color = float3(1.0, 1.0, 1.0);
   // float3 base_color = float3(0.02, 0.02, 0.02); // water

    float3 F0 = colorF0(base_color, metallic);


    float n_dot_l = dot(N, L);
    float n_dot_h = dot(N, H);
    float n_dot_v = dot(N, V);
    float v_dot_h = dot(V, H);
    float l_dot_h = dot(L, H);

    float4 spec_diff_lighting = evalShading(l_dot_h, v_dot_h, n_dot_l, n_dot_h, n_dot_v, roughness2, metallic, F0);

    float3 color = spec_diff_lighting.w * base_color + spec_diff_lighting.xyz * 0.0;
//    color *= 0.8 * base_color;

//    float fog = (1.0f - max(0.f, pow(1 - E2Flen / 100.f, 1.2f)));
 //   color = fog * color;

    return float4(pow(color,  2.2), 1.0);
    return float4(N, 1.0);
}