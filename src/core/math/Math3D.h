// Math3D.h 
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

#include "Vec.h"
#include "Box.h"
#include "Rotor.h"

namespace core 
{
 
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
#ifdef PICO_SIMD
        auto a_row_0 = a.row_0();
        auto a_row_1 = a.row_1();
        auto a_row_2 = a.row_2();

        __m128 r0 = { a_row_0.x, a_row_0.y, a_row_0.z, a_row_0.w };
        __m128 r1 = { a_row_1.x, a_row_1.y, a_row_1.z, a_row_1.w };
        __m128 r2 = { a_row_2.x, a_row_2.y, a_row_2.z, a_row_2.w };

        __m128 c0 = { b.x().x, b.x().y, b.x().z, 0 };
        __m128 c1 = { b.y().x, b.y().y, b.y().z, 0 };
        __m128 c2 = { b.z().x, b.z().y, b.z().z, 0 };
        __m128 c3 = { b.w().x, b.w().y, b.w().z, 1 };

/*
        __m128 mX0 = _m_mm_dp_ps(r0, c0, 0xFF);
        __m128 mX1 = _m_mm_dp_ps(r1, c0, 0xFF);
        __m128 mX2 = _m_mm_dp_ps(r2, c0, 0xFF);

        __m128 mY0 = _m_mm_dp_ps(r0, c1, 0xFF);
        __m128 mY1 = _m_mm_dp_ps(r1, c1, 0xFF);
        __m128 mY2 = _m_mm_dp_ps(r2, c1, 0xFF);

        __m128 mZ0 = _m_mm_dp_ps(r0, c2, 0xFF);
        __m128 mZ1 = _m_mm_dp_ps(r1, c2, 0xFF);
        __m128 mZ2 = _m_mm_dp_ps(r2, c2, 0xFF);

        __m128 mW0 = _m_mm_dp_ps(r0, c3, 0xFF);
        __m128 mW1 = _m_mm_dp_ps(r1, c3, 0xFF);
        __m128 mW2 = _m_mm_dp_ps(r2, c3, 0xFF);
        */

        return { {
                _mm_dp_ps(r0, c0, 0xFF).m128_f32[0],
                _mm_dp_ps(r1, c0, 0xFF).m128_f32[0],
                _mm_dp_ps(r2, c0, 0xFF).m128_f32[0]},
                {
                _mm_dp_ps(r0, c1, 0xFF).m128_f32[0],
                _mm_dp_ps(r1, c1, 0xFF).m128_f32[0],
                _mm_dp_ps(r2, c1, 0xFF).m128_f32[0]},
                {
                _mm_dp_ps(r0, c2, 0xFF).m128_f32[0],
                _mm_dp_ps(r1, c2, 0xFF).m128_f32[0],
                _mm_dp_ps(r2, c2, 0xFF).m128_f32[0]},
                {
                _mm_dp_ps(r0, c3, 0xFF).m128_f32[0],
                _mm_dp_ps(r1, c3, 0xFF).m128_f32[0],
                _mm_dp_ps(r2, c3, 0xFF).m128_f32[0]} };

#else
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
#endif
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

