// Batch.cpp
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
#include "Batch.h"

using namespace poco;

Batch::Batch() {

}

Batch::~Batch() {

}

void Batch::begin(uint8_t currentIndex) {}
void Batch::end() {}

void Batch::beginPass(const SwapchainPointer& swapchain, uint8_t currentIndex) {}
void Batch::endPass() {}

void Batch::clear(const vec4& color, const SwapchainPointer& swapchain, uint8_t index) {}

void Batch::resourceBarrierTransition(
    ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
    const SwapchainPointer& swapchain, uint8_t currentIndex, uint32_t subresource) {}

void Batch::setViewport(vec4& viewport) {}
void Batch::setScissor(vec4& scissor) {}

void Batch::setPipeline(PipelineStatePointer pipeline) {}

void Batch::bindIndexBuffer(BufferPointer& buffer) {}
void Batch::bindVertexBuffers(uint32_t num, BufferPointer* buffers) {}

void Batch::drawIndexed(uint32_t numPrimitives, uint32_t startIndex) {}


