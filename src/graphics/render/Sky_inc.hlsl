//
// Sky API
// 
// Implementation taken from
// https://www.scratchapixel.com/lessons/procedural-generation-virtual-worlds/simulating-sky
//
#ifndef SKY_INC
#define SKY_INC

struct Atmosphere {
    float4 er_ar_hr_hm;
    float4 betaR; // { 5.8e-6f, 13.5e-6f, 33.1e-6f };   // Rayleygh Scattering
    float4 betaM; // { 4e-6f };                        // Mie Scattering

    float earthRadius() { return er_ar_hr_hm.x; } // = 6360e3;                         // In the paper this is usually Rg or Re (radius ground, eart) 
    float atmosphereRadius() { return er_ar_hr_hm.y; } // = 6420e3;                    // In the paper this is usually R or Ra (radius atmosphere) 
    float Hr() { return er_ar_hr_hm.z; } // = 7994;                                    // Thickness of the atmosphere if density was uniform (Hr) 
    float Hm() { return er_ar_hr_hm.w; } // = 1200;                                    // Same as above but for Mie scattering (Hm) 
};


//float3 _SkyColor(const float3 dir) {
//    float3 HORIZON_COLOR = float3(1.71, 1.31, 0.83);
//    float3 SKY_COLOR = float3(0.4, 0.75, 1);
//    float3 SUN_COLOR = float3(1.5, 1, 0.6);
//    float3 SUN_DIRECTION = getLightDir();
//
//    float mixWeight = sqrt(abs(dir.z));
//    float3 sky = (1 - mixWeight) * HORIZON_COLOR + mixWeight * SKY_COLOR;
//    float3 sun = 4 * SUN_COLOR * pow(max(dot(SUN_DIRECTION, dir), 0), 1500);
//    return sky + sun;
//}

int raySphereIntersect(float3 orig, float3 dir, float sphereRadius, in out float t0, in out float t1) {

    float p = dot(dir, orig);
    float q = dot(orig, orig) - sphereRadius * sphereRadius;

    float discriminant = (p * p) - q;
    if (discriminant < 0.0f) {
        return 0;
    }

    float dRoot = sqrt(discriminant);
    t0 = -p - dRoot;
    t1 = -p + dRoot;

    return (discriminant > 1e-7) ? 2 : 1;
}

float3 sky_computeIncidentLight(int4 simDim, Atmosphere atmos, float3 sunDirection, float3 orig, float3 dir, float tmin, float tmax) {
    const float M_PI = acos(-1);
    float earthRadius = atmos.earthRadius();
    float atmosphereRadius = atmos.atmosphereRadius();          
    float Hr = atmos.Hr();   
    float Hm = atmos.Hm();
    float3 betaR = atmos.betaR.xyz;
    float3 betaM = atmos.betaM.xyz;

    // Ray intersect the atmosphere sphere
    float t0, t1;
    int numAtmosphereIntersections = raySphereIntersect(orig, dir, atmosphereRadius, t0, t1);
    // Ray above the atmosphere looking out in space => black
    if (!numAtmosphereIntersections || t1 < 0)
        return float3(0, 0, 0);
    // Ray start above atmosphere looking down
    if (t0 > tmin && t0 > 0)
        tmin = t0;
    // Ray end at atmosphere second intersection
    if (t1 < tmax || tmax < 0)
        tmax = t1;
    
    uint numSamples = simDim.x;
    uint numSamplesLight = simDim.y;
    float segmentLength = (tmax - tmin) / numSamples;
    float tCurrent = tmin;

    float3 sumR = (0);
    float3 sumM = (0); // mie and rayleigh contribution 
    float opticalDepthR = 0;
    float opticalDepthM = 0;

    float mu = dot(dir, sunDirection); // mu in the paper which is the cosine of the angle between the sun direction and the ray direction 
    float phaseR = 3.f / (16.f * M_PI) * (1 + mu * mu);
    float g = 0.76f;
    float phaseM = 3.f / (8.f * M_PI) * ((1.f - g * g) * (1.f + mu * mu)) / ((2.f + g * g) * pow(1.f + g * g - 2.f * g * mu, 1.5f));

    for (int i = 0; i < numSamples; ++i) {
        float3 samplePosition = orig + (tCurrent + segmentLength * 0.5f) * dir;
        float height = length(samplePosition) - earthRadius;

        // compute optical depth for light
        float hr = exp(-height / Hr) * segmentLength;
        float hm = exp(-height / Hm) * segmentLength;
        opticalDepthR += hr;
        opticalDepthM += hm;

        // light optical depth
        float t0Light, t1Light;
        raySphereIntersect(samplePosition, sunDirection, atmosphereRadius, t0Light, t1Light);
        float segmentLengthLight = t1Light / numSamplesLight;
        float tCurrentLight = 0;
        float opticalDepthLightR = 0;
        float opticalDepthLightM = 0;

        uint j;
        for (j = 0; j < numSamplesLight; ++j) {
            float3 samplePositionLight = samplePosition + (tCurrentLight + segmentLengthLight * 0.5f) * sunDirection;
            float heightLight = length(samplePositionLight) - earthRadius;
            if (heightLight < 0) break;
            opticalDepthLightR += exp(-heightLight / Hr) * segmentLengthLight;
            opticalDepthLightM += exp(-heightLight / Hm) * segmentLengthLight;
            tCurrentLight += segmentLengthLight;
        }
        if (j == numSamplesLight) {
            float3 tau = betaR * (opticalDepthR + opticalDepthLightR) + betaM * 1.1f * (opticalDepthM + opticalDepthLightM);
            float3 attenuation = float3(exp(-tau.x), exp(-tau.y), exp(-tau.z));
            sumR += attenuation * hr;
            sumM += attenuation * hm;
        }

        tCurrent += segmentLength;
    }

    // We use a magic number here for the intensity of the sun (20). We will make it more
    // scientific in a future revision of this lesson/code
    return (sumR * betaR * phaseR + sumM * betaM * phaseM) * 20;
}



