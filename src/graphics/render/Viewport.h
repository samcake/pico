// Viewport.h 
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

#include "render.h"

#include <core/Realtime.h>
#include "Renderer.h"

#include <gpu/Descriptor.h>

namespace graphics {

    struct VISUALIZATION_API ViewportInit {
        ScenePointer    scene;
        DevicePointer   device;
        RenderCallback  postSceneRC = nullptr;
        CameraID        cameraID = 0;
    };

    class VISUALIZATION_API Viewport {
    public:
        Viewport(const ViewportInit& init);
        ~Viewport();

        void setCamera(CameraID camID) { _cameraID = camID; }
        CameraID getCamera() const { return _cameraID; }

        void present(const SwapchainPointer& swapchain);

        core::FrameTimer::Sample lastFrameTimerSample() const;

        static const DescriptorSetLayout viewPassLayout;

        void animate(float time);

    protected:
        void _renderCallback(RenderArgs& args);
        void _animateCallback(AnimateArgs& args);

        void renderScene(RenderArgs& args);

        ScenePointer _scene;
        DevicePointer _device;
        RendererPointer _renderer;

        RenderCallback _postSceneRC;

        // Measuring framerate
        core::FrameTimer _frameTimer;
        BatchTimerPointer _batchTimer;

        DescriptorSetPointer _viewPassDescriptorSet;

        // Bag of interesting constants for the Viewport ?
        CameraID _cameraID = 0;

    };
}
