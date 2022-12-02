// LinearAlgebra.h 
//
// Sam Gateau - January 2020
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
#include <stdint.h>
#include <cmath>

namespace core 
{
    template <typename T>
    inline void swap(T& a, T& b) { T t = a; a = b; b = t; }

    inline constexpr double pi() { return 3.14159265358979323846; }
    inline constexpr double two_pi() { return pi() * 2.0; }
    inline constexpr double half_pi() { return pi() * 0.5; }
    inline constexpr double inv_pi() { return 1.0 / pi(); }
    inline constexpr double inv_two_pi() { return 1.0 / two_pi(); }
    inline constexpr double rad2deg() { return 180.0 * inv_pi(); }
    inline constexpr double deg2rad() { return pi() / 180.0; }
    inline float rad_to_deg(float r) { return r * rad2deg(); }
    inline float deg_to_rad(float d) { return d * deg2rad(); }

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

        vec4 operator+(const vec4& a) const { return vec4(x + a.x, y + a.y, z + a.z, w + a.w); }
        vec4 operator-(const vec4& a) const { return vec4(x - a.x, y - a.y, z - a.z, w - a.w); }
        vec4 operator*(float s) const { return vec4(x * s, y * s, z * s, w * s); }
        vec4 operator-() const { return vec4(-x, -y, -z, -w); }

        float operator[](int i) const { return data()[i]; }
        float& operator[](int i) { return data()[i]; }

        vec3 xyz() const { return vec3(x, y, z); }
        vec2 xy() const { return vec2(x, y); }
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

