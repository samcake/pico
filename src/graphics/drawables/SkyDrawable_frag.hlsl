#include "Sky_inc.hlsl"
#include "Color_inc.hlsl"
#include "Camera_inc.hlsl"

Texture2D sky_map : register(t0);
Texture2D diffuse_sky_map : register(t1);

SamplerState uSampler0[2] : register(s0);

float3 evalSH(float3 dir)
{
    float3 d = dir.zxy;

  //------------------------------------------------------------------       
  // We now define the constants and assign values to x,y, and z 

    
    float3 L00 = diffuse_sky_map.Load(int3(0, 0, 0)).xyz;
    float3 L1_1 = diffuse_sky_map.Load(int3(1, 0, 0)).xyz;
    float3 L10 = diffuse_sky_map.Load(int3(2, 0, 0)).xyz;
    float3 L11 = diffuse_sky_map.Load(int3(3, 0, 0)).xyz;
    float3 L2_2 = diffuse_sky_map.Load(int3(4, 0, 0)).xyz;
    float3 L2_1 = diffuse_sky_map.Load(int3(5, 0, 0)).xyz;
    float3 L20 = diffuse_sky_map.Load(int3(6, 0, 0)).xyz;
    float3 L21 = diffuse_sky_map.Load(int3(7, 0, 0)).xyz;
    float3 L22 = diffuse_sky_map.Load(int3(8, 0, 0)).xyz;
    
  //------------------------------------------------------------------ 
  // We now compute the squares and products needed 
    /* x2 = x * x;
    y2 = y * y;
    z2 = z * z;
    xy = x * y;
    yz = y * z;
    xz = x * z;*/
	
    const float c1 = 0.429043;
    const float c2 = 0.511664;
    const float c3 = 0.743125;
    const float c4 = 0.886227;
    const float c5 = 0.247708;
    
    //------------------------------------------------------------------ 
  // Finally, we compute equation 13
    
    float3 col = c1 * L22 * (d.x * d.x - d.y * d.y)
            + c3 * L20 * d.z * d.z
            + c4 * L00
            - c5 * L20
            + 2 * c1 * (  L2_2 * d.x * d.y
                        + L21  * d.x * d.z
                        + L2_1 * d.y * d.z)
            + 2 * c2 * (  L11  * d.x
                        + L1_1 * d.y
                        + L10  * d.z);

    return col;
}


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

    
    
    { // Scopes
        float aspectratio = _viewport.z / _viewport.w;
        float2 v_uv1 = (IN.coords.zw - float2(0.7, 0.0)) / (0.3 * float2(1.0, aspectratio));
        float2 v_uv2 = (IN.coords.zw - float2(0.0, 0.0)) / (0.3 * float2(1.0, aspectratio));
        float2 v_uv3 = (IN.coords.zw - float2(0.0, 0.3 * aspectratio)) / (0.3 * float2(1.0, aspectratio));
        if (all(abs(v_uv1 - 0.5) < 0.5))
        {
            if (v_uv1.x > 0.5)
                color = sky_map.SampleLevel(uSampler0[0], v_uv1, 0).xyz;
            else
                color = diffuse_sky_map.SampleLevel(uSampler0[0], v_uv1, 0).xyz;       
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
             //   color = sky_fetchEnvironmentMap(dome_dir, diffuse_sky_map, uSampler0[0], uSampler0[1]);
                color = evalSH(dome_dir);
            }
        }
    }
    
    // return float4(HDR(color), 1.0);
    return float4((color), 1.0);

}


