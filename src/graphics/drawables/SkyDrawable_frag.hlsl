#include "Sky_inc.hlsl"
#include "Color_inc.hlsl"

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

float4 main(PixelShaderInput IN) : SV_Target
{
    float3 dir = normalize(IN.wdir);

 //   float3 color = color_rgbFromDir(dir);

    float3 color;// = SkyColor(dir);


    float2 mapSize;
    sky_map.GetDimensions(mapSize.x, mapSize.y);
    float2 texelSize = rcp(mapSize);


  //  if (IN.coords.z > 0.5)
    {
        float2 v_uv = (IN.coords.zw - float2(0.55, 0.05)) / 0.4;
     //   float2 v_uv = (IN.coords.zw - float2(0.5, 0.0)) / 0.5;  
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
            }
        }
        else
        {
   color = sky_map.SampleLevel(uSampler0[0], v_uv, 0).xyz;
           
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