#include "Sky_inc.hlsl"
#include "Color_inc.hlsl"

Texture2D sky_map : register(t0);

SamplerState uSampler0[2] : register(s0);

#define SKYMAP_SIDE 2048


// octahedron mapping
// 


// Octahedron wrap the uv in range [-1,1] from up hemisphere to bottom hemisphere (and back?)
float2 octahedron_uvWrap(float2 uv) {
    return (1.0 - abs(uv.yx)) * (uv.xy >= 0.0 ? 1.0 : -1.0);
}

// Octahedron convert from dir normalized to uv in range [-1,1]
float2 octahedron_uvFromDir(float3 dir) {
    // REF https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
    dir /= (abs(dir.x) + abs(dir.y) + abs(dir.z));
    float2 uv = dir.y >= 0.0 ? dir.zx : octahedron_uvWrap(dir.zx);
    return uv;
}

// Octahedron convert from  uv in range [-1,1] to dir normalized
float3 octahedron_dirFromUv(float2 uv)
{
    // REF https://twitter.com/Stubbesaurus/status/937994790553227264
    float3 dir = float3(uv.y, 1.0 - abs(uv.x) - abs(uv.y), uv.x);
    float t = max(-dir.y, 0.0);
    dir.xz += (dir.xz >= 0.0 ? -t : t);
    return normalize(dir);
}

float3 sky_dirFromTexcoord(float2 tc) {
    return octahedron_dirFromUv(2 * tc - 1.0);
}

float2 sky_texcoordFromDir(float3 dir) {
    return (octahedron_uvFromDir(dir) * 0.5 + 0.5);
}


// Check for offset over texture edge,
bool OctFlipped(float2 r)
{
    float t = max(abs(r.x), abs(r.y));
    return t >= 1.0;
}

bool OctFlipped2(float2 c, in out bool2 r)
{
    r.x = abs(c.x) >= 1.0;
    r.y = abs(c.y) >= 1.0;
    return r.x || r.y;
}

// Example of computing mirrored repeat sampling 
// of an octahedron map with a small texel offset.
// Note this is not designed to solve the double wrap case.
// The "base" is as computed by Oct3To2() above.

float2 offsetOctCoord(float2 base, float2 offset)
{
    float2 coord = base + offset; // 2 VALU

   // coord = OctFlipped(coord) ? -coord : coord; // 4 VALU
    bool2 r;
    if (OctFlipped2(coord, r))
    {
        if (r.x && r.y)
        {
            coord = -coord;
        }
        else if (r.x)
        {
            coord = float2((coord.x >= 0 ? 1 : -1) * (2 - abs(coord.x)), -coord.y);
        }
        else if (r.y)
        {
            coord = float2(-coord.x, (coord.y >= 0 ? 1 : -1) * (2 - abs(coord.y)));
        }
    }

    coord = coord * 0.5 + 0.5; // 2 VALU
    return coord;
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

float4 main(PixelShaderInput IN) : SV_Target
{
    float3 dir = normalize(IN.wdir);

 //   float3 color = color_rgbFromDir(dir);

    float3 color = SkyColor(dir);

    float2 texcoord = sky_texcoordFromDir(dir);

    float2 mapSize;
    sky_map.GetDimensions(mapSize.x, mapSize.y);
    float2 texelSize = rcp(mapSize);

    if (IN.coords.z > 0.5)
    {
        float2 v_uv = (IN.coords.zw - float2(0.55, 0.05)) / 0.4;
        
        if (v_uv.x < 0 || v_uv.y < 0 || v_uv.x >= 1 || v_uv.y >= 1)
        {
            float2 base = octahedron_uvFromDir(dir);
 
            if (base.x < (texelSize.x - 1.0) || base.x > 1.0 - texelSize.x || base.y < (texelSize.y - 1.0) || base.y > 1.0 - texelSize.y)
            {
                texcoord = offsetOctCoord(base, 0);
                color = sky_map.SampleLevel(uSampler0[0], offsetOctCoord(base, 0), texelSize).xyz;
                color += sky_map.SampleLevel(uSampler0[0], offsetOctCoord(base, 0), float2(texelSize.x, -texelSize.y)).xyz;
                color += sky_map.SampleLevel(uSampler0[0], offsetOctCoord(base, 0), float2(-texelSize.x, texelSize.y)).xyz;
                color += sky_map.SampleLevel(uSampler0[0], offsetOctCoord(base, 0), float2(-texelSize.x, -texelSize.y)).xyz;

              //  color = float3(abs(base), 0);
                color *= 0.25;

            }
            else
            {
                texcoord = offsetOctCoord(base, 0);
                color = sky_map.SampleLevel(uSampler0[1], texcoord, 0).xyz;
            }
        }
        else
        {
            color = sky_map.SampleLevel(uSampler0[1], v_uv, 0).xyz;
        }
    }

    
    return float4(HDR(color), 1.0);

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