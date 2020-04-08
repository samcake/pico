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

#include "../core/LinearAlgebra.h"

namespace pico {

    struct VISUALIZATION_API BatchInit {
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
        IMAGE_SHADER_RESOURCE,
        STREAM_OUT,
        INDIRECT_ARGUMENT,
        COPY_DEST,
        COPY_SOURCE,
        RESOLVE_DEST,
        RESOLVE_SOURCE,
        _GENERIC_READ,

        PRESENT,
        PREDICATION,


        COUNT,
    };
    enum class ResourceBarrierFlag {
        NONE = 0,
        BEGIN_ONLY,
        END_ONLY,

        COUNT,
    };

    class VISUALIZATION_API Batch {
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

        virtual void clear(const SwapchainPointer& swapchain, uint8_t index, const core::vec4& color, float depth = 0.0f);

        virtual void resourceBarrierTransition(
            ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
            const SwapchainPointer& swapchain, uint8_t currentIndex, uint32_t subresource);
        virtual void resourceBarrierTransition(
            ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
            const BufferPointer& buffer);
        virtual void resourceBarrierTransition(
            ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
            const TexturePointer& buffer, uint32_t subresource = -1);

        virtual void setViewport(const core::vec4& viewport);
        virtual void setScissor(const core::vec4& scissor);

        virtual void setPipeline(const PipelineStatePointer& pipeline);
        virtual void bindDescriptorSet(const DescriptorSetPointer& descriptorSet);
        virtual void bindPushUniform(uint32_t slot, uint32_t size, const uint8_t* data);

        virtual void bindIndexBuffer(const BufferPointer& buffer);
        virtual void bindVertexBuffers(uint32_t num, const BufferPointer* buffers);

        virtual void draw(uint32_t numPrimitives, uint32_t startIndex);
        virtual void drawIndexed(uint32_t numPrimitives, uint32_t startIndex);


        virtual void uploadTexture(const TexturePointer& dest, const BufferPointer& src);

    };
}