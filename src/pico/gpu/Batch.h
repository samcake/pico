// Batch.h 
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

#include "../Forward.h"

#include "Device.h"

#include "../mas.h"

namespace pico {

    struct BatchInit {
    };

    enum class ResourceUsage {
        INDEX_BUFFER = 0,
        VERTEX_BUFFER,
        UNIFORM_BUFFER,

        COUNT,
    };

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
       // RAYTRACING_ACCELERATION_STRUCTURE,
       // SHADING_RATE_SOURCE,
        PRESENT,
        PREDICATION,
       // VIDEO_DECODE_READ,
       // VIDEO_DECODE_WRITE,
       // VIDEO_PROCESS_READ,
       // VIDEO_PROCESS_WRITE,
       // VIDEO_ENCODE_READ,
       // VIDEO_ENCODE_WRITE,

        COUNT,
    };
    enum class ResourceBarrierFlag {
        NONE = 0,
        BEGIN_ONLY,
        END_ONLY,

        COUNT,
    };

    class Batch {
    protected:
        // Batch is created from the device
        friend class Device;
        Batch();

    public:
        ~Batch();


        virtual void begin(uint8_t currentIndex);
        virtual void end();

        virtual void beginPass(const SwapchainPointer& swapchain, uint8_t currentIndex);
        virtual void endPass();

        virtual void clear(const SwapchainPointer& swapchain, uint8_t index, const vec4& color, float depth = 1.0f);

        virtual void resourceBarrierTransition(
            ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
            const SwapchainPointer& swapchain, uint8_t currentIndex, uint32_t subresource);

        virtual void setViewport(vec4& viewport);
        virtual void setScissor(vec4& scissor);

        virtual void setPipeline(PipelineStatePointer pipeline);
        virtual void bindDescriptorSet(DescriptorSetPointer descriptorSet);

        virtual void bindIndexBuffer(BufferPointer& buffer);
        virtual void bindVertexBuffers(uint32_t num, BufferPointer* buffers);

        virtual void draw(uint32_t numPrimitives, uint32_t startIndex);
        virtual void drawIndexed(uint32_t numPrimitives, uint32_t startIndex);



    };
}