#include "Transform_inc.hlsl"

struct SphericalHarmonics {
    float4 L00 ;
    float4 L1_1;
    float4 L10 ;
    float4 L11 ;
    float4 L2_2;
    float4 L2_1;
    float4 L20 ;
    float4 L21 ;
    float4 L22 ;
};

// Sky buffer
cbuffer SkyConstant : register(b11) {
    Atmosphere _atmosphere;
    float3 _sunDirection;       // the direction TO the light
    float _sunIntensity;
    Transform _stage;
    int4 _simDims;
    int4 _drawControl;
    SphericalHarmonics _irradianceSH;
};


float3 getSunDir() { 
    return _sunDirection;
}
float getSunIntensity() {
    return _sunIntensity;
}


float3 SkyColor(const float3 dir) {
    float3 stage_dir =rotateFrom(_stage, dir);

    float3 origin = float3(0, _atmosphere.earthRadius() + _stage.col_w().y, 0);
    float t0, t1, tMax = -1.0;

    int testEarth = raySphereIntersect(origin, stage_dir, _atmosphere.earthRadius(), t0, t1);
    if (testEarth && t0 > 0)
        tMax = max(0.f, t0);

    return sky_computeIncidentLight(_simDims, _atmosphere, _sunDirection, origin, stage_dir, 0, tMax);
}




// octahedron mapping
// 


// Octahedron wrap the uv in range [-1,1] from up hemisphere to bottom hemisphere (and back?)
float2 octahedron_uvWrap(float2 uv)
{
    return (1.0 - abs(uv.yx)) * (uv.xy >= 0.0 ? 1.0 : -1.0);
}

// Octahedron convert from dir normalized to uv in range [-1,1]
float2 octahedron_uvFromDir(float3 dir)
{
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

float3 sky_dirFromTexcoord(float2 tc)
{
    return octahedron_dirFromUv(2 * tc - 1.0);
}

float2 sky_texcoordFromDir(float3 dir)
{
    return (octahedron_uvFromDir(dir) * 0.5 + 0.5);
}


// Check for offset over texture edge,
bool octahedron_flipped(float2 c, in out bool2 r)
{
    r.x = abs(c.x) >= 1.0;
    r.y = abs(c.y) >= 1.0;
    return r.x || r.y;
}

// Example of computing mirrored repeat sampling 
// of an octahedron map with a small texel offset.
// Note this is not designed to solve the double wrap case.
// The "base" is as computed by Oct3To2() above.

float2 octahedron_offsetCoord(float2 base, float2 offset)
{
    float2 coord = base + offset; // 2 VALU

   // coord = OctFlipped(coord) ? -coord : coord; // 4 VALU
    bool2 r;
    if (octahedron_flipped(coord, r))
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

float3 sky_fetchEnvironmentMap(float3 dir, Texture2D map, SamplerState sampP, SamplerState sampL)
{
    float2 mapSize;
    map.GetDimensions(mapSize.x, mapSize.y);
    float2 texelSize = rcp(mapSize);
    
    float3 color;
    float2 base = octahedron_uvFromDir(dir);
    float2 texcoord = octahedron_offsetCoord(base, 0);
 
    if ((1.0 - abs(base.x) < texelSize.x) || (1.0 - abs(base.y) < texelSize.y))
    {
        color = map.SampleLevel(sampP, octahedron_offsetCoord(base, texelSize), 0).xyz;
        color += map.SampleLevel(sampP, octahedron_offsetCoord(base, float2(texelSize.x, -texelSize.y)), 0).xyz;
        color += map.SampleLevel(sampP, octahedron_offsetCoord(base, float2(-texelSize.x, texelSize.y)), 0).xyz;
        color += map.SampleLevel(sampP, octahedron_offsetCoord(base, float2(-texelSize.x, -texelSize.y)), 0).xyz;
        color *= 0.25;
    }
    else
    {
        color = map.SampleLevel(sampL, texcoord, 0).xyz;
    }
    return color;
}


float3 sky_evalIrradianceSH(float3 dir) {
    float3 d = dir.zxy;
	
    const float c1 = 0.429043;
    const float c2 = 0.511664;
    const float c3 = 0.743125;
    const float c4 = 0.886227;
    const float c5 = 0.247708;
    
    float3 col = c1 * _irradianceSH.L22.xyz * (d.x * d.x - d.y * d.y)
            + c3 * _irradianceSH.L20.xyz * d.z * d.z
            + c4 * _irradianceSH.L00.xyz
            - c5 * _irradianceSH.L20.xyz
            + 2 * c1 * (_irradianceSH.L2_2.xyz * d.x * d.y
                        + _irradianceSH.L21.xyz * d.x * d.z
                        + _irradianceSH.L2_1.xyz * d.y * d.z)
            + 2 * c2 * (_irradianceSH.L11.xyz * d.x
                        + _irradianceSH.L1_1.xyz * d.y
                        + _irradianceSH.L10.xyz * d.z);

    return col;
}

#endif
