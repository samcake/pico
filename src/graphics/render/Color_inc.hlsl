//
// Color API
// 

float3 color_rgbFromHSV(const float3 hsv) {
    const float4 k = float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
    float3 f = frac(float3(hsv.x + k.x, hsv.x + k.y, hsv.x + k.z)) * 6.0f;
    float3 p = abs(float3(f.x - k.w, f.y - k.w, f.z - k.w));
    p = clamp(float3(p.x - k.x, p.y - k.x, p.z - k.z), float3(0, 0, 0), float3(1.0, 1.0, 1.0));
    return lerp(k.xxx, p, hsv.yyy) * hsv.z;
}

float3 color_rainbow(float n, float d = 1.0f) {
    return color_rgbFromHSV(float3((n / d) * (5.0 / 6.0), 1.0, 1.0));
}

float3 color_rgbFromDir(float3 dir) {
    float h = 0.5 + 0.5 * ((atan2(dir.z, dir.x)) / acos(-1.0));
    float s = 1.0 - (dir.y > 0 ? dir.y : 0.0);
    float v = (dir.y > 0 ? 1.0 : 1.0 + dir.y);
    return color_rgbFromHSV(float3(h, sqrt(s), sqrt(v)));
}