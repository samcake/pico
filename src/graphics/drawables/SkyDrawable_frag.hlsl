#include "Sky_inc.hlsl"
#include "Color_inc.hlsl"
#include "Camera_inc.hlsl"

Texture2D sky_map : register(t0);
Texture2D diffuse_sky_map : register(t1);

SamplerState uSampler0[2] : register(s0);

float3 evalSH(float3 dir)
{
    float3 d = dir.xyz;

  //------------------------------------------------------------------       
  // We now define the constants and assign values to x,y, and z 
	
    const float c1 = 0.429043;
    const float c2 = 0.511664;
    const float c3 = 0.743125;
    const float c4 = 0.886227;
    const float c5 = 0.247708;
    
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

    float3 d2 = d * d;
   /* x2 = x * x;
    y2 = y * y;
    z2 = z * z;
    xy = x * y;
    yz = y * z;
    xz = x * z;*/
  //------------------------------------------------------------------ 
  // Finally, we compute equation 13
    
    float3 col = c1 * L22 * (d2.x - d2.y)
            + c3 * L20 * d2.z
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



void updatecoeffs(float3 hdr, float domega, float3 d, inout float3 coeffs[9])
{
    float3 dir = d.xyz;
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
    coeffs[0] += hdr * c0 * domega;

    /* L_{1m}. -1 <= m <= 1.  The linear terms */
    const float c1 = 0.488603;
    coeffs[1] += hdr * (c1 * dir.y) * domega; /* Y_{1-1} = 0.488603 y  */
    coeffs[2] += hdr * (c1 * dir.z) * domega; /* Y_{10}  = 0.488603 z  */
    coeffs[3] += hdr * (c1 * dir.x) * domega; /* Y_{11}  = 0.488603 x  */

    /* The Quadratic terms, L_{2m} -2 <= m <= 2 */

    /* First, L_{2-2}, L_{2-1}, L_{21} corresponding to xy,yz,xz */
    const float c2 = 1.092548;
    coeffs[4] += hdr * (c2 * dir.x * dir.y) * domega; /* Y_{2-2} = 1.092548 xy */
    coeffs[5] += hdr * (c2 * dir.y * dir.z) * domega; /* Y_{2-1} = 1.092548 yz */
    coeffs[7] += hdr * (c2 * dir.x * dir.z) * domega; /* Y_{21}  = 1.092548 xz */

    /* L_{20}.  Note that Y_{20} = 0.315392 (3z^2 - 1) */
    const float c3 = 0.315392;
    coeffs[6] += hdr * (c3 * (3 * dir.z * dir.z - 1)) * domega;

    /* L_{22}.  Note that Y_{22} = 0.546274 (x^2 - y^2) */
    const float c4 = 0.546274;
    coeffs[8] += hdr * (c4 * (dir.x * dir.x - dir.y * dir.y)) * domega;
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
void main_makeDiffuseSkymap_old
(uint3 DTid : SV_DispatchThreadID)
{
    int2 pixelCoord = DTid.xy;

    float2 mapSize;
    out_buffer.GetDimensions(mapSize.x, mapSize.y);
    float2 invMapSize = rcp(mapSize);

    float3 dir = (sky_dirFromTexcoord((pixelCoord + 0.5) * invMapSize));

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


[numthreads(THREAD_GROUP_SIDE, THREAD_GROUP_SIDE, 1)]
void main_makeDiffuseSkymap(uint3 DTid : SV_DispatchThreadID)
{
    int2 pixelCoord = DTid.xy;

    out_buffer[pixelCoord] = float4(0, 0, 0, 1);
    AllMemoryBarrier();
    
    float2 mapSize;
    out_buffer.GetDimensions(mapSize.x, mapSize.y);
    float2 invMapSize = rcp(mapSize);
    float2 texcoord = (pixelCoord + 0.5) * invMapSize;
    float3 dir = sky_dirFromTexcoord(texcoord);
    float3 light = sky_map.SampleLevel(uSampler0[0], texcoord, 0).xyz;
    
    float solidangle = 2 * 3.14 * invMapSize * 3.14 * 2.0 ;
    
    float3 coeffs[9] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
                         { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
                         { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
    
    updatecoeffs(light, solidangle, dir, coeffs);
    
    for (int i = 0; i < 9; ++i)
        out_buffer[int2(i, 0)] += float4(coeffs[i], 1);
    
    
    if (pixelCoord.y > 0)
        out_buffer[pixelCoord] = float4(coeffs[3], 1);
}