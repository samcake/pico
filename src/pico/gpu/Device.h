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

#include "../Forward.h"

#include "Descriptor.h" 

namespace pico {

    struct DeviceInit {

    };

    // Device concrete backend implementation
    class DeviceBackend {
    public:
        virtual ~DeviceBackend() {}

        virtual SwapchainPointer createSwapchain(const SwapchainInit& init) = 0;
        virtual FramebufferPointer createFramebuffer(const FramebufferInit& init) = 0;

        virtual BatchPointer createBatch(const BatchInit& init) = 0;

        virtual BufferPointer createBuffer(const BufferInit& init) = 0;

        virtual ShaderPointer createShader(const ShaderInit& init) = 0;
        virtual ShaderPointer createProgram(const ProgramInit& init) = 0;

        virtual PipelineStatePointer createPipelineState(const PipelineStateInit& init) = 0;

        
        virtual DescriptorSetLayoutPointer createDescriptorSetLayout(const DescriptorSetLayoutInit& init) = 0;
        virtual DescriptorSetPointer createDescriptorSet(const DescriptorSetInit& init) = 0;
        virtual void updateDescriptorSet(DescriptorSetPointer& descriptorSet, DescriptorObjects& objects) = 0;

        virtual void executeBatch(const BatchPointer& batch) = 0;
        virtual void presentSwapchain(const SwapchainPointer& swapchain) = 0;
    };

    class Device {
        // Device is created from the api instance
        friend class api;
        Device();
    public:
        ~Device();

        // Factories
        SwapchainPointer createSwapchain(const SwapchainInit& init);
        FramebufferPointer createFramebuffer(const FramebufferInit& init);

        BatchPointer createBatch(const BatchInit& init);

        BufferPointer createBuffer(const BufferInit& init);

        ShaderPointer createShader(const ShaderInit& init);
        ShaderPointer createProgram(const ProgramInit& init);

        PipelineStatePointer createPipelineState(const PipelineStateInit& init);

        DescriptorSetLayoutPointer createDescriptorSetLayout(const DescriptorSetLayoutInit& init);
        DescriptorSetPointer createDescriptorSet(const DescriptorSetInit& init);

        void updateDescriptorSet(DescriptorSetPointer& descriptorSet, DescriptorObjects& objects);

        // CommandQueue work
        void executeBatch(const BatchPointer& batch);
        void presentSwapchain(const SwapchainPointer& swapchain);

    private:
        std::unique_ptr<DeviceBackend> _backend;

    };
}
