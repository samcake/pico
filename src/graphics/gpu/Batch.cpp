// Batch.cpp
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
#include "Batch.h"
#include "Resource.h"

namespace graphics
{

Batch::Batch() {

}

Batch::~Batch() {

}

void Batch::begin(uint8_t currentIndex, const BatchTimerPointer& timer) {}
void Batch::end() {}

void Batch::beginPass(const SwapchainPointer& swapchain, uint8_t currentIndex) {}
void Batch::endPass() {}

void Batch::clear(const SwapchainPointer& swapchain, uint8_t index, const core::vec4& color, float depth) {}
void Batch::clear(const FramebufferPointer& framebuffer, const core::vec4& color, float depth) {}

void Batch::resourceBarrierTransition(
    ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
    const SwapchainPointer& swapchain, uint8_t currentIndex, uint32_t subresource) {}
void Batch::resourceBarrierTransition(
    ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
    const BufferPointer& buffer) {}
void Batch::resourceBarrierTransition(
    ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
    const TexturePointer& buffer, uint32_t subresource) {}

void Batch::resourceBarrierRW(
    ResourceBarrierFlag flag, const BufferPointer& buffer) {}
void Batch::resourceBarrierRW(
    ResourceBarrierFlag flag, const TexturePointer& texture, uint32_t subresource) {}

void Batch::bindFramebuffer(const FramebufferPointer& framebuffer) {}

void Batch::bindRootDescriptorLayout(PipelineType type, const RootDescriptorLayoutPointer& rootLayout) {}
void Batch::bindPipeline(const PipelineStatePointer& pipeline) {}
void Batch::bindDescriptorSet(PipelineType type, const DescriptorSetPointer& descriptorSet) {}
void Batch::bindPushUniform(PipelineType type, uint32_t slot, uint32_t size, const uint8_t* data) {}

void Batch::bindIndexBuffer(const BufferPointer& buffer) {}
void Batch::bindVertexBuffers(uint32_t num, const BufferPointer* buffers) {}

void Batch::draw(uint32_t numPrimitives, uint32_t startIndex) {}
void Batch::drawIndexed(uint32_t numPrimitives, uint32_t startIndex) {}

void Batch::uploadTexture(const TexturePointer& dest, const UploadSubresourceLayoutArray& subresourceLayout, const BufferPointer& src) {}
void Batch::uploadTexture(const TexturePointer& dest) {
    // find amount of data required to fit all the init data
    auto layoutAndSize = Texture::evalUploadSubresourceLayout(dest);

    uploadTexture(dest, layoutAndSize.first, dest->_cpuDataBuffer);
   
    dest->notifyUploaded(); // Indicate that the init data has been loaded
}
void Batch::uploadTextureFromInitdata(const DevicePointer& device, const TexturePointer& dest, const std::vector<uint32_t>& subresources) {
    
    // find amount of data required to fit all the init data
    auto layoutAndSize = Texture::evalUploadSubresourceLayout(dest, subresources);

    // create a buffer, fill it with the data and then call uploadTexture
    BufferInit bufferInit;
    bufferInit.bufferSize = layoutAndSize.second;
    bufferInit.hostVisible = true;
    auto uploadBuffer = device->createBuffer(bufferInit);

    for (const auto& l : layoutAndSize.first) {
        std::byte* destmem = (std::byte*)(uploadBuffer->_cpuMappedAddress) + l.byteOffset;
        memcpy(destmem, dest->_init.initData[l.subresource].data(), l.byteLength);
    }

    uploadTexture(dest, layoutAndSize.first, uploadBuffer);

    dest->notifyUploaded(); // Indicate that the init data has been loaded
}

void Batch::copyBufferRegion(const BufferPointer& dest, uint32_t destOffset, const BufferPointer& src, uint32_t srcOffset, uint32_t size) {}
void Batch::uploadBuffer(const BufferPointer& dest) {}

void Batch::dispatch(uint32_t numThreadsX, uint32_t numThreadsY, uint32_t numThreadsZ) {}
void Batch::dispatchRays(const DispatchRaysArgs& args) {}

} // !using namespace graphics
