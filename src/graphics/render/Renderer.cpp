// Renderer.cpp
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
#include "Renderer.h"

#include "gpu/Device.h"
#include "gpu/Batch.h"
#include "gpu/Swapchain.h"

using namespace graphics;

Renderer::Renderer(const DevicePointer& device, RenderCallback callback) :
    _device(device),
    _callback(callback)
{
    graphics::BatchInit batchInit{};
    _batch= _device->createBatch(batchInit);

}

Renderer::~Renderer() {

}

void Renderer::render(const CameraPointer& camera, const SwapchainPointer& swapchain) {
    if (_callback) {
        _callback(camera, swapchain, _device, _batch);
    } else {
        auto currentIndex = swapchain->currentIndex();

        _batch->begin(currentIndex);

        _batch->resourceBarrierTransition(
            graphics::ResourceBarrierFlag::NONE,
            graphics::ResourceState::PRESENT,
            graphics::ResourceState::RENDER_TARGET,
            swapchain, currentIndex, -1);

        static float time = 0.0f;
        time += 1.0f/60.0f;
        time = core::frac(time);
        core::vec4 clearColor(colorRGBfromHSV(core::vec3(time, 0.5f, 1.f)), 1.f);

        _batch->clear(swapchain, currentIndex, clearColor);

        _batch->resourceBarrierTransition(
            graphics::ResourceBarrierFlag::NONE,
            graphics::ResourceState::RENDER_TARGET,
            graphics::ResourceState::PRESENT,
            swapchain, currentIndex, -1);

        _batch->end();

        _device->executeBatch(_batch);

        _device->presentSwapchain(swapchain);
    }
}

