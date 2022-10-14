#include "Sky_inc.hlsl"
#include "Color_inc.hlsl"
#include "Camera_inc.hlsl"

Texture2D sky_map : register(t0);
SamplerState uSampler0[2] : register(s0);


struct PixelShaderInput
{
    float4 coords : TEXCOORD;
    float3 wdir : WPOS;
};

float3 HDR(float3 color)
{
    return 1.0f - exp(-2.0 * color);
}

float4 main_draw(PixelShaderInput IN) : SV_Target
{
    float3 dir = normalize(IN.wdir);

    float3 color = sky_fetchEnvironmentMap(dir, sky_map, uSampler0[0], uSampler0[1]);
    Transform _view = cam_view();
    float4 _viewport = cam_viewport();
    
    if (_drawControl.x == 1)
    { // Scopes
        float aspectratio = _viewport.z / _viewport.w;
        float2 v_uv1 = (IN.coords.zw - float2(0.8, 0.0)) / (0.2 * float2(1.0, aspectratio));
        float2 v_uv2 = (IN.coords.zw - float2(0.0, 0.0)) / (0.3 * float2(1.0, aspectratio));
        float2 v_uv3 = (IN.coords.zw - float2(0.0, 0.3 * aspectratio)) / (0.3 * float2(1.0, aspectratio));
        if (all(abs(v_uv1 - 0.5) < 0.5))
        {
            if (v_uv1.x > 0.5)
                color = sky_map.SampleLevel(uSampler0[0], v_uv1, 0).xyz;
            else
            {
                color = 0.2 * sky_evalIrradianceSH(sky_dirFromTexcoord(v_uv1));
            }
        }
        else if (all(abs(v_uv2 - 0.5) < 0.5))
        {
            v_uv2 = 2.2 * (v_uv2 - 0.5);
            float r = dot(v_uv2, v_uv2);
            if (r <= 1.0f)
            {
                float3 dome_dir = float3(v_uv2.x, v_uv2.y, sqrt(1 - r));
                dome_dir = worldFromEyeSpaceDir(_view, dome_dir);
                color = sky_fetchEnvironmentMap(dome_dir, sky_map, uSampler0[0], uSampler0[1]);
            }
        }
        else if (all(abs(v_uv3 - 0.5) < 0.5))
        {
            v_uv3 = 2.2 * (v_uv3 - 0.5);
            float r = dot(v_uv3, v_uv3);
            if (r <= 1.0f)
            {
                float3 dome_dir = float3(v_uv3.x, v_uv3.y, sqrt(1 - r));
                dome_dir = worldFromEyeSpaceDir(_view, dome_dir);
                color = 0.2 * sky_evalIrradianceSH(dome_dir);
            }
        }
    }
    
    // return float4(HDR(color), 1.0);
    return float4((color), 1.0);

}


