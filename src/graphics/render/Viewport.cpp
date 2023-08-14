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
#include "Sky.h"
#include "Camera.h"
#include "Renderer.h"
#include "Draw.h"
#include "gpu/Swapchain.h"
#include "gpu/Batch.h"
#include "gpu/Query.h"


using namespace graphics;

// Allocate a rootLayout just for the scene descriptor set
const DescriptorSetLayout Viewport::viewPassLayout = {
    { graphics::DescriptorType::UNIFORM_BUFFER, graphics::ShaderStage::VERTEX, 11, 1},  // Sky
    { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::ALL_GRAPHICS, 19, 1},  // Camera
    { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 20, 1}, // Node Transform
    { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::ALL_GRAPHICS, 21, 1}, // Timer
};

Viewport::Viewport(const ViewportInit& init) :
    _scene(init.scene),
    _device(init.device),
    _postSceneRC(init.postSceneRC),
    _cameraID(init.cameraID)
{
    _batchTimer = _device->createBatchTimer({});

    _renderer = std::make_shared<Renderer>(_device,
         [this] (RenderArgs& args) {
             this->_renderCallback(args);
          },
         [this](AnimateArgs& args) {
              this->_animateCallback(args);
          });

    DescriptorSetInit dsInit = {
        nullptr,
        0, false,
        viewPassLayout
    };
    _viewPassDescriptorSet = _device->createDescriptorSet(dsInit);

    DescriptorObjects descriptorObjects = {
        { graphics::DescriptorType::UNIFORM_BUFFER, _scene->_sky->getGPUBuffer() },
        { graphics::DescriptorType::RESOURCE_BUFFER, _scene->_cameras.getGPUBuffer() },
        { graphics::DescriptorType::RESOURCE_BUFFER, _scene->_nodes.getNodeTransformGPUBuffer()},
        { graphics::DescriptorType::RESOURCE_BUFFER, _batchTimer->getBuffer() },
    };
    _device->updateDescriptorSet(_viewPassDescriptorSet, descriptorObjects);

}

Viewport::~Viewport() {

}

core::FrameTimer::Sample Viewport::lastFrameTimerSample() const {
    return _frameTimer.lastSample();
}


void Viewport::animate(float time) {
    _renderer->animate(time);
}

void Viewport::_animateCallback(AnimateArgs& args) {
    args.scene = _scene;

    auto itemInfos = _scene->_items.fetchItemInfos();
    
    for (int i = 0; i < itemInfos.size(); i++) {
        const auto& info = itemInfos[i];
        if (info.isValid() && info.isVisible() && info.isAnim()) {
             _scene->_anims.animate(info._animID, info._nodeID, args);
        }
    }
}

void Viewport::present(const SwapchainPointer& swapchain) {
    _renderer->render(_scene->getCamera(_cameraID), swapchain);
}

void Viewport::_renderCallback(RenderArgs& args) {
    _frameTimer.beginFrame();

    auto currentIndex = args.swapchain->currentIndex();

    args.batch->begin(currentIndex, _batchTimer);

    // 
    syncSceneResourcesForFrame(_scene, args.batch);

    args.batch->resourceBarrierTransition(
        graphics::ResourceBarrierFlag::NONE,
        graphics::ResourceState::PRESENT,
        graphics::ResourceState::RENDER_TARGET,
        args.swapchain, currentIndex, -1);

   // core::vec4 clearColor(14 / 255.f, 14.f / 255.f, 45.f / 255.f, 1.f);
    core::vec4 clearColor(128.f / 255.f, 128.f / 255.f, 128.f / 255.f, 1.f);
    args.batch->clear(args.swapchain, currentIndex, clearColor);

    args.batch->beginPass(args.swapchain, currentIndex);

    args.batch->setViewport(args.swapchain->viewportRect());
    args.batch->setScissor(args.swapchain->viewportRect());

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
    args.scene = _scene;

    auto itemInfos = _scene->_items.fetchItemInfos();

    for (int i = 0; i < itemInfos.size(); i++) {
        const auto& info = itemInfos[i];
        if (info.isValid() && info.isVisible() && info.isDraw()) {
        //    auto drawcall = _scene->_drawables.getDrawcall(info._drawID);
            _scene->_drawables.getDrawcall(info._drawID)(info._nodeID, args);
        }
    }
/*
    if (itemInfos.size() > 0) {
        const auto& info = itemInfos[0];
        if (info.isValid() && info.isVisible() && info.isDraw()) {
   //         auto drawcall = _scene->_drawables.getDrawcall(info._drawID);
            _scene->_drawables.getDrawcall(info._drawID)(info._nodeID, args);
        }
    }*/
}
