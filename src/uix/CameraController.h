// CameraController.h 
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

#include <chrono>
#include <core/math/LinearAlgebra.h>
#include <graphics/render/render.h>

namespace uix {

    // Camera Controller connects standard inputs (keyboard and mouse) to drive the camera

    struct KeyboardEvent;
    struct MouseEvent;
    struct ResizeEvent;
    class CameraController {

        graphics::CameraPointer _cam;
    public:
        CameraController(const graphics::CameraPointer& cam);

        struct ControlData {
            float _translateFront{ 0 };
            float _translateBack{ 0 };

            float _translateLeft{ 0 };
            float _translateRight{ 0 };

            float _rotateLeft{ 0 };
            float _rotateRight{ 0 };

            float _zoomIn{ 0 };
            float _zoomOut{ 0 };

            float _boomLength{ 1.0f };
        };

        ControlData _controlData;

        void updateCameraFromController(ControlData& control, std::chrono::microseconds& duration);

        void update(std::chrono::microseconds& duration);

        bool onKeyboard(const KeyboardEvent& e);
        bool onMouse(const MouseEvent& e);
        bool onResize(const ResizeEvent& e);

        float zoomTo(const core::vec4& sphere);

    };
}