// Renderer.cpp
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
#include "Renderer.h"

#include "Device.h"
#include "Batch.h"
#include "Swapchain.h"

using namespace poco;

Renderer::Renderer(const DevicePointer& device) :
    _device(device)
{
    poco::BatchInit batchInit{};
    _batch= _device->createBatch(batchInit);

}

Renderer::~Renderer() {

}

void Renderer::render(const CameraPointer& camera, SwapchainPointer& swapchain) {
    auto currentIndex = swapchain->currentIndex();

    _batch->begin(currentIndex);

    _batch->resourceBarrierTransition(
        poco::Batch::BarrierFlag::NONE,
        poco::Batch::ResourceState::PRESENT,
        poco::Batch::ResourceState::RENDER_TARGET,
        swapchain, currentIndex, -1);

    static float time = 0.0f;
    time += 1.0f/60.0f;
    float intPart;
    time = modf(time, &intPart);
    poco::vec4 clearColor(colorRGBfromHSV(vec3(time, 0.5f, 1.f)), 1.f);

    _batch->clear(clearColor, swapchain, currentIndex);

    _batch->resourceBarrierTransition(
        poco::Batch::BarrierFlag::NONE,
        poco::Batch::ResourceState::RENDER_TARGET,
        poco::Batch::ResourceState::PRESENT,
        swapchain, currentIndex, -1);

    _batch->end();

    _device->executeBatch(_batch);

    _device->presentSwapchain(swapchain);

}
