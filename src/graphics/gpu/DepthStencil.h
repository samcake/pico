// DepthStencil.h 
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

namespace graphics {

    enum class ComparisonFunction : uint16_t {
        NEVER = 0,
        LESS,
        EQUAL,
        LESS_EQUAL,
        GREATER,
        NOT_EQUAL,
        GREATER_EQUAL,
        ALWAYS,

        NUM_COMPARISON_FUNCS,
    };

    class DepthTest {
        ComparisonFunction function{ ComparisonFunction::GREATER }; // Default is greater because we are using inverted z by default
        bool writeMask  : 1;
        bool enabled : 1;

    public:
        DepthTest(bool enabled = false, bool writeEnabled = true, ComparisonFunction func = ComparisonFunction::GREATER) :
            function(func), writeMask(writeEnabled), enabled(enabled) {}

        bool isEnabled() const { return enabled; }
        bool isWriteEnabled() const { return writeMask; }
        ComparisonFunction getFunction() const { return function; }

        int32_t getRaw() const { return *(reinterpret_cast<const int32_t*>(this)); }
        DepthTest(int32_t raw) { *(reinterpret_cast<int32_t*>(this)) = raw; }
        bool operator==(const DepthTest& right) const { return getRaw() == right.getRaw(); }
        bool operator!=(const DepthTest& right) const { return getRaw() != right.getRaw(); }
    };

    static_assert(sizeof(DepthTest) == sizeof(uint32_t), "DepthTest size check");


    enum class StencilOp : uint16_t {
        KEEP = 0,
        ZERO,
        REPLACE,
        INCR_SAT,
        DECR_SAT,
        INVERT,
        INCR,
        DECR,

        NUM_STENCIL_OPS,
    };

    class StencilTest {
        ComparisonFunction function : 4;
        StencilOp failOp : 4;
        StencilOp depthFailOp : 4;
        StencilOp passOp : 4;
        int8_t reference{ 0 };
        uint8_t readMask{ 0xff };

    public:
        StencilTest(int8_t reference = 0,
            uint8_t readMask = 0xFF,
            ComparisonFunction func = ComparisonFunction::ALWAYS,
            StencilOp failOp = StencilOp::KEEP,
            StencilOp depthFailOp = StencilOp::KEEP,
            StencilOp passOp = StencilOp::KEEP) :
            function(func),
            failOp(failOp), depthFailOp(depthFailOp), passOp(passOp), reference(reference), readMask(readMask) {}

        ComparisonFunction getFunction() const { return function; }
        StencilOp getFailOp() const { return failOp; }
        StencilOp getDepthFailOp() const { return depthFailOp; }
        StencilOp getPassOp() const { return passOp; }

        int8_t getReference() const { return reference; }
        uint8_t getReadMask() const { return readMask; }

        int32_t getRaw() const { return *(reinterpret_cast<const int32_t*>(this)); }
        StencilTest(int32_t raw) { *(reinterpret_cast<int32_t*>(this)) = raw; }
        bool operator==(const StencilTest& right) const { return getRaw() == right.getRaw(); }
        bool operator!=(const StencilTest& right) const { return getRaw() != right.getRaw(); }
    };
    static_assert(sizeof(StencilTest) == sizeof(uint32_t), "StencilTest size check");

    class StencilActivation {
        uint8_t frontWriteMask = 0xFF;
        uint8_t backWriteMask = 0xFF;
        bool enabled : 1;
        uint8_t _spare1 : 7;
        uint8_t _spare2{ 0 };

    public:
        StencilActivation(bool enabled = false, uint8_t frontWriteMask = 0xFF, uint8_t backWriteMask = 0xFF) :
            frontWriteMask(frontWriteMask), backWriteMask(backWriteMask), enabled(enabled), _spare1{ 0 } {}

        bool isEnabled() const { return enabled; }
        uint8_t getWriteMaskFront() const { return frontWriteMask; }
        uint8_t getWriteMaskBack() const { return backWriteMask; }

        int32_t getRaw() const { return *(reinterpret_cast<const int32_t*>(this)); }
        StencilActivation(int32_t raw) { *(reinterpret_cast<int32_t*>(this)) = raw; }
        bool operator==(const StencilActivation& right) const { return getRaw() == right.getRaw(); }
        bool operator!=(const StencilActivation& right) const { return getRaw() != right.getRaw(); }
    };

    static_assert(sizeof(StencilActivation) == sizeof(uint32_t), "StencilActivation size check");



    class DepthStencilState {
    public:
        DepthStencilState() {}
        DepthStencilState(DepthTest depth) : depthTest(depth) {}

        DepthTest depthTest;
        StencilActivation stencilActivation;
        StencilTest stencilTestFront;
        StencilTest stencilTestBack;


        StencilActivation getStencilActivation() const { return stencilActivation; }
        StencilTest getStencilTestFront() const { return stencilTestFront; }
        StencilTest getStencilTestBack() const { return stencilTestBack; }

    };
}
