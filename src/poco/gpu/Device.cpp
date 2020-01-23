// Device.cpp
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
#include "Device.h"

#include "Swapchain.h"

#include "../d3d12/D3D12Backend.h"

using namespace poco;

Device::Device() {
    _backend.reset(new D3D12Backend());
}

Device::~Device() {

}

SwapchainPointer Device::createSwapchain(const SwapchainInit& init) {
    return _backend->createSwapchain(init);
}

FramebufferPointer Device::createFramebuffer(const FramebufferInit& init) {
    return _backend->createFramebuffer(init);
}

BufferPointer Device::createBuffer(const BufferInit& init) {
    return _backend->createBuffer(init);
}

ShaderPointer Device::createShader(const ShaderInit& init) {
    return _backend->createShader(init);
}
ShaderPointer Device::createProgram(const ProgramInit& init) {
    return _backend->createProgram(init);
}

PipelineStatePointer Device::createPipelineState(const PipelineStateInit& init) {
    return _backend->createPipelineState(init);
}

DescriptorSetLayoutPointer Device::createDescriptorSetLayout(const DescriptorSetLayoutInit& init) {
    return _backend->createDescriptorSetLayout(init);
}
DescriptorSetPointer Device::createDescriptorSet(const DescriptorSetInit& init) {
    return _backend->createDescriptorSet(init);
}
void Device::updateDescriptorSet(DescriptorSetPointer& descriptorSet, DescriptorObjects& objects) {
    return _backend->updateDescriptorSet(descriptorSet, objects);
}

BatchPointer Device::createBatch(const BatchInit& init) {
    return _backend->createBatch(init);
}

void Device::executeBatch(const BatchPointer& batch) {
    _backend->executeBatch(batch);
}

void Device::presentSwapchain(const SwapchainPointer& swapchain) {
    _backend->presentSwapchain(swapchain);
}
