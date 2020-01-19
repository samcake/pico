// mas.h 
//
// Sam Gateau - 2020/1/1
// 
// MIT License
//
// Copyright (c) 2020 Sam Gateau
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#pragma once

namespace poco {

    struct vec2 {
        float x, y;
        float* data() { return &x; }
        const float* data() const { return &x; }
        
        vec2() : x(0.0f), y(0.0f) {}
        vec2(float _x) : x(_x), y(_x) {}
        vec2(float _x, float _y) : x(_x), y(_y) {}
        vec2& operator=(const vec2& a) { x = a.x; y = a.y; return *this; }

        vec2 operator+(const vec2& a) const { return vec2(x + a.x, y + a.y); }
        vec2 operator-(const vec2& a) const { return vec2(x - a.x, y - a.y); }
        vec2 operator*(float s) const { return vec2(x * s, y * s); }
    };
    struct vec3 {
        float x, y, z;
        float* data() { return &x; }
        const float* data() const { return &x; }

        vec3() : x(0.0f), y(0.0f), z(0.0f) {}
        vec3(float _x) : x(_x), y(_x), z(_x) {}
        vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
        vec3& operator=(const vec3& a) { x = a.x; y = a.y; z = a.z; return *this; }

        vec3 operator+(const vec3& a) const { return vec3(x + a.x, y + a.y, z + a.z); }
        vec3 operator-(const vec3& a) const { return vec3(x - a.x, y - a.y, z - a.z); }
        vec3 operator*(float s) const { return vec3(x * s, y * s, z * s); }
    };
    struct vec4 {
        float x, y, z, w;
        float* data() { return &x; }
        const float* data() const { return &x; }

        vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
        vec4(float _x) : x(_x), y(_x), z(_x), w(_x) {}
        vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
        vec4(const vec3& xyz, float _w) : x(xyz.x), y(xyz.y), z(xyz.z), w(_w) {}
        vec4& operator=(const vec4& a) { x = a.x; y = a.y; z = a.z; return *this; }

        vec4 operator+(const vec4& a) const { return vec4(x + a.x, y + a.y, z + a.z, w + a.w); }
        vec4 operator-(const vec4& a) const { return vec4(x - a.x, y - a.y, z - a.z, w - a.w); }
        vec4 operator*(float s) const { return vec4(x * s, y * s, z * s, w * s); }
    };

    struct ucvec4 {
        uint8_t x, y, z, w;
        uint8_t* data() { return &x; }
        const uint8_t* data() const { return &x; }

        ucvec4() : x(0), y(0), z(0), w(0) {}
        ucvec4(uint8_t _x) : x(_x), y(_x), z(_x), w(_x) {}
        ucvec4(uint8_t _x, uint8_t _y, uint8_t _z, uint8_t _w) : x(_x), y(_y), z(_z), w(_w) {}
        ucvec4& operator=(const ucvec4& a) { x = a.x; y = a.y; z = a.z; return *this; }

        ucvec4 operator+(const ucvec4& a) const { return ucvec4(x + a.x, y + a.y, z + a.z, w + a.w); }
        ucvec4 operator-(const ucvec4& a) const { return ucvec4(x - a.x, y - a.y, z - a.z, w - a.w); }
        ucvec4 operator*(uint8_t s) const { return ucvec4(x * s, y * s, z * s, w * s); }
    };

    // Abs
    inline float abs(float v) { return (v > 0.0f ? v : -v); }
    inline vec2 abs(const vec2& v) {
        return vec2(abs(v.x), abs(v.y));
    }
    inline vec3 abs(const vec3& v) {
        return vec3(abs(v.x), abs(v.y), abs(v.z));
    }
    inline vec4 abs(const vec4& v) {
        return vec4(abs(v.x), abs(v.y), abs(v.z), abs(v.w));
    }

    // Frac
    inline float frac(float v) {
        float intRem;  
        return modff(v, &intRem);
    }
    inline vec2 frac(const vec2& v) {
        return vec2(frac(v.x), frac(v.y));
    }
    inline vec3 frac(const vec3& v) {
        return vec3(frac(v.x), frac(v.y), frac(v.z));
    }
    inline vec4 frac(const vec4& v) {
        return vec4(frac(v.x), frac(v.y), frac(v.z), frac(v.w));
    }

    // Scale
    inline float scale(float a, float s) { return (a * s); }
    inline vec2 scale(const vec2& a, float s) {
        return vec2(a.x * s, a.y * s);
    }
    inline vec3 scale(const vec3& a, float s) {
        return vec3(a.x * s, a.y * s, a.z * s);
    }
    inline vec4 scale(const vec4& a, float s) {
        return vec4(a.x * s, a.y * s, a.z * s, a.w * s);
    }

    // Mix
    inline float mix(float a, float b, float t) { return (a * (1.f - t) + b * t); }
    inline vec2 mix(const vec2& a, const vec2& b, const vec2& t) {
        return vec2(mix(a.x, b.x, t.x), mix(a.y, b.y, t.y));
    }
    inline vec3 mix(const vec3& a, const vec3& b, const vec3& t) {
        return vec3(mix(a.x, b.x, t.x), mix(a.y, b.y, t.y), mix(a.z, b.z, t.z));
    }
    inline vec4 mix(const vec4& a, const vec4& b, const vec4& t) {
        return vec4(mix(a.x, b.x, t.x), mix(a.y, b.y, t.y), mix(a.z, b.z, t.z), mix(a.w, b.w, t.w));
    }

    // Clamp
    inline float clamp(float a, float b, float c) { return (a < b ? b : (a > c ? c : a)); }
    inline vec2 clamp(const vec2& a, const vec2& b, const vec2& c) {
        return vec2(clamp(a.x, b.x, c.x), clamp(a.y, b.y, c.y));
    }
    inline vec3 clamp(const vec3& a, const vec3& b, const vec3& c) {
        return vec3(clamp(a.x, b.x, c.x), clamp(a.y, b.y, c.y), clamp(a.z, b.z, c.z));
    }
    inline vec4 clamp(const vec4& a, const vec4& b, const vec4& c) {
        return vec4(clamp(a.x, b.x, c.x), clamp(a.y, b.y, c.y), clamp(a.z, b.z, c.z), clamp(a.w, b.w, c.w));
    }

    // Color tools
    inline vec3 colorRGBfromHSV(const vec3& hsv) {
        const vec4 k( 1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
        vec3 f = frac(vec3(hsv.x + k.x, hsv.x + k.y, hsv.x + k.z)) * 6.0f;
        vec3 p = abs(vec3(f.x - k.w, f.y - k.w, f.z - k.w));
        return scale(mix(vec3(k.x), clamp(vec3(p.x - k.x, p.y - k.x, p.z - k.z), vec3(0.0), vec3(1.0)), vec3(hsv.y)), hsv.z);

    }

    inline vec3 colorYUVfromRGB(const vec3& rgb) {
        return vec3( 0.299f * rgb.x + 0.587f * rgb.y + 0.114f * rgb.z,
                    -0.147f * rgb.x - 0.289f * rgb.y + 0.436f * rgb.z,
                     0.615f * rgb.x - 0.515f * rgb.y - 0.100f * rgb.z);
    }

    inline vec3 colorRGBfromYUV(const vec3& yuv) {
        return vec3( yuv.x + 1.140f * yuv.z,
                     yuv.x - 0.395f * yuv.y - 0.581f * yuv.z,
                     yuv.x + 2.032f * yuv.y);
    }

}

