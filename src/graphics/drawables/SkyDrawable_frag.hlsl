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

    float3 color;


    float2 mapSize;
    sky_map.GetDimensions(mapSize.x, mapSize.y);
    float2 texelSize = rcp(mapSize);


    float aspectratio = _viewport.z / _viewport.w;

  //  if (IN.coords.z > 0.5)
    {
      //  float2 v_uv = (IN.coords.zw - float2(0.55, 0.05)) / 0.4;
        float2 v_uv = (IN.coords.zw - float2(0.7, 0.0)) / (0.3 * float2(1.0, aspectratio));
        
        if (v_uv.x < 0 || v_uv.y < 0 || v_uv.x >= 1 || v_uv.y >= 1)
        {
            float2 base = octahedron_uvFromDir(dir);
            float2 texcoord = octahedron_offsetCoord(base, 0);
            //float2 texcoord = sky_texcoordFromDir(dir);
 
            if ( (1.0 - abs(base.x) < texelSize.x) || (1.0 - abs(base.y) < texelSize.y) )
            {
                color = sky_map.SampleLevel(uSampler0[0], octahedron_offsetCoord(base, texelSize), 0).xyz;
                color += sky_map.SampleLevel(uSampler0[0], octahedron_offsetCoord(base, float2(texelSize.x, -texelSize.y)), 0).xyz;
                color += sky_map.SampleLevel(uSampler0[0], octahedron_offsetCoord(base, float2(-texelSize.x, texelSize.y)), 0).xyz;
                color += sky_map.SampleLevel(uSampler0[0], octahedron_offsetCoord(base, float2(-texelSize.x, -texelSize.y)), 0).xyz;
                color *= 0.25;
            }
            else 
            {
               color = sky_map.SampleLevel(uSampler0[1], texcoord, 0).xyz;
              // color = diffuse_sky_map.SampleLevel(uSampler0[1], texcoord, 0).xyz;
            }
        }
        else
        {
            if (v_uv.x > 0.5)
                color = sky_map.SampleLevel(uSampler0[0], v_uv, 0).xyz;
            else
                color = diffuse_sky_map.SampleLevel(uSampler0[0], v_uv, 0).xyz;
        }
    }

    
  //  return float4(HDR(color), 1.0);
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
    float a = frac(float(i) * rcp( 1 + (float(count))) );
    return sampleHemisphere(z, a);
}

[numthreads(THREAD_GROUP_SIDE, THREAD_GROUP_SIDE, 1)]
void main_makeDiffuseSkymap(uint3 DTid : SV_DispatchThreadID)
{
    int2 pixelCoord = DTid.xy;

    float2 mapSize;
    out_buffer.GetDimensions(mapSize.x, mapSize.y);
    float2 invMapSize = rcp(mapSize);

    float3 dir = sky_dirFromTexcoord((pixelCoord + 0.5) * invMapSize);

   // float3 dirX = normalize(cross(float3(0, 1, 0), dir));
    float3 dirX = float3(+dir.z, dir.y, -dir.x);
    float3 dirY = cross(dir, dirX);
    float3 difffuse = 0;

    const int num = 1000;
    for (int i = 0; i < num; i++) {
        float3 sampleDirHemi = sampleHemisphereSequence(i, num);

        float3 sampleDir = transform_rotateFrom(dirX, dirY, dir, sampleDirHemi);

        float2 texcoord = sky_texcoordFromDir(sampleDir);
        difffuse += sky_map.SampleLevel(uSampler0[0], texcoord, 0).xyz;
    }
    difffuse /= float(num);
    out_buffer[pixelCoord] = float4(difffuse, 1);
}