    static const float FLOAT_EPSILON { 0.0001f };

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
        return a.x * b.x  + a.y * b.y;
    }
    inline float dot(const vec3& a, const vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    inline float dot(const vec4& a, const vec4& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    // Cross product
    inline vec3 cross(const vec3& a, const vec3& b) {
        return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x );
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
                    ((uint32_t)((n.z + 1.0f) * 511) & 0x3FF) << 20 ;

        return vi;
    }

    // Matrix: 4 columns 3 rows 
    // Standard representation of an RTS transform
    struct mat4x3 {
        vec3 _columns[4]{ vec3::X, vec3::Y, vec3::Z, vec3() };
        float* data() { return _columns[0].data(); }
        const float* data() const { return _columns[0].data(); }

        mat4x3() {}
        inline mat4x3(const vec3& c0, const vec3& c1, const vec3& c2, const vec3& c3) {
            _columns[0] = (c0);
            _columns[1] = (c1);
            _columns[2] = (c2);
            _columns[3] = (c3);
        }
        
        inline vec3& x() { return _columns[0]; }
        inline vec3& y() { return _columns[1]; }
        inline vec3& z() { return _columns[2]; }
        inline vec3& w() { return _columns[3]; }

        inline const vec3& x() const { return _columns[0]; }
        inline const vec3& y() const { return _columns[1]; }
        inline const vec3& z() const { return _columns[2]; }
        inline const vec3& w() const { return _columns[3]; }

        inline vec4 row_0() const { return { _columns[0].x, _columns[1].x, _columns[2].x, _columns[3].x }; }
        inline vec4 row_1() const { return { _columns[0].y, _columns[1].y, _columns[2].y, _columns[3].y }; }
        inline vec4 row_2() const { return { _columns[0].z, _columns[1].z, _columns[2].z, _columns[3].z }; }

        inline vec3 row_x() const { return { _columns[0].x, _columns[1].x, _columns[2].x}; }
        inline vec3 row_y() const { return { _columns[0].y, _columns[1].y, _columns[2].y }; }
        inline vec3 row_z() const { return { _columns[0].z, _columns[1].z, _columns[2].z }; }
    };


    // Matrix: 4 raws . 4 columns
    struct mat4 {
        vec4 _columns[4]{ {1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f} };
        float* data() { return _columns[0].data(); }
        const float* data() const { return _columns[0].data(); }

        mat4() {}
        mat4(const vec4& c0, const vec4& c1, const vec4& c2, const vec4& c3) {
            _columns[0] = (c0);
            _columns[1] = (c1);
            _columns[2] = (c2);
            _columns[3] = (c3);
        }
        mat4(const vec3& c0, const vec3& c1, const vec3& c2, const vec3& c3) {
            _columns[0] = vec4(c0, 0.f);
            _columns[1] = vec4(c1, 0.f);
            _columns[2] = vec4(c2, 0.f);
            _columns[3] = vec4(c3, 1.f);
        }
    };


    struct aabox3 {
        vec3 center{ 0.0f };
        vec3 half_size{ 1.0f };

        aabox3() {};
        aabox3(const vec3& center) : center(center) {};
        aabox3(const vec3& center, const vec3& half_size) : center(center), half_size(half_size) {};

        vec4 toSphere() const { return vec4(center, length(half_size)); }

        static aabox3 fromMinMax(const vec3& minPos, const vec3& maxPos) {
             return aabox3((minPos + maxPos) *  0.5, (maxPos - minPos) * 0.5);
        }

        vec3 minPos() const { return center - half_size; }
        vec3 maxPos() const { return center + half_size; }


        static aabox3 fromBound(const aabox3& a, const aabox3& b) {
            return fromMinMax(min(a.minPos(), b.minPos()), max(a.maxPos(), b.maxPos()));
        }
    };

    struct Bounds {
        vec3 _minPos{ 0.0f };
        vec3 _maxPos{ 0.0f };
        vec3 _midPos{ 0.0f };

        const vec3& minPos() const { return _minPos; }
        const vec3& maxPos() const { return _maxPos; }
        const vec3& midPos() const { return _midPos; }

        vec4 toSphere() const {
            vec3 center = (_maxPos + _minPos) * 0.5;
            float radius = 0.5f * length(_maxPos - _minPos);
            return vec4(center, radius);
        }

        aabox3 toBox() const {
            return aabox3::fromMinMax(_minPos, _maxPos);
        }
    };



    struct bivec3 {
        float xy { 0.0f };
        float xz{ 0.0f };
        float yz{ 0.0f };
        float* data() { return &xy; }
        const float* data() const { return &xy; }

        bivec3() {}
        bivec3(float _xy, float _xz, float _yz) : xy(_xy), xz(_xz), yz(_yz) {}
    };

    // Wedge product
    inline bivec3 wedge(const vec3& u, const vec3& v) {
        bivec3 ret(
            u.x * v.y - u.y * v.x, // XY
            u.x * v.z - u.z * v.x, // XZ
            u.y * v.z - u.z * v.y  // YZ
        );
        return ret;
    }

    struct rotor3 {
        // scalar part
        float a = 1;
        bivec3 b;

        rotor3() {}
        rotor3(float a, float b01, float b02, float b12);
        rotor3(float a, const bivec3& bv);

        // construct the rotor that rotates one vector to another
        rotor3(const vec3& vFrom, const vec3& vTo);
        // angle+axis, or rather angle+plane
        rotor3(const bivec3& bvPlane, float angleRadian);

        // rotate a vector by the rotor
        vec3 rotate(const vec3& v) const;

        // multiply
        rotor3 operator*(const rotor3& r) const;
        rotor3 operator*=(const rotor3& r);
        rotor3 rotate(const rotor3& r) const;

        // length utility
        rotor3 reverse() const; // equivalent to conjugate
        float lengthsqrd() const;
        float length() const;
        void normalize();
        rotor3 normal() const;

        vec3 rotate_X() const;
        vec3 rotate_Y() const;
        vec3 rotate_Z() const;
    };

    // default ctor
    inline rotor3::rotor3(float a, float b01, float b02, float b12)
        : a(a), b(b01, b02, b12) {}

    inline rotor3::rotor3(float a, const bivec3& bv)
        : a(a), b(bv) {}

    // construct the rotor that rotates one vector to another
    // uses the usual trick to get the half angle
    inline rotor3::rotor3(const vec3& vFrom, const vec3& vTo) {
        a = 1 + dot(vTo, vFrom);
        // the left side of the products have b a, not a b, so flip
        b = wedge(vTo, vFrom);
        normalize();
    }

    // angle+plane, plane must be normalized
    inline rotor3::rotor3(const bivec3& bvPlane, float angleRadian) {
        float sina = sin(angleRadian / 2.f);
        a = cos(angleRadian / 2.f);
        // the left side of the products have b a, not a b
        b.xy = -sina * bvPlane.xy;
        b.xz = -sina * bvPlane.xz;
        b.yz = -sina * bvPlane.yz;
    }

    // Rotor3-Rotor3 product
    // non-optimized
    inline rotor3 rotor3::operator*(const rotor3& q) const {
        const rotor3& p = *this;
        rotor3 r;

        // here we just expanded the geometric product rules
        r.a = p.a * q.a
            - p.b.xy * q.b.xy - p.b.xz * q.b.xz - p.b.yz * q.b.yz;

        r.b.xy = p.b.xy * q.a + p.a * q.b.xy
            + p.b.yz * q.b.xz - p.b.xz * q.b.yz;

        r.b.xz = p.b.xz * q.a + p.a * q.b.xz
            - p.b.yz * q.b.xy + p.b.xy * q.b.yz;

        r.b.yz = p.b.yz * q.a + p.a * q.b.yz
            + p.b.xz * q.b.xy - p.b.xy * q.b.xz;

        return r;
    }

    /// R v R*
    // non-optimized
    inline vec3 rotor3::rotate(const vec3& v) const {
        const rotor3& p = *this;

        // q = P v
        vec3 q;
        q.x = p.a * v.x + v.y * p.b.xy + v.z * p.b.xz;
        q.y = p.a * v.y - v.x * p.b.xy + v.z * p.b.yz;
        q.z = p.a * v.z - v.x * p.b.xz - v.y * p.b.yz;

        float q012 = -v.x * p.b.yz + v.y * p.b.xz - v.z * p.b.xy; // trivector

        // r = q P*
        vec3 r;
        r.x = p.a * q.x + q.y * p.b.xy + q.z * p.b.xz - q012 * p.b.yz;
        r.y = p.a * q.y - q.x * p.b.xy + q012 * p.b.xz + q.z * p.b.yz;
        r.z = p.a * q.z - q012 * p.b.xy - q.x * p.b.xz - q.y * p.b.yz;

        return r;
    }

    // Rotor3-Rotor3 product
    inline rotor3 rotor3::operator*=(const rotor3& r) {
        (*this) = (*this) * r;
        return *this;
    }

    // rotate a rotor by another
    inline rotor3 rotor3::rotate(const rotor3& r) const {
        // should unwrap this for efficiency
        return (*this) * r * (*this).reverse();
    }

    // Equivalent to conjugate
    inline rotor3 rotor3::reverse() const {
        return rotor3(a, -b.xy, -b.xz, -b.yz);
    }
    // Length Squared
    inline float rotor3::lengthsqrd() const {
        return a*a + (b.xy * b.xy) + (b.xz * b.xz) + (b.yz * b.yz);
    }
    // Length
    inline float rotor3::length() const {
        return sqrt(lengthsqrd());
    }
    // Normalize
    inline void rotor3::normalize() {
        float l = length();
        a /= l; b.xy /= l; b.xz /= l; b.yz /= l;
    }
    // Normalized rotor
    inline rotor3 rotor3::normal() const {
        rotor3 r = *this;
        r.normalize();
        return r;
    }

    inline vec3 rotor3::rotate_X() const {
        return rotate(vec3::X);
    }
    inline vec3 rotor3::rotate_Y() const {
        return rotate(vec3::Y);
    }
    inline vec3 rotor3::rotate_Z() const {
        return rotate(vec3::Z);
    }

    // geometric product (for reference), produces twice the angle, negative direction
    inline rotor3 geo(const vec3& a, const vec3& b) {
        return rotor3(dot(a, b), wedge(a, b));
    }


    // convert Translation T vec3 and Rotation R rotor3 to matrix
    // non-optimized
    inline mat4x3& translation(mat4x3& mat, const vec3& t) {
        mat._columns[3] = t;
        return mat;
    }
    inline mat4x3 translation(const vec3& t) {
        mat4x3 m;
        return translation(m, t);
    }
    inline mat4x3& rotation(mat4x3& mat, const rotor3& r) {
        mat.x() = r.rotate_X();
        mat.y() = r.rotate_Y();
        mat.z() = r.rotate_Z();
        return mat;
    }
    inline mat4x3 rotation(const rotor3& r) {
        mat4x3 m;
        return rotation(m, r);
    }
    inline mat4x3& translation_rotation(mat4x3& mat, const vec3& t, const rotor3& r) {
        return translation(rotation(mat, r), t);
    }
    inline mat4x3 translation_rotation(const vec3& t, const rotor3& r) {
        mat4x3 m;
        return translation_rotation(m, t, r);
    }
    inline mat4x3& rotate(mat4x3& mat, const rotor3& r) {
        mat.x() = r.rotate(mat.x());
        mat.y() = r.rotate(mat.y());
        mat.z() = r.rotate(mat.z());
        return mat;
    }

    

    inline vec3 rotateFrom(const mat4x3& mat, const vec3& d) {
        return vec3(dot(mat.row_x(), d), dot(mat.row_y(), d), dot(mat.row_z(), d));
    }
    inline vec3 rotateTo(const mat4x3& mat, const vec3& d) {
        return vec3(dot(mat.x(), d), dot(mat.y(), d), dot(mat.z(), d));
    }

    inline vec3 transformTo(const mat4x3& mat, const vec3& p) {
        return rotateTo(mat, p - mat.w());
    }
    inline vec3 transformFrom(const mat4x3& mat, const vec3& p) {
        return rotateFrom(mat, p) + mat.w();
    }

    inline vec4 sphere_transformTo(const mat4x3& mat, const vec4& s) {
        return vec4(transformTo(mat, s.xyz()), s.w);
    }
    inline vec4 sphere_transformFrom(const mat4x3& mat, const vec4& s) {
        return vec4(transformFrom(mat, s.xyz()), s.w);
    }

    inline aabox3 aabox_transformTo(const mat4x3& mat, const aabox3& b) {
        // not good yet
        return aabox3(transformTo(mat, b.center), rotateTo(mat, b.half_size));
    }
    inline aabox3 aabox_transformFrom(const mat4x3& mat, const aabox3& b) {
        return aabox3(
            transformFrom(mat, b.center),
            vec3(dot(abs(mat.row_x()), b.half_size),
                 dot(abs(mat.row_y()), b.half_size),
                 dot(abs(mat.row_z()), b.half_size))
        );
    }

    inline mat4x3 mul(const mat4x3& a, const mat4x3& b) {
        auto a_row_0 = a.row_x();
        auto a_row_1 = a.row_y();
        auto a_row_2 = a.row_z();

        return { { dot(a_row_0, b.x()),
                  dot(a_row_1, b.x()),
                  dot(a_row_2, b.x()) },
                        { dot(a_row_0, b.y()),
                          dot(a_row_1, b.y()),
                          dot(a_row_2, b.y()) },
                                { dot(a_row_0, b.z()),
                                  dot(a_row_1, b.z()),
                                  dot(a_row_2, b.z()) },
                                        { dot(a_row_0, b.w()) + a.w().x,
                                          dot(a_row_1, b.w()) + a.w().y,
                                          dot(a_row_2, b.w()) + a.w().z } };
    }

    // Set the orientation part of a transform from the right (X) and up (Y) dirs
    inline mat4x3& transform_set_orientation_from_right_up(mat4x3& mat, const core::vec3& right, const core::vec3& up) {
        mat.x() = normalize(right); // make sure Right is normalized
        mat.z() = normalize(cross(mat.x(), normalize(up))); // compute Back as normalize(Right^Up)
        mat.y() = normalize(cross(mat.z(), mat.x())); // make sure Up is orthogonal to XZ and normalized 
        return mat;
    }

    // Set the orientation part of a transform from the right (X) and up (Y) dirs
    inline mat4x3& transform_set_orientation_from_front_up(mat4x3& mat, const core::vec3& front, const core::vec3& up) {
        mat.z() = -normalize(front); // make sure Front (-Back) is normalized
        mat.x() = normalize(cross(normalize(up), mat.z())); // compute Right as normalize(Up^Back)
        mat.y() = normalize(cross(mat.z(), mat.x())); // make sure Up is orthogonal to XZ and normalized
        return mat;
    }


    // Pico convention for Spherical Coordinates: Azimuth and Elevation angles [rad]
    // A direction D expressed in the reference frame XYZ is defined as:
    // D = [Elevation rotation around local X azimuth][Azimuth rotation around Y] Z
    // Azimuth is rotation around Y axis (vertical) to reference Z axis
    // Elevation is rotation around local Xazimuth axis, the angle in the vertical plane

    // Wrap around the azimuth coordinate to keep it in the range [-pi, pi]
    inline float spherical_wrap_azimuth(float azimuth) {
        if (azimuth > core::pi()) {
            float a = (azimuth + core::pi()) * core::inv_two_pi();
            azimuth = frac(a) * core::two_pi() - core::pi();
        } else if (azimuth < -core::pi()) {
            float a = (azimuth - core::pi()) * core::inv_two_pi();
            azimuth = frac(a) * core::two_pi() + core::pi();
        }
        return azimuth;
    }

    // Clamp elevation in the valid range [-pi/2, pi/2]
    inline float spherical_clamp_elevation(float elevation) {
        return clamp(elevation, -core::half_pi(), core::half_pi());
    }

    // Evaluate the azimuth and elevation angles [rad] from a dir vector
    inline vec2 spherical_dir_to_azimuth_elevation(const core::vec3& dir) {
        // Here atan2(a,b) parameters are flipped!
        // But we measure azimuth angle relative to z axis
        return { atan2(dir.x, dir.z), asin(dir.y) };
    }

    // Generate the dir vector from azimuth and elevation angles [rad]
    inline vec3 spherical_dir_from_azimuth_elevation(float azimuth, float elevation) {
        float calt = cos(elevation);
        return { sin(azimuth) * calt, sin(elevation), cos(azimuth) * calt };
    }

    // Eval the azimuth and elevation angles [rad] for the Z axis of a mat transform
    // avoid gimbal lock!
    inline vec2 spherical_transform_to_Z_azimuth_elevation(const core::mat4x3& mat) {
        // avoid gimbal lock if needed by using the x axis instead of Z axis for azimuth
        if (abs(mat.z().y) >= 1.0) {
            return { atan2(-mat.x().z, mat.x().x), asin(mat.z().y) };
        }
        // Or go the same code a
        else {
            return { atan2(mat.z().x, mat.z().z), asin(mat.z().y) };
        }
    }

    // Set the orientation part of a transform from the Z direction vector from azimuth and elevation angles [rad]
    inline mat4x3& transform_set_orientation_from_Z_azimuth_elevation(mat4x3& mat, float azimuth, float elevation) {
        // compute the cos/sin of the 2 angles
        float ele_s = sin(elevation);
        float ele_c = (abs(ele_s) >= 1.0 ? 0.0 : cos(elevation));
        float azi_s = sin(azimuth);
        float azi_c = (abs(azi_s) >= 1.0 ? 0.0 : cos(azimuth));

        // Compute explicitely the orthonormal base
        mat.z() = {  azi_s * ele_c,  ele_s,   azi_c * ele_c };
        mat.x() = {  azi_c,          0,      -azi_s         };
        mat.y() = { -azi_s * ele_s,  ele_c,  -azi_c * ele_s };

        return mat;
    }

    // Evaluate an orthonormal base given a normalized vector D considered to be the Z+ dir (Back)
    // Return the 2 missing axes X and Y, X is always in horizontal plane of reference
    // If D is aligned with the reference Y axis then we return arbitrary XY values
    inline void transform_eval_orthonormal_base_from_Z(const vec3& D, vec3& X, vec3& Y) {
        if (abs(D.y) > 0.99999999f) {         
            X = vec3(0.0f, -1.0f, 0.0f);
            Y = vec3(-1.0f, 0.0f, 0.0f);
            return;
        }

        // D ={ azi_s * ele_c,    ele_s,   azi_c * ele_c };
        float ele_s = D.y;
        float ele_c = sqrt(D.z * D.z + D.x * D.x);
        float inv_ele_c = 1.0 / ele_c;
        X =   { D.z * inv_ele_c,   0,       -D.x * inv_ele_c };
        Y =   { X.z * ele_s,       ele_c,   -X.x * ele_s     };

        // Inpired by this algorithm:
        // Algorithm by Jeppe Revall Frisvad
        // https://backend.orbit.dtu.dk/ws/portalfiles/portal/126824972/onb_frisvad_jgt2012_v2.pdf
        // evaluate an orthonormal base from a direction D
        // D is injected as the Z axis and teh function produce the X and Y axis
        /*// Handle the singularity
            if (D.z < -0.9999999f) {
                X = vec3(0.0f, -1.0f, 0.0f);
                Y = vec3(-1.0f, 0.0f, 0.0f);
                return;
            }

            float a = 1.0f / (1.0f + D.z);
            float b = -D.x * D.y * a;
            X = vec3(1.0f - D.x * D.x * a, b, -D.x);
            Y = vec3(b, 1.0f - D.y * D.y * a, -D.y);
        */
    }



}

