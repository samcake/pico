// Blend.h 
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

    enum class BlendArg : uint16_t
    {
        ZERO = 0,
        ONE,
        SRC_COLOR,
        INV_SRC_COLOR,
        SRC_ALPHA,
        INV_SRC_ALPHA,
        DEST_ALPHA,
        INV_DEST_ALPHA,
        DEST_COLOR,
        INV_DEST_COLOR,
        SRC_ALPHA_SAT,
        FACTOR_COLOR,
        INV_FACTOR_COLOR,
        FACTOR_ALPHA,
        INV_FACTOR_ALPHA,

        NUM_BLEND_ARGS,
    };

    enum class BlendOp : uint16_t
    {
        ADD = 0,
        SUBTRACT,
        REV_SUBTRACT,
        MIN,
        MAX,

        NUM_BLEND_OPS,
    };

    enum ColorMask : uint8_t {
        COLOR_MASK_NONE = 0,
        COLOR_MASK_RED = 1,
        COLOR_MASK_GREEN = 2,
        COLOR_MASK_BLUE = 4,
        COLOR_MASK_ALPHA = 8,
        COLOR_MASK_ALL = (COLOR_MASK_RED | COLOR_MASK_GREEN | COLOR_MASK_BLUE | COLOR_MASK_ALPHA),
    };

    struct BlendFunction {
        // Using uint8 here will make the structure as a whole not align to 32 bits
        uint16_t enabled : 8;
        BlendArg sourceColor : 4;
        BlendArg sourceAlpha : 4;
        BlendArg destColor : 4;
        BlendArg destAlpha : 4;
        BlendOp opColor : 4;
        BlendOp opAlpha : 4;

    public:
        BlendFunction(bool enabled,
                      BlendArg sourceColor,
                      BlendOp operationColor,
                      BlendArg destinationColor,
                      BlendArg sourceAlpha,
                      BlendOp operationAlpha,
                      BlendArg destinationAlpha) :
            enabled(enabled),
            sourceColor(sourceColor), sourceAlpha(sourceAlpha), 
            destColor(destinationColor), destAlpha(destinationAlpha),
            opColor(operationColor), opAlpha(operationAlpha) {}

        BlendFunction(bool enabled = false, BlendArg source = BlendArg::ONE, BlendOp operation = BlendOp::ADD, BlendArg destination = BlendArg::ZERO) :
            BlendFunction(enabled, source, operation, destination, source, operation, destination) {}

        bool isEnabled() const { return (enabled != 0); }

        BlendArg getSourceColor() const { return sourceColor; }
        BlendArg getDestinationColor() const { return destColor; }
        BlendOp getOperationColor() const { return opColor; }

        BlendArg getSourceAlpha() const { return sourceAlpha; }
        BlendArg getDestinationAlpha() const { return destAlpha; }
        BlendOp getOperationAlpha() const { return opAlpha; }

        int32_t getRaw() const { return *(reinterpret_cast<const int32_t*>(this)); }
        BlendFunction(int32_t raw) { *(reinterpret_cast<int32_t*>(this)) = raw; }
        bool operator==(const BlendFunction& right) const { return getRaw() == right.getRaw(); }
        bool operator!=(const BlendFunction& right) const { return getRaw() != right.getRaw(); }
    };
    static_assert(sizeof(BlendFunction) == sizeof(uint32_t), "BlendFunction size check");


    struct BlendState {
        BlendFunction blendFunc;
        uint8_t colorWriteMask{ COLOR_MASK_ALL };
    };
}

