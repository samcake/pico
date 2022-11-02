// LinearAlgebraCore.h 
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
#include <stdint.h>
#include <cmath>
//#define PICO_SIMD
#ifdef PICO_SIMD
#include <immintrin.h>
#endif

namespace core {
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

    static const float FLOAT_EPSILON{ 0.0001f };

}
