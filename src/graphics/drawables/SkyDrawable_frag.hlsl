#include "Sky_inc.hlsl"
#include "Color_inc.hlsl"
#include "Camera_inc.hlsl"

Texture2D sky_map : register(t0);
Texture2D diffuse_sky_map : register(t1);

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
                color = sky_fetchEnvironmentMap(dome_dir, diffuse_sky_map, uSampler0[0], uSampler0[1]);
            }
        }
    }
    
    // return float4(HDR(color), 1.0);
    return float4((color), 1.0);

}




// Read previous state from 
RWTexture2D<float4> out_buffer : register(u0);
#define THREAD_GROUP_SIDE 4

[numthreads(THREAD_GROUP_SIDE, THREAD_GROUP_SIDE, 1)]
void main_makeSkymap(uint3 DTid : SV_DispatchThreadID)
{
    int2 pixelCoord = DTid.xy;

    float2 mapSize;
    out_buffer.GetDimensions(mapSize.x, mapSize.y);
    float2 invMapSize = rcp(mapSize);
   

    float3 dir = sky_dirFromTexcoord((pixelCoord + 0.5) * invMapSize);
    
    float3 color = SkyColor(dir);

        
    out_buffer[pixelCoord] = float4(color, 1);
}

float3 sampleHemisphere(float u1, float u2) {
    float cosTheta = 1 - u1;
    float sinTheta = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta));
    float phi = 2 * 3.14 * u2;
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

float3 sampleHemisphereSequence(int i, const int count) {
    float z = i * rcp(float(count));
    float a = i + 1 - z;
    return sampleHemisphere(z, a);
}


[numthreads(THREAD_GROUP_SIDE, THREAD_GROUP_SIDE, 1)]
void main_makeDiffuseSkymap(uint3 DTid : SV_DispatchThreadID)
{
    int2 pixelCoord = DTid.xy;

    float2 mapSize;
    out_buffer.GetDimensions(mapSize.x, mapSize.y);
    float2 invMapSize = rcp(mapSize);

    float3 dir = normalize(sky_dirFromTexcoord((pixelCoord + 0.5) * invMapSize));

    // Make an orthonormal base from direction
    float3 dirX;
    float3 dirY;
    transform_evalOrthonormalBase(dir, dirX, dirY);

    float3 diffuse = 0;

    int num = _simDims.z;
    for (int i = 0; i < num; i++) {
        float3 sampleDirHemi = sampleHemisphereSequence(i, num);
        
        float3 sampleDir = transform_rotateFrom(dirX, dirY, dir, sampleDirHemi);

        float2 texcoord = sky_texcoordFromDir(sampleDir);
        diffuse += sky_map.SampleLevel(uSampler0[0], texcoord, 0).xyz;
    }
    diffuse /= float(num);
   // diffuse *= 4.0;
    diffuse *= 3.14;
    out_buffer[pixelCoord] = float4(diffuse, 1);
}
