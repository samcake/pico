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

#include "Vec.h"

namespace core {

    struct bivec3 {
        float xy{ 0.0f };
        float xz{ 0.0f };
        float yz{ 0.0f };
        float* data() { return &xy; }
        const float* data() const { return &xy; }

        bivec3() {}
        bivec3(float _xy, float _xz, float _yz) : xy(_xy), xz(_xz), yz(_yz) {}
    };

    // Wedge product result in a bivec
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
        static rotor3 make_from_to_dirs(const vec3& from, const vec3& to);
        static rotor3 make_from_x_to_dir(const vec3& to);

        // angle+axis, or rather angle+plane
        static rotor3 make_from_bvplane_angle(const bivec3& bvPlane, float angleRadian);
        static rotor3 make_from_quaternion(float x, float y, float z, float w) {
            return rotor3(w, -z, y, -x);
        }

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
    inline rotor3 rotor3::make_from_to_dirs(const vec3& from, const vec3& to) {
        // the left side of the products have b a, not a b, so flip
        rotor3 r(1 + dot(to, from), wedge(to, from) );
        r.normalize();
        return r;
    }

    inline rotor3 rotor3::make_from_x_to_dir(const vec3& to) {
      //  return make_from_to_dirs(vec3::X, to);

      //  rotor3 r(1 + to.x, wedge(to, vec3::X));
        rotor3 r(1 + to.x, -to.y, -to.z, 0);
       // r.normalize(); return r;
        float l2 = (r.a != 0 ? 2.0f * r.a : r.a * r.a + r.b.xy * r.b.xy + r.b.xz * r.b.xz);
        float linv = 1.0 / sqrt(l2);
        return { r.a * linv, r.b.xy * linv, r.b.xz * linv, 0 };
    }

    // angle+plane, plane must be normalized
    inline rotor3 rotor3::make_from_bvplane_angle(const bivec3& bvPlane, float angleRadian) {
        float sina = sin(angleRadian / 2.f);
        // the left side of the products have b a, not a b
        return { cos(angleRadian / 2.f), { -sina * bvPlane.xy, -sina * bvPlane.xz, -sina * bvPlane.yz }};
    }

    // Rotor3-Rotor3 product
    // non-optimized
    inline rotor3 rotor3::operator*(const rotor3& q) const {
        const rotor3& p = *this;
        rotor3 r;

        // here we just expanded the geometric product rules
        r.a = p.a * q.a - p.b.xy * q.b.xy - p.b.xz * q.b.xz - p.b.yz * q.b.yz;

        r.b.xy = p.b.xy * q.a + p.a * q.b.xy + p.b.yz * q.b.xz - p.b.xz * q.b.yz;

        r.b.xz = p.b.xz * q.a + p.a * q.b.xz - p.b.yz * q.b.xy + p.b.xy * q.b.yz;

        r.b.yz = p.b.yz * q.a + p.a * q.b.yz + p.b.xz * q.b.xy - p.b.xy * q.b.xz;

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
}