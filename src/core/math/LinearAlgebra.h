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
#include <utility>


namespace core 
{

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
        ivec4& operator=(const ivec4& a) { x = a.x; y = a.y; z = a.z; return *this; }

        ivec4 operator+(const ivec4& a) const { return ivec4(x + a.x, y + a.y, z + a.z, w + a.w); }
        ivec4 operator-(const ivec4& a) const { return ivec4(x - a.x, y - a.y, z - a.z, w - a.w); }
        ivec4 operator*(int32_t s) const { return ivec4(x * s, y * s, z * s, w * s); }
        ivec4 operator-() const { return ivec4(-x, -y, -z, -w); }
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
        const uint32_t xs = n.x < 0;
        const uint32_t ys = n.y < 0;
        const uint32_t zs = n.z < 0;
        const int w = 0;
        const uint32_t ws = w < 0;
  /*      uint32_t vi =
            ws << 31 | ((uint32_t)(w + (ws << 1)) & 1) << 30 |
            zs << 29 | ((uint32_t)(n.z * 511 + (zs << 9)) & 511) << 20 |
            ys << 19 | ((uint32_t)(n.y * 511 + (ys << 9)) & 511) << 10 |
            xs << 9 | ((uint32_t)(n.x * 511 + (xs << 9)) & 511);
    */    uint32_t vi = 
                    ((uint32_t)((n.x + 1.0f) * 511) & 1023) |
                    ((uint32_t)((n.y + 1.0f) * 511) & 1023) << 10 |
                    ((uint32_t)((n.z + 1.0f) * 511) & 1023) << 20 ;

        return vi;
    }

    // Matrix: 4 columns 3 rows 
    struct mat4x3 {
        vec3 _columns[4]{ vec3::X, vec3::Y, vec3::Z, vec3() };
        float* data() { return _columns[0].data(); }
        const float* data() const { return _columns[0].data(); }

        mat4x3() {}
        mat4x3(const vec3& c0, const vec3& c1, const vec3& c2, const vec3& c3) {
            _columns[0] = (c0);
            _columns[1] = (c1);
            _columns[2] = (c2);
            _columns[3] = (c3);
        }
        
        vec4 row_0() const { return { _columns[0].x, _columns[1].x, _columns[2].x, _columns[3].x }; }
        vec4 row_1() const { return { _columns[0].y, _columns[1].y, _columns[2].y, _columns[3].y }; }
        vec4 row_2() const { return { _columns[0].z, _columns[1].z, _columns[2].z, _columns[3].z }; }

        const vec3& x() const { return _columns[0]; }
        const vec3& y() const { return _columns[1]; }
        const vec3& z() const { return _columns[2]; }

        vec3 row_x() const { return { _columns[0].x, _columns[1].x, _columns[2].x}; }
        vec3 row_y() const { return { _columns[0].y, _columns[1].y, _columns[2].y }; }
        vec3 row_z() const { return { _columns[0].z, _columns[1].z, _columns[2].z }; }

        const vec3& w() const { return _columns[3]; }
    };

    // Matrix: 3 raws . 4 columns
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
        return std::move(translation(mat4x3(), t));
    }
    inline mat4x3& rotation(mat4x3& mat, const rotor3& r) {
        mat._columns[0] = r.rotate_X();
        mat._columns[1] = r.rotate_Y();
        mat._columns[2] = r.rotate_Z();
        return mat;
    }
    inline mat4x3 rotation(const rotor3& r) {
        return std::move(rotation(mat4x3(), r));
    }
    inline mat4x3& translation_rotation(mat4x3& mat, const vec3& t, const rotor3& r) {
        return translation(rotation(mat, r), t);
    }
    inline mat4x3 translation_rotation(const vec3& t, const rotor3& r) {
        return std::move(translation_rotation(mat4x3(), t, r));
    }
    inline mat4x3& rotate(mat4x3& mat, const rotor3& r) {
        mat._columns[0] = r.rotate(mat._columns[0]);
        mat._columns[1] = r.rotate(mat._columns[1]);
        mat._columns[2] = r.rotate(mat._columns[2]);
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
}

