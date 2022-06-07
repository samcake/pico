//
// Surface Geometry API
//
#ifndef SURFACE_INC
#define SURFACE_INC

// Technique to compute tangent space in fragment shader by
// Christian Schüler
// http://www.thetenthplanet.de/archives/1180
// Compute tangent space and final normal
float3x3 computeTangentSpace(float3 surfNormal, float3 viewVec, float2 uv) {
    float3 dp1 = ddx(viewVec);
    float3 dp2 = ddy(viewVec);
    float2 duv1 = ddx(uv);
    float2 duv2 = ddy(uv);

    float3 dp2perp = cross(dp2, surfNormal); // V ^ N ~= T
    float3 dp1perp = cross(surfNormal, dp1); // N ^ U ~= B
    float3 T = (dp2perp * duv1.x + dp1perp * duv2.x);
    float3 B = (dp2perp * duv1.y + dp1perp * duv2.y);

    float invmax = rsqrt(max(dot(T, T), dot(B, B)));
    T *= invmax;
    B *= invmax;
    return float3x3(T,B, surfNormal);
}

#endif
