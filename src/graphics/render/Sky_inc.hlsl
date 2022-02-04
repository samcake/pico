//
// Sky API
// 
// Implementation taken from
// https://www.scratchapixel.com/lessons/procedural-generation-virtual-worlds/simulating-sky
//

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

// Sky buffer
cbuffer SkyConstant : register(b11) {
    Atmosphere _atmosphere;
    float3 _sunDirection;       // the direction TO the light
    float _sunIntensity;
    Transform _stage;
    int4 _simDims;
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



