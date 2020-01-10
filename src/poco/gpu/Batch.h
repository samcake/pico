// Batch.h 
//
// Sam Gateau - 2020/1/1
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

#include "../Forward.h"

#include "Device.h"

#include "../mas.h"

namespace poco {

    struct BatchInit {
    };

    class Batch {
    protected:
        // Batch is created from the device
        friend class Device;
        Batch();

    public:
        ~Batch();


        virtual void begin(uint8_t currentIndex);
        virtual void clear(const vec4& color, const SwapchainPointer& swapchain, uint8_t index);


        enum class ResourceState {
            COMMON = 0,
            VERTEX_AND_CONSTANT_BUFFER,
            INDEX_BUFFER,
            RENDER_TARGET,
            UNORDERED_ACCESS,
            DEPTH_WRITE,
            DEPTH_READ,
            NON_PIXEL_SHADER_RESOURCE,
            PIXEL_SHADER_RESOURCE,
            STREAM_OUT,
            INDIRECT_ARGUMENT,
            COPY_DEST,
            COPY_SOURCE,
            RESOLVE_DEST,
            RESOLVE_SOURCE,
            _GENERIC_READ,
            RAYTRACING_ACCELERATION_STRUCTURE,
            SHADING_RATE_SOURCE,
            PRESENT,
            PREDICATION,
            VIDEO_DECODE_READ,
            VIDEO_DECODE_WRITE,
            VIDEO_PROCESS_READ,
            VIDEO_PROCESS_WRITE,
            VIDEO_ENCODE_READ,
            VIDEO_ENCODE_WRITE,
        
            COUNT,
        };
        enum class BarrierFlag {
            NONE = 0,
            BEGIN_ONLY,
            END_ONLY,

            COUNT,
        };
        virtual void resourceBarrierTransition(
            BarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
            const SwapchainPointer& swapchain, uint8_t currentIndex, uint32_t subresource);

        virtual void end();

    };
}