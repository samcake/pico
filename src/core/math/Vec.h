// Vec.h 
//
// Sam Gateau - November 2022
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
#include "math.h"

namespace core {

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
        vec2 operator-() const { return vec2(-x, -y); }
    };

    struct ivec2 {
        int32_t x, y;
        int32_t* data() { return &x; }
        const int32_t* data() const { return &x; }

        ivec2() : x(0), y(0) {}
        ivec2(int32_t _x) : x(_x), y(_x) {}
        ivec2(int32_t _x, int32_t _y) : x(_x), y(_y) {}
        ivec2& operator=(const ivec2& a) { x = a.x; y = a.y; return *this; }

        ivec2 operator+(const ivec2& a) const { return ivec2(x + a.x, y + a.y); }
        ivec2 operator-(const ivec2& a) const { return ivec2(x - a.x, y - a.y); }
        ivec2 operator*(int32_t s) const { return ivec2(x * s, y * s); }
        ivec2 operator-() const { return ivec2(-x, -y); }

        int32_t operator[](int i) const { return data()[i]; }
        int32_t& operator[](int i) { return data()[i]; }
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
        vec3 operator*(const vec3& a) const { return vec3(x * a.x, y * a.y, z * a.z); }
        vec3 operator-() const { return vec3(-x, -y, -z); }

        float operator[](int i) const { return data()[i]; }
        float& operator[](int i) { return data()[i]; }

        const static vec3 X;
        const static vec3 Y;
        const static vec3 Z;
    };

    inline const vec3 vec3::X{ 1.0f, 0, 0 };
    inline const vec3 vec3::Y{ 0, 1.0f, 0 };
    inline const vec3 vec3::Z{ 0, 0, 1.0f };
    struct ivec3 {
        int32_t x, y, z;
        int32_t* data() { return &x; }
        const int32_t* data() const { return &x; }

        ivec3() : x(0), y(0), z(0) {}
        ivec3(int32_t _x) : x(_x), y(_x), z(_x) {}
        ivec3(int32_t _x, int32_t _y, int32_t _z) : x(_x), y(_y), z(_z) {}
        ivec3& operator=(const ivec3& a) { x = a.x; y = a.y; z = a.z; return *this; }

        ivec3 operator+(const ivec3& a) const { return ivec3(x + a.x, y + a.y, z + a.z); }
        ivec3 operator-(const ivec3& a) const { return ivec3(x - a.x, y - a.y, z - a.z); }
        ivec3 operator*(int32_t s) const { return ivec3(x * s, y * s, z * s); }
        ivec3 operator-() const { return ivec3(-x, -y, -z); }

        int32_t operator[](int i) const { return data()[i]; }
        int32_t& operator[](int i) { return data()[i]; }
    };

    struct vec4 {
        float x, y, z, w;
        float* data() { return &x; }
        const float* data() const { return &x; }

        vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
        vec4(float _x) : x(_x), y(_x), z(_x), w(_x) {}
        vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
        vec4(const vec3& xyz, float _w) : x(xyz.x), y(xyz.y), z(xyz.z), w(_w) {}
        vec4& operator=(const vec4& a) { x = a.x; y = a.y; z = a.z; w = a.w; return *this; }
        vec4& operator=(const vec3& a) { x = a.x; y = a.y; z = a.z; w = 0; return *this; }

        vec4 operator+(const vec4& a) const { return vec4(x + a.x, y + a.y, z + a.z, w + a.w); }
        vec4 operator-(const vec4& a) const { return vec4(x - a.x, y - a.y, z - a.z, w - a.w); }
        vec4 operator*(float s) const { return vec4(x * s, y * s, z * s, w * s); }
        vec4 operator-() const { return vec4(-x, -y, -z, -w); }

        float operator[](int i) const { return data()[i]; }
        float& operator[](int i) { return data()[i]; }

        vec3 xyz() const { return vec3(x, y, z); }
        vec2 xy() const { return vec2(x, y); }

        vec3& as_xyz() { return *((vec3*) this); }
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
        ucvec4 operator-() const { return ucvec4(-x, -y, -z, -w); }
    };

    struct ivec4 {
        int32_t x, y, z, w;
        int32_t* data() { return &x; }
        const int32_t* data() const { return &x; }

        ivec4() : x(0), y(0), z(0), w(0) {}
        ivec4(int32_t _x) : x(_x), y(_x), z(_x), w(_x) {}
        ivec4(int32_t _x, int32_t _y, int32_t _z, int32_t _w) : x(_x), y(_y), z(_z), w(_w) {}
        ivec4& operator=(const ivec4& a) { x = a.x; y = a.y; z = a.z; w = a.w; return *this; }

        ivec4 operator+(const ivec4& a) const { return ivec4(x + a.x, y + a.y, z + a.z, w + a.w); }
        ivec4 operator-(const ivec4& a) const { return ivec4(x - a.x, y - a.y, z - a.z, w - a.w); }
        ivec4 operator*(int32_t s) const { return ivec4(x * s, y * s, z * s, w * s); }
        ivec4 operator-() const { return ivec4(-x, -y, -z, -w); }

        int32_t operator[](int i) const { return data()[i]; }
        int32_t& operator[](int i) { return data()[i]; }

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

    // Max Min
    inline uint64_t min(uint64_t a, uint64_t b) { return (a < b ? a : b); }
    inline uint64_t max(uint64_t a, uint64_t b) { return (a > b ? a : b); }
    inline uint32_t min(uint32_t a, uint32_t b) { return (a < b ? a : b); }
    inline uint32_t max(uint32_t a, uint32_t b) { return (a > b ? a : b); }
    inline float min(float a, float b) { return (a < b ? a : b); }
    inline float max(float a, float b) { return (a > b ? a : b); }
    inline vec2 min(const vec2& a, const vec2& b) {
        return vec2(min(a.x, b.x), min(a.y, b.y));
    }
    inline vec2 max(const vec2& a, const vec2& b) {
        return vec2(max(a.x, b.x), max(a.y, b.y));
    }
    inline vec3 min(const vec3& a, const vec3& b) {
        return vec3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
    }
    inline vec3 max(const vec3& a, const vec3& b) {
        return vec3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
    }
    inline vec4 min(const vec4& a, const vec4& b) {
        return vec4(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w));
    }
    inline vec4 max(const vec4& a, const vec4& b) {
        return vec4(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w));
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

    // Dot product
    inline float dot(float a, float b) { return (a * b); }
    inline float dot(const vec2& a, const vec2& b) {
        return a.x * b.x + a.y * b.y;
    }
    inline float dot(const vec3& a, const vec3& b) {
#ifdef PICO_SIMD
        __m128 r0 = { a.x, a.y, a.z, 0 };
        __m128 r1 = { b.x, b.y, b.z, 0 };
        __m128 r = _mm_dp_ps(r0, r1, 0xFF);
        return r.m128_f32[3];
#else
        return a.x * b.x + a.y * b.y + a.z * b.z;
#endif
    }
    inline float dot(const vec4& a, const vec4& b) {
#ifdef PICO_SIMD
        __m128 r0 = { a.x, a.y, a.z, a.w };
        __m128 r1 = { b.x, b.y, b.z, b.w };
        __m128 r = _mm_dp_ps(r0, r1, 0xFF);
        return r.m128_f32[0];
#else
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
#endif
    }

    // Cross product
    inline vec3 cross(const vec3& a, const vec3& b) {
        return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    }

    // Length
    inline float length(const vec2& a) {
        return sqrt(dot(a, a));
    }
    inline float length(const vec3& a) {
        return sqrt(dot(a, a));
    }
    inline float length(const vec4& a) {
        return sqrt(dot(a, a));
    }

    // Normalize
    inline vec2 normalize(const vec2& a) {
        auto l = sqrt(dot(a, a));
        auto il = ((l > 0.0f) ? (1.0f / l) : 1.0f);
        return scale(a, il);
    }
    inline vec3 normalize(const vec3& a) {
        auto l = sqrt(dot(a, a));
        auto il = ((l > 0.0f) ? (1.0f / l) : 1.0f);
        return scale(a, il);
    }
    inline vec4 normalize(const vec4& a) {
        auto l = sqrt(dot(a, a));
        auto il = ((l > 0.0f) ? (1.0f / l) : 1.0f);
        return scale(a, il);
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

    // Sort in increasing order 
    inline ivec2 sort_increasing(const ivec2& a) {
        return (a.x <= a.y ? a : ivec2(a.y, a.x));
    }
    inline ivec3 sort_increasing(const ivec3& a) {
        ivec3 ordered{ a };
        if (ordered.y < ordered.x) {
            swap(ordered.x, ordered.y);
        }

        if (ordered.z < ordered.y) {
            swap(ordered.z, ordered.y);
        }

        if (ordered.y < ordered.x) {
            swap(ordered.x, ordered.y);
        }

        return ordered;
    }

    // Color tools
    inline vec3 colorRGBfromHSV(const vec3& hsv) {
        const vec4 k(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
        vec3 f = frac(vec3(hsv.x + k.x, hsv.x + k.y, hsv.x + k.z)) * 6.0f;
        vec3 p = abs(vec3(f.x - k.w, f.y - k.w, f.z - k.w));
        return scale(mix(vec3(k.x), clamp(vec3(p.x - k.x, p.y - k.x, p.z - k.z), vec3(0.0), vec3(1.0)), vec3(hsv.y)), hsv.z);

    }

    inline vec3 colorYUVfromRGB(const vec3& rgb) {
        return vec3(0.299f * rgb.x + 0.587f * rgb.y + 0.114f * rgb.z,
            -0.147f * rgb.x - 0.289f * rgb.y + 0.436f * rgb.z,
            0.615f * rgb.x - 0.515f * rgb.y - 0.100f * rgb.z);
    }

    inline vec3 colorRGBfromYUV(const vec3& yuv) {
        return vec3(yuv.x + 1.140f * yuv.z,
            yuv.x - 0.395f * yuv.y - 0.581f * yuv.z,
            yuv.x + 2.032f * yuv.y);
    }

    // Normal packing
    inline uint32_t packNormal32I(const vec3& n) {
        //inline uint32_t Pack_INT_2_10_10_10_REV(float x, float y, float z, float w)
     /*    const uint32_t xs = n.x < 0;
        const uint32_t ys = n.y < 0;
        const uint32_t zs = n.z < 0;
        const int w = 0;
        const uint32_t ws = w < 0;
       uint32_t vi =
            ws << 31 | ((uint32_t)(w + (ws << 1)) & 1) << 30 |
            zs << 29 | ((uint32_t)(n.z * 511 + (zs << 9)) & 511) << 20 |
            ys << 19 | ((uint32_t)(n.y * 511 + (ys << 9)) & 511) << 10 |
            xs << 9 | ((uint32_t)(n.x * 511 + (xs << 9)) & 511);
    */    uint32_t vi =
        ((uint32_t)((n.x + 1.0f) * 511) & 0x3FF) |
        ((uint32_t)((n.y + 1.0f) * 511) & 0x3FF) << 10 |
        ((uint32_t)((n.z + 1.0f) * 511) & 0x3FF) << 20;

    return vi;
    }
}
