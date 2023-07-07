//
// Color API
// 
#ifndef COLOR_INC
#define COLOR_INC

float3 color_rgbFromHSV(const float3 hsv) {
    const float4 k = float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
    float3 f = frac(float3(hsv.x + k.x, hsv.x + k.y, hsv.x + k.z)) * 6.0f;
    float3 p = abs(float3(f.x - k.w, f.y - k.w, f.z - k.w));
    p = clamp(float3(p.x - k.x, p.y - k.x, p.z - k.z), float3(0, 0, 0), float3(1.0, 1.0, 1.0));
    return lerp(k.xxx, p, hsv.yyy) * hsv.z;
}

float3 color_rainbow(float n, float d = 1.0f)
{
    float level = floor(frac(rcp(d) * n) * 6.0);
    float r = float(level <= 2.0) + float(level > 4.0) * 0.5;
    float g = max(1.0 - abs(level - 2.0) * 0.5, 0.0);
    float b = (1.0 - (level - 4.0) * 0.5) * float(level >= 4.0);
    return float3(r, g, b);
}

float3 color_rainbowHSV(float n, float d = 1.0f) {
    return color_rgbFromHSV(float3((n / d) * (5.0 / 6.0), 1.0, 1.0));
}

float3 color_rgbFromDir(float3 dir) {
    float h = 0.5 + 0.5 * ((atan2(dir.z, dir.x)) / acos(-1.0));
    float s = 1.0 - (dir.y > 0 ? dir.y : 0.0);
    float v = (dir.y > 0 ? 1.0 : 1.0 + dir.y);
    return color_rgbFromHSV(float3(h, sqrt(s), sqrt(v)));
}

// Converts a color from linear light gamma to sRGB gamma
float3 color_sRGBFromLinear(float3 linearRGB) {
    float3 cutoff = step(linearRGB, 0.0031308);
    float3 higher = (1.055) * pow(linearRGB, 1.0 / 2.4) - (0.055);
    float3 lower = linearRGB.rgb * (12.92);

    return lerp(higher, lower, cutoff);
}

// Converts a color from sRGB gamma to linear light gamma
float3 color_sRGBToLinear(float3 sRGB) {
    float3 cutoff = step(sRGB, 0.04045);
    float3 higher = pow((sRGB + 0.055) / (1.055), (2.4));
    float3 lower = sRGB / (12.92);

    return lerp(higher, lower, cutoff);
}


#endif
