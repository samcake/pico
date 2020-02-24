// Camera.cpp
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
#include "CameraController.h"

#include "../render/Camera.h"
#include "../gpu/Device.h"
#include "../gpu/Resource.h"
#include "Window.h"

using namespace pico;


CameraController::CameraController(const CameraPointer& cam) : _cam(cam) {

}

void CameraController::update(std::chrono::milliseconds& duration) {
    // copy the control data and excute on this
    ControlData data = _controlData;
    updateCameraFromController(data, duration);
}

void CameraController::updateCameraFromController(CameraController::ControlData& control,
    std::chrono::milliseconds& duration) {

    if (!_cam) return;

    float time = core::min(0.2f, (0.001f * duration.count()));

    float translationSpeed = 2.0f;
    float rotationSpeed = 0.5f;

    auto view = _cam->getView();
    auto projection = _cam->getProjection();

    core::vec3 back = view.back();
    core::vec3 right = view.right();
    core::vec3 up = view.up();

    core::vec3 translation{ 0.0f };
    translation = translation + back * -(control._translateFront - control._translateBack) * translationSpeed * time;
    translation = translation + right * (control._translateRight - control._translateLeft) * translationSpeed * time;

    view.setEye(view.eye() + translation);

    
    float rotation = - (control._rotateLeft - control._rotateRight) * rotationSpeed * time;
    if (rotation != 0) {
        core::vec3 rotatedRight = right + back * rotation;
        view.setOrientationFromRightUp(rotatedRight, up);
    }

    float focalChange = -(control._zoomOut - control._zoomIn) * 0.1f * time;
    if (focalChange != 0) {
        projection.setFocal(projection._focal + focalChange);
    }

    _cam->setView(view);
    _cam->setProjection(projection);

    // fade out
    float fadeOut = 0.1f * time;
    _controlData._translateFront *= fadeOut;
    _controlData._translateBack *= fadeOut;
    _controlData._translateLeft *= fadeOut;
    _controlData._translateRight *= fadeOut;
    _controlData._rotateLeft = 0.0f;
    _controlData._rotateRight = 0.0f;
    _controlData._zoomIn *= fadeOut;
    _controlData._zoomOut *= fadeOut;
}


bool CameraController::onKeyboard(const KeyboardEvent& e) {

    if (e.key == Key::KEY_W) {
        if (e.state) {
            _controlData._translateFront = 1.0f;
        } else {
            _controlData._translateFront = 0.0f;
        }
    }
    if (e.key == Key::KEY_S) {
        if (e.state) {
            _controlData._translateBack = 1.0f;
        } else {
            _controlData._translateBack = 0.0f;
        }
    }
    if (e.key == Key::KEY_Q) {
        if (e.state) {
            _controlData._translateLeft = 1.0f;
        } else {
            _controlData._translateLeft = 0.0f;
        }
    }
    if (e.key == Key::KEY_E) {
        if (e.state) {
            _controlData._translateRight = 1.0f;
        } else {
            _controlData._translateRight = 0.0f;
        }
    }
    if (e.key == Key::KEY_A) {
        if (e.state) {
            _controlData._rotateLeft = 1.0f;
        } else {
            _controlData._rotateLeft = 0.0f;
        }
    }
    if (e.key == Key::KEY_D) {
        if (e.state) {
            _controlData._rotateRight = 1.0f;
        } else {
            _controlData._rotateRight = 0.0f;
        }
    }

    if (e.key == Key::KEY_I) {
        if (e.state) {
            _controlData._zoomIn = 1.0f;
        } else {
            _controlData._zoomIn = 0.0f;
        }
    }
    if (e.key == Key::KEY_O) {
        if (e.state) {
            _controlData._zoomOut = 1.0f;
        } else {
            _controlData._zoomOut = 0.0f;
        }
    }

    return false;
}

bool CameraController::onMouse(const MouseEvent& e) {
    return false;
}

bool CameraController::onResize(const ResizeEvent& e) {
    // aspect ratio changes with a resize always
    // viewport resolution, only once the resize is over
    if (!e.over) {
        _cam->setAspectRatio(e.width/ (float) e.height);
    } else {
        _cam->setViewport(e.width, e.height, true);
    }

    return false;
}