// Viewport.cpp
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
#include "Viewport.h"

#include "Scene.h"
#include "Camera.h"
#include "Renderer.h"
#include "Drawable.h"
#include "gpu/Swapchain.h"
#include "gpu/Batch.h"


using namespace graphics;

Viewport::Viewport(const ScenePointer& scene, const CameraPointer& camera, const DevicePointer& device) :
    _scene(scene),
    _camera(camera),
    _device(device)
{
    // Configure the Camera to look at the scene
    _camera->setViewport(1280.0f, 720.0f, true); // setting the viewport size, and yes adjust the aspect ratio
    _camera->setOrientationFromRightUp({ 1.f, 0.f, 0.0f }, { 0.f, 1.f, 0.f });
    _camera->setEye(_camera->getBack() * 10.0f);
    _camera->setFar(100.0f);

    // Let s allocate a gpu buffer managed by the Camera
    _camera->allocateGPUData(_device);
    //auto render = std::bind(&Viewport::_renderCallback, this);

    _renderer = std::make_shared<Renderer>(device,
         [this] (const CameraPointer& camera, const SwapchainPointer& swapchain, const DevicePointer& device, const BatchPointer& batch) {
             this->_renderCallback(camera, swapchain, device, batch); 
          });
}

Viewport::~Viewport() {

}

core::FrameTimer::Sample Viewport::lastFrameTimerSample() const {
    return _frameTimer.lastSample();
}


void Viewport::present(const SwapchainPointer& swapchain) {
    _renderer->render(_camera, swapchain);
}

void Viewport::_renderCallback(const CameraPointer& camera, const SwapchainPointer& swapchain, const DevicePointer& device, const BatchPointer& batch) {
    _frameTimer.beginFrame();

    auto currentIndex = swapchain->currentIndex();
    camera->updateGPUData();

    batch->begin(currentIndex);

    batch->resourceBarrierTransition(
        graphics::ResourceBarrierFlag::NONE,
        graphics::ResourceState::PRESENT,
        graphics::ResourceState::RENDER_TARGET,
        swapchain, currentIndex, -1);

    core::vec4 clearColor(14 / 255.f, 14.f / 255.f, 45.f / 255.f, 1.f);
    batch->clear(swapchain, currentIndex, clearColor);

    batch->beginPass(swapchain, currentIndex);

    batch->setViewport(camera->getViewportRect());
    batch->setScissor(camera->getViewportRect());

    this->renderScene(camera, swapchain, device, batch);

    batch->endPass();

    batch->resourceBarrierTransition(
        graphics::ResourceBarrierFlag::NONE,
        graphics::ResourceState::RENDER_TARGET,
        graphics::ResourceState::PRESENT,
        swapchain, currentIndex, -1);

    batch->end();

    device->executeBatch(batch);

    _frameTimer.endFrame();

    device->presentSwapchain(swapchain);
}

void Viewport::renderScene(const graphics::CameraPointer& camera, const graphics::SwapchainPointer& swapchain, const graphics::DevicePointer& device, const graphics::BatchPointer& batch) {
    for (int i = 1; i < _scene->getItems().size(); i++) {
        auto& item = _scene->getItems()[i];
        if (item.isValid() && item.isVisible()) {
            getDrawable(_scene->getItems()[i])->draw(camera, swapchain, device, batch);
        }
    }

    if (_scene->getItems().size() > 0) {
        auto item0 = _scene->getItems()[0];
        if (item0.isValid() && item0.isVisible()) {
            getDrawable(_scene->getItems()[0])->draw(camera, swapchain, device, batch);
        }
    }
}
