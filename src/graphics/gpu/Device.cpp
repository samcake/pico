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


graphics::PixelFormat graphics::defaultColorBufferFormat() {
    return  graphics::PixelFormat::R8G8B8A8_UNORM_SRGB;
    //return  graphics::PixelFormat::R8G8B8A8_UNORM;
    
    //return  graphics::PixelFormat::R10G10B10_XR_BIAS_A2_UNORM; // doesn't work in some pipeline ?
    //return  graphics::PixelFormat::R10G10B10A2_UNORM;

    //return  graphics::PixelFormat::R16G16B16A16_FLOAT;
}


using namespace graphics;


DevicePointer Device::createDevice(const DeviceInit& init) {
    DevicePointer device;
    if (init.backend.compare("D3D12") == 0 ) {
        device = std::make_shared<Device>(new D3D12Backend());
    }

    return device;
}

Device::Device(DeviceBackend* backend) : _backend(backend) {
}

Device::~Device() {

}

SwapchainPointer Device::createSwapchain(const SwapchainInit& init) {
    return _backend->createSwapchain(init);
}

void Device::resizeSwapchain(const SwapchainPointer& swapchain, uint32_t width, uint32_t height) {
    _backend->resizeSwapchain(swapchain, width, height);
}

FramebufferPointer Device::createFramebuffer(const FramebufferInit& init) {
    return _backend->createFramebuffer(init);
}

BufferPointer Device::_createBuffer(const BufferInit& init, const std::string& name) {
    return _backend->_createBuffer(init, name);
}
TexturePointer Device::createTexture(const TextureInit& init) {
    return _backend->createTexture(init);
}

GeometryPointer Device::createGeometry(const GeometryInit& init) {
    return _backend->createGeometry(init);
}

ShaderPointer Device::createShader(const ShaderInit& init) {
    return _backend->createShader(init);
}
ShaderPointer Device::createProgram(const ProgramInit& init) {
    return _backend->createProgram(init);
}

SamplerPointer Device::createSampler(const SamplerInit& init) {
    return _backend->createSampler(init);
}

PipelineStatePointer Device::createGraphicsPipelineState(const GraphicsPipelineStateInit& init) {
    return _backend->createGraphicsPipelineState(init);
}
PipelineStatePointer Device::createComputePipelineState(const ComputePipelineStateInit& init) {
    return _backend->createComputePipelineState(init);
}
PipelineStatePointer Device::createRaytracingPipelineState(const RaytracingPipelineStateInit& init) {
    return _backend->createRaytracingPipelineState(init);
}

RootDescriptorLayoutPointer Device::createRootDescriptorLayout(const RootDescriptorLayoutInit& init) {
    return _backend->createRootDescriptorLayout(init);
}
DescriptorHeapPointer Device::createDescriptorHeap(const DescriptorHeapInit& init) {
    return _backend->createDescriptorHeap(init);
}
DescriptorHeapPointer Device::getDescriptorHeap() {
    return _backend->getDescriptorHeap();
}

DescriptorSetPointer Device::createDescriptorSet(const DescriptorSetInit& init) {
    return _backend->createDescriptorSet(init);
}
void Device::updateDescriptorSet(DescriptorSetPointer& descriptorSet, DescriptorObjects& objects) {
    return _backend->updateDescriptorSet(descriptorSet, objects);
}

ShaderEntry Device::getShaderEntry(const PipelineStatePointer& pipeline, const std::string& entry) {
    return _backend->getShaderEntry(pipeline, entry);
}
ShaderTablePointer Device::createShaderTable(const ShaderTableInit& init) {
    return _backend->createShaderTable(init);
}

BatchTimerPointer Device::createBatchTimer(const BatchTimerInit& init) {
    return _backend->createBatchTimer(init);
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

void Device::flush() {
    _backend->flush();
}
