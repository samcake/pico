#include "Sky_inc.hlsl"

Texture2D sky_map : register(t0);
SamplerState uSampler0[2] : register(s0);


void updatecoeffs(float3 hdr, float domega, float3 d, inout float3 coeffs[9])
{
    float3 dir = d.zxy;
    /******************************************************************
     Update the coefficients (i.e. compute the next term in the
     integral) based on the lighting value hdr, the differential
     solid angle domega and cartesian components of surface normal x,y,z

     Inputs:  hdr = L(x,y,z) [note that x^2+y^2+z^2 = 1]
              i.e. the illumination at position (x,y,z)

              domega = The solid angle at the pixel corresponding to
          (x,y,z).  For these light probes, this is given by

          x,y,z  = Cartesian components of surface normal

     Notes:   Of course, there are better numerical methods to do
              integration, but this naive approach is sufficient for our
          purpose.

    *********************************************************************/

    /* L_{00}.  Note that Y_{00} = 0.282095 */
    const float c0 = 0.282095;
    coeffs[0] = hdr * c0 * domega;

    /* L_{1m}. -1 <= m <= 1.  The linear terms */
    const float c1 = 0.488603;
    coeffs[1] = hdr * (c1 * dir.y) * domega; /* Y_{1-1} = 0.488603 y  */
    coeffs[2] = hdr * (c1 * dir.z) * domega; /* Y_{10}  = 0.488603 z  */
    coeffs[3] = hdr * (c1 * dir.x) * domega; /* Y_{11}  = 0.488603 x  */

    /* The Quadratic terms, L_{2m} -2 <= m <= 2 */

    /* First, L_{2-2}, L_{2-1}, L_{21} corresponding to xy,yz,xz */
    const float c2 = 1.092548;
    coeffs[4] = hdr * (c2 * dir.x * dir.y) * domega; /* Y_{2-2} = 1.092548 xy */
    coeffs[5] = hdr * (c2 * dir.y * dir.z) * domega; /* Y_{2-1} = 1.092548 yz */
    coeffs[7] = hdr * (c2 * dir.x * dir.z) * domega; /* Y_{21}  = 1.092548 xz */

    /* L_{20}.  Note that Y_{20} = 0.315392 (3z^2 - 1) */
    const float c3 = 0.315392;
    coeffs[6] = hdr * (c3 * (3 * dir.z * dir.z - 1)) * domega;

    /* L_{22}.  Note that Y_{22} = 0.546274 (x^2 - y^2) */
    const float c4 = 0.546274;
    coeffs[8] = hdr * (c4 * (dir.x * dir.x - dir.y * dir.y)) * domega;
}

// Read previous state from 
RWTexture2D<float4> out_texture : register(u0);
RWBuffer<float4> out_buffer : register(u1);

#define THREAD_GROUP_SIDE 8
#define THREAD_GROUP_DIM THREAD_GROUP_SIDE * THREAD_GROUP_SIDE
#define SH_DIM 9

[numthreads(THREAD_GROUP_SIDE, THREAD_GROUP_SIDE, 1)]
void main_makeSkymap(uint3 DTid : SV_DispatchThreadID)
{
    int2 pixelCoord = DTid.xy;

    float2 mapSize;
    out_texture.GetDimensions(mapSize.x, mapSize.y);
    float2 invMapSize = rcp(mapSize);
   

    float3 dir = sky_dirFromTexcoord((pixelCoord + 0.5) * invMapSize);
    
    float3 color = SkyColor(dir);

        
    out_texture[pixelCoord] = float4(color, 1);
}

//////////////////////////////////// 
//If I use a groupshared
groupshared float3 group_coeffs[THREAD_GROUP_DIM * SH_DIM];

void sumUpSHInGroupSharedMem(in uint groupThreadIndex)
{
    // Sum SH contributions for the group in shared mem
    for (uint k = THREAD_GROUP_DIM / 2; k > 0; k >>= 1)
    {
        if (groupThreadIndex < k)
        {
            int destGroupIndex = groupThreadIndex * SH_DIM;
            int foldGroupIndex = (groupThreadIndex + k) * SH_DIM;
            
            for (int i = 0; i < SH_DIM; ++i)
                group_coeffs[destGroupIndex + i] += group_coeffs[foldGroupIndex + i];
        }
        GroupMemoryBarrierWithGroupSync();
    }
}

[numthreads(THREAD_GROUP_DIM, 1, 1)]
void main_makeDiffuseSkymap_first(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
    uint groupIndex = Gid.x;
    uint groupThreadIndex = GTid.x;
    uint2 groupThreadCoord = uint2(groupThreadIndex % THREAD_GROUP_SIDE, groupThreadIndex / THREAD_GROUP_SIDE);

    // Compute this pixel's contribution to SH 
    
    float2 mapSize = float2(_simDims.z, _simDims.z);
    uint2 pixelCoord = uint2(DTid.x / uint(mapSize.x), DTid.x % uint(mapSize.x));
    float2 invMapSize = rcp(mapSize);
    float2 texcoord = (pixelCoord + 0.5) * invMapSize;
    float3 dir = sky_dirFromTexcoord(texcoord);
    
    float3 light = sky_map.SampleLevel(uSampler0[0], texcoord, 0).xyz;
    
    float solidangle = 2 * 3.14 * invMapSize.x * invMapSize.y;
    // float solidangle = 1.0;
    
    float3 coeffs[SH_DIM] = {
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 }
    };
    updatecoeffs(light, solidangle, dir, coeffs);
    

    // Store SH in shared mem for the group
    for (int i = 0; i < 9; ++i)
        group_coeffs[SH_DIM * groupThreadIndex + i] = coeffs[i];
    GroupMemoryBarrierWithGroupSync();
    
    sumUpSHInGroupSharedMem(groupThreadIndex);
    
    // Write group SH in out_buffer at corresponding groupIndex
    if (groupThreadIndex < SH_DIM) // <=> for (int i = 0; i < SH_DIM; ++i)
        out_buffer[SH_DIM * groupIndex + groupThreadIndex] = float4(group_coeffs[groupThreadIndex], 0);
}


[numthreads(THREAD_GROUP_DIM, 1, 1)]
void main_makeDiffuseSkymap_next(uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
    uint groupIndex = Gid.x;
    uint groupThreadIndex = GTid.x; 

    DeviceMemoryBarrier();
  
    // Store SH in shared mem for the group
    for (int i = 0; i < SH_DIM; ++i)
        group_coeffs[SH_DIM * groupThreadIndex + i] = out_buffer[SH_DIM * (groupIndex * THREAD_GROUP_DIM + groupThreadIndex) + i];
    GroupMemoryBarrierWithGroupSync();
    
    sumUpSHInGroupSharedMem(groupThreadIndex);
    
    // Write group SH in out_buffer at corresponding groupIndex
    if (groupThreadIndex < SH_DIM) // <=> for (int i = 0; i < SH_DIM; ++i)
        out_buffer[SH_DIM * groupIndex + groupThreadIndex] = float4(group_coeffs[groupThreadIndex], 0);
  
    GroupMemoryBarrierWithGroupSync();
}