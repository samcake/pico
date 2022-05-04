// Device.h 
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

#include "gpu.h"

#include "Descriptor.h" 

namespace graphics {

    struct VISUALIZATION_API DeviceInit {
        std::string backend{ "D3D12" };
    };

    // Device concrete backend implementation
    class DeviceBackend {
    public:
        virtual ~DeviceBackend() {}

        virtual SwapchainPointer createSwapchain(const SwapchainInit& init) = 0;
        virtual void resizeSwapchain(const SwapchainPointer& swapchain, uint32_t width, uint32_t height) = 0;

        virtual FramebufferPointer createFramebuffer(const FramebufferInit& init) = 0;

        virtual BatchPointer createBatch(const BatchInit& init) = 0;
        virtual BatchTimerPointer createBatchTimer(const BatchTimerInit& init) = 0;

        virtual BufferPointer _createBuffer(const BufferInit& init, const std::string& name) = 0;
        virtual TexturePointer createTexture(const TextureInit& init) = 0;

        virtual ShaderPointer createShader(const ShaderInit& init) = 0;
        virtual ShaderPointer createProgram(const ProgramInit& init) = 0;

        virtual SamplerPointer createSampler(const SamplerInit& init) = 0;

        virtual PipelineStatePointer createGraphicsPipelineState(const GraphicsPipelineStateInit& init) = 0;
        virtual PipelineStatePointer createComputePipelineState(const ComputePipelineStateInit& init) = 0;

        virtual RootDescriptorLayoutPointer createRootDescriptorLayout(const RootDescriptorLayoutInit& init) = 0;
        
        virtual DescriptorHeapPointer createDescriptorHeap(const DescriptorHeapInit& init) = 0;
        virtual DescriptorHeapPointer getDescriptorHeap() = 0;

        virtual DescriptorSetPointer createDescriptorSet(const DescriptorSetInit& init) = 0;

        virtual void updateDescriptorSet(DescriptorSetPointer& descriptorSet, DescriptorObjects& objects) = 0;

        virtual void executeBatch(const BatchPointer& batch) = 0;
        virtual void presentSwapchain(const SwapchainPointer& swapchain) = 0;

        virtual void flush() = 0;


        virtual void* nativeDevice() = 0;
    };

    class VISUALIZATION_API Device {


    public:
        // Device is created from the factory call instance
        // Do not use this
        Device(DeviceBackend* backend);

        // Factory
        static DevicePointer createDevice(const DeviceInit& init);

        ~Device();

        // Factories
        SwapchainPointer createSwapchain(const SwapchainInit& init);

        FramebufferPointer createFramebuffer(const FramebufferInit& init);

        BatchPointer createBatch(const BatchInit& init);
        BatchTimerPointer createBatchTimer(const BatchTimerInit& init);

        BufferPointer _createBuffer(const BufferInit& init, const std::string& name);

        TexturePointer createTexture(const TextureInit& init);

        ShaderPointer createShader(const ShaderInit& init);
        ShaderPointer createProgram(const ProgramInit& init);

        SamplerPointer createSampler(const SamplerInit& init);

        PipelineStatePointer createGraphicsPipelineState(const GraphicsPipelineStateInit& init);
        PipelineStatePointer createComputePipelineState(const ComputePipelineStateInit& init);

        RootDescriptorLayoutPointer createRootDescriptorLayout(const RootDescriptorLayoutInit& init);

        DescriptorHeapPointer createDescriptorHeap(const DescriptorHeapInit& init);
        DescriptorHeapPointer getDescriptorHeap();

        DescriptorSetPointer createDescriptorSet(const DescriptorSetInit& init);

        // Operations

        // resize swapchain calls a device flush
        void resizeSwapchain(const SwapchainPointer& swapchain, uint32_t width, uint32_t height);

        // update a descriptor with a new set of objects
        void updateDescriptorSet(DescriptorSetPointer& descriptorSet, DescriptorObjects& objects);

        // CommandQueue work
        void executeBatch(const BatchPointer& batch);
        void flush();

        void presentSwapchain(const SwapchainPointer& swapchain);

        void* nativeDevice() { return _backend->nativeDevice(); }

    private:
#pragma warning(push)
#pragma warning(disable: 4251)
        std::unique_ptr<DeviceBackend> _backend;
#pragma warning(pop)

    };
}

#define createBuffer(init) _createBuffer(init, __FILE__)