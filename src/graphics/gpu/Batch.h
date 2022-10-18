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

#include <core/math/LinearAlgebra.h>

#include "gpu.h"

#include "Device.h"


namespace graphics {

    struct VISUALIZATION_API BatchInit {
    };

    class VISUALIZATION_API Batch {
    protected:
        // Batch is created from the device
        friend class Device;
        Batch();

    public:
        ~Batch();


        virtual void begin(uint8_t currentIndex, const BatchTimerPointer& timer = nullptr);
        virtual void end();

        virtual void beginPass(const SwapchainPointer& swapchain, uint8_t currentIndex);
        virtual void endPass();

        virtual void clear(const SwapchainPointer& swapchain, uint8_t index, const core::vec4& color, float depth = 0.0f);
        virtual void clear(const FramebufferPointer& framebuffer, const core::vec4& color, float depth = 0.0f);

        virtual void resourceBarrierTransition(
            ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
            const SwapchainPointer& swapchain, uint8_t currentIndex, uint32_t subresource);
        virtual void resourceBarrierTransition(
            ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
            const BufferPointer& buffer);
        virtual void resourceBarrierTransition(
            ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
            const TexturePointer& buffer, uint32_t subresource = -1);

        virtual void resourceBarrierRW(
            ResourceBarrierFlag flag, const BufferPointer& buffer);
        virtual void resourceBarrierRW(
            ResourceBarrierFlag flag, const TexturePointer& texture, uint32_t subresource);

        // Viewport and scissor provide a stack management system
        
        inline virtual void setViewport(const core::vec4& viewport) final {
            _viewportStack.back() = viewport;
            _setViewport(viewport);
        }

        inline virtual void pushViewport(const core::vec4& viewport) final {
            _viewportStack.emplace_back(viewport);
            _setViewport(viewport);
        }
        inline virtual void popViewport() final {
            _viewportStack.pop_back();
            _setViewport(_viewportStack.back());
        }

        inline virtual void setScissor(const core::vec4& scissor) final {
            _scissorStack.back() = scissor;
            _setScissor(scissor);
        }

        inline virtual void pushScissor(const core::vec4& scissor) final {
            _scissorStack.emplace_back(scissor);
            _setScissor(scissor);
        }
        inline virtual void popScissor() final {
            _scissorStack.pop_back();
            _setScissor(_scissorStack.back());
        }

    protected:
        virtual void _setViewport(const core::vec4& viewport) {}
        virtual void _setScissor(const core::vec4& scissor) {}

    public:

        virtual void bindFramebuffer(const FramebufferPointer& framebuffer);

        virtual void bindRootDescriptorLayout(PipelineType type, const RootDescriptorLayoutPointer& rootDescriptorLayout);
        virtual void bindPipeline(const PipelineStatePointer& pipeline);

        virtual void bindDescriptorSet(PipelineType type, const DescriptorSetPointer& descriptorSet);
        virtual void bindPushUniform(PipelineType type, uint32_t slot, uint32_t size, const uint8_t* data);

        virtual void bindIndexBuffer(const BufferPointer& buffer);
        virtual void bindVertexBuffers(uint32_t num, const BufferPointer* buffers);

        virtual void draw(uint32_t numPrimitives, uint32_t startIndex);
        virtual void drawIndexed(uint32_t numPrimitives, uint32_t startIndex);

        virtual void uploadTexture(const TexturePointer& dest, const UploadSubresourceLayoutArray& subresourceLayout, const BufferPointer& src);
        virtual void uploadTexture(const TexturePointer& dest);
        virtual void uploadTextureFromInitdata(const DevicePointer& device, const TexturePointer& dest, const std::vector<uint32_t>& subresources = std::vector<uint32_t>());

        virtual void uploadBuffer(const BufferPointer& dest);
        virtual void copyBufferRegion(const BufferPointer& dest, uint32_t destOffset, const BufferPointer& src, uint32_t srcOffset, uint32_t size);

        virtual void dispatch(uint32_t numThreadsX, uint32_t numThreadsY = 1, uint32_t numThreadsZ = 1);

        struct DispatchRaysArgs {
            ShaderTablePointer  shaderTable;
            uint64_t generationShaderRecordStart = 0;
            uint64_t generationShaderRecordSize = 0;

            uint64_t missShaderRecordStart = 0;
            uint64_t missShaderRecordSize = 0;
            uint64_t missShaderRecordStride = 0;

            uint64_t hitGroupShaderRecordStart = 0;
            uint64_t hitGroupShaderRecordSize = 0;
            uint64_t hitGroupShaderRecordStride = 0;

            uint64_t callableShaderRecordStart = 0;
            uint64_t callableShaderRecordSize = 0;
            uint64_t callableShaderRecordStride = 0;

            uint16_t width = 1;
            uint16_t height = 1;
            uint16_t depth = 1;
        };
        virtual void dispatchRays(const DispatchRaysArgs& args);

        std::vector< core::vec4 > _viewportStack = std::vector< core::vec4 >(1); // The stack of viewports used by the set push pop viewport system
        std::vector< core::vec4 > _scissorStack = std::vector< core::vec4 >(1); // The stack of scissors used by the set push pop scissor system
    };
}