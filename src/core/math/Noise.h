// Noise.h 
//
// Sam Gateau - August 2020
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

namespace core 
{
    // Noise function by Squirrel Eiserloh 'Squirrel3'
    inline uint32_t noise1D(int32_t pos, uint32_t seed) {
        const uint32_t BIT_NOISE1 = 0x68E31DA4;
        const uint32_t BIT_NOISE2 = 0xB5297A4D;
        const uint32_t BIT_NOISE3 = 0x1B56C4E9;

        uint32_t mangled = (uint32_t) pos;
        mangled *= BIT_NOISE1;
        mangled += seed;
        mangled ^= (mangled >> 8);
        mangled += BIT_NOISE2;
        mangled ^= (mangled << 8);
        mangled *= BIT_NOISE3;
        mangled ^= (mangled >> 8);
        return mangled;
    }

    inline uint32_t noise2D(int32_t x, int32_t y, uint32_t seed) {
        const int32_t PRIME_NUMBER = 198491317;
        return noise1D(x + (PRIME_NUMBER * y), seed);
    }

    inline uint32_t noise3D(int32_t x, int32_t y, int32_t z, uint32_t seed) {
        const int32_t PRIME_NUMBER1 = 198491317;
        const int32_t PRIME_NUMBER2 = 6542989;
        return noise1D(x + (PRIME_NUMBER1 * y) + (PRIME_NUMBER2 * z), seed);
    }
}
