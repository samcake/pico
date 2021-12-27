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
#include "gpu/Query.h"


using namespace graphics;

// Allocate a rootLayout just for the scene descriptor set
const DescriptorSetLayout Viewport::viewPassLayout = {
    { graphics::DescriptorType::UNIFORM_BUFFER, graphics::ShaderStage::VERTEX, 10, 1},
    { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 20, 1}, // Node Transform
    { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::ALL_GRAPHICS, 21, 1}, // Timer
};

Viewport::Viewport(const ScenePointer& scene, const CameraPointer& camera, const DevicePointer& device, RenderCallback postSceneRC) :
    _scene(scene),
    _camera(camera),
    _device(device),
    _postSceneRC(postSceneRC)
{
    // Configure the Camera to look at the scene
    _camera->setViewport(1280.0f, 720.0f, true); // setting the viewport size, and yes adjust the aspect ratio
    _camera->setOrientationFromRightUp({ 1.f, 0.f, 0.0f }, { 0.f, 1.f, 0.f });
    _camera->setEye(_camera->getBack() * 10.0f);
    _camera->setFar(100.0f);

    // Let s allocate a gpu buffer managed by the Camera
    _camera->allocateGPUData(_device);
    //auto render = std::bind(&Viewport::_renderCallback, this);


    _batchTimer = _device->createBatchTimer({});

    _renderer = std::make_shared<Renderer>(device,
         [this] (RenderArgs& args) {
             this->_renderCallback(args);
          });



    RootDescriptorLayoutPointer rootLayout = _device->createRootDescriptorLayout({
    {
    { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::VERTEX, 1, 48},
    },
    { viewPassLayout },
    {}
    });
    _viewPassRootLayout = rootLayout;

    DescriptorSetInit dsInit = {
        nullptr,
        0, false,
        viewPassLayout
    };
    _viewPassDescriptorSet = _device->createDescriptorSet(dsInit);

    DescriptorObjects descriptorObjects = {
        { graphics::DescriptorType::UNIFORM_BUFFER, _camera->getGPUBuffer() },
        { graphics::DescriptorType::RESOURCE_BUFFER, _scene->_nodes._transforms_buffer },
        { graphics::DescriptorType::RESOURCE_BUFFER, _batchTimer->getBuffer() },
    };
    _device->updateDescriptorSet(_viewPassDescriptorSet, descriptorObjects);

}

Viewport::~Viewport() {

}

core::FrameTimer::Sample Viewport::lastFrameTimerSample() const {
    return _frameTimer.lastSample();
}


void Viewport::present(const SwapchainPointer& swapchain) {
    _renderer->render(_camera, swapchain);
}

void Viewport::_renderCallback(RenderArgs& args) {
    _frameTimer.beginFrame();

    auto currentIndex = args.swapchain->currentIndex();
    args.camera->updateGPUData();

    args.batch->begin(currentIndex, _batchTimer);

    args.batch->resourceBarrierTransition(
        graphics::ResourceBarrierFlag::NONE,
        graphics::ResourceState::PRESENT,
        graphics::ResourceState::RENDER_TARGET,
        args.swapchain, currentIndex, -1);

   // core::vec4 clearColor(14 / 255.f, 14.f / 255.f, 45.f / 255.f, 1.f);
    core::vec4 clearColor(128.f / 255.f, 128.f / 255.f, 128.f / 255.f, 1.f);
    args.batch->clear(args.swapchain, currentIndex, clearColor);

    args.batch->beginPass(args.swapchain, currentIndex);

    args.batch->setViewport(args.camera->getViewportRect());
    args.batch->setScissor(args.camera->getViewportRect());

    this->renderScene(args);


    if (_postSceneRC) {
        this->_postSceneRC(args);
    }

    args.batch->endPass();

    args.batch->resourceBarrierTransition(
        graphics::ResourceBarrierFlag::NONE,
        graphics::ResourceState::RENDER_TARGET,
        graphics::ResourceState::PRESENT,
        args.swapchain, currentIndex, -1);

    args.batch->end();

    args.device->executeBatch(args.batch);

    _frameTimer.endFrame();

    args.device->presentSwapchain(args.swapchain);
}

void Viewport::renderScene(RenderArgs& args) {
    args.timer = _batchTimer;
    args.viewPassDescriptorSet = _viewPassDescriptorSet;

    for (int i = 1; i < _scene->getItems().size(); i++) {
        auto& item = _scene->getItems()[i];
        if (item.isValid() && item.isVisible()) {
            auto drawable = item.getDrawableID();
            if (drawable != INVALID_DRAWABLE_ID) {
                auto drawcall = _scene->_drawables.getDrawcall(drawable);
                drawcall(item.getNodeID(), args);
            }
        }
    }

    if (_scene->getItems().size() > 0) {
        auto item0 = _scene->getItems()[0];
        if (item0.isValid() && item0.isVisible()) {
            auto drawable = item0.getDrawableID();
            if (drawable != INVALID_DRAWABLE_ID) {
                auto drawcall = _scene->_drawables.getDrawcall(drawable);
                drawcall(item0.getNodeID(), args);
            }
        }
    }
}
