// Rasterizer.h 
//
// Sam Gateau - October 2020
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

#include <cstdint>

namespace graphics {
    enum class FillMode : uint8_t
    {
        FILL = 0,
        LINE,
        POINT,

        NUM_FILL_MODES,
    };

    enum class CullMode : uint8_t
    {
        NONE = 0,
        FRONT,
        BACK,

        NUM_CULL_MODES,
    };

    struct RasterizerState { 
        RasterizerState() :
            depthBias(0.0f),
            depthBiasSlopeScale(0.0f),
            fillMode(FillMode::FILL),
            cullMode(CullMode::NONE),
            frontFaceClockwise(false),
            depthClampEnable(false),
            primitiveDiscardEnable(false),
            conservativeRasterizerEnable(false),

            multisampleEnable(true),
            antialisedLineEnable(true),
            alphaToCoverageEnable(false)
        {}


        float depthBias;
        float depthBiasSlopeScale;
        FillMode fillMode;
        CullMode cullMode;
        bool frontFaceClockwise : 1;
        bool depthClampEnable : 1;
        bool primitiveDiscardEnable : 1; // set to true to discard any primitive from going to rasterizer
        bool conservativeRasterizerEnable : 1;

        // Maybe its own state in the futuere to describe multisample state separately like in vulkan
        bool multisampleEnable : 1;
        bool antialisedLineEnable : 1;
        bool alphaToCoverageEnable : 1;
      //  uint8_t _spare1 : 2;
    };
}

