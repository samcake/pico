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
}