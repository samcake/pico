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

#include <graphics/render/Camera.h>
#include <graphics/gpu/Device.h>
#include <graphics/gpu/Resource.h>
#include "Window.h"

using namespace uix;


CameraController::CameraController(const graphics::CameraPointer& cam, bool orthoNorthUp) : _cam(cam), _orthoNorthUp( orthoNorthUp ) {

}

void CameraController::update(std::chrono::microseconds& duration) {
    // copy the control data and excute on this
    ControlData data = _controlData;
    updateCameraFromController(data, duration);
}

void CameraController::updateCameraFromController(CameraController::ControlData& control,
    std::chrono::microseconds& duration) {

    if (!_cam) return;

    float time = core::min(0.2f, (0.000001f * duration.count()));

    float translationSpeed = 50.0f;
    float rotationSpeed = 30.0f;

    auto view = _cam->getView();
    bool view_changed = false;
    auto projection = _cam->getProjection();
    bool projection_changed = false;

    if (control._translateFront || control._translateBack || control._translateRight - control._translateLeft) {
        auto deltaLong = -(control._translateFront - control._translateBack) * translationSpeed * time;
        auto deltaLat = (control._translateRight - control._translateLeft) * translationSpeed * time;

        if (deltaLat) {
            _cam->pan(deltaLat, 0);
        }
        if (deltaLong) {
            _cam->dolly(deltaLong);
        }
    }
    
    float rotation = - (control._rotateLeft - control._rotateRight) * rotationSpeed * time;
    if (rotation != 0) {
        _cam->orbit(_controlData._boomLength, rotation, 0);
    }

    float focalChange = -(control._zoomOut - control._zoomIn) * 0.1f * time;
    if (focalChange != 0) {
        projection.setFocal(projection._focal + focalChange);
    }


    if (projection_changed) {
        _cam->setProjection(projection);
    }

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
    if (e.key == Key::KEY_A && _orthoNorthUp) {
        if (e.state) {
            _controlData._rotateLeft = 1.0f;
        } else {
            _controlData._rotateLeft = 0.0f;
        }
    }
    if (e.key == Key::KEY_D && _orthoNorthUp) {
        if (e.state) {
            _controlData._rotateRight = 1.0f;
        } else {
            _controlData._rotateRight = 0.0f;
        }
    }

    return false;
}

bool CameraController::onMouse(const MouseEvent& e) {

    if (e.state & uix::MOUSE_MOVE) {
        if (e.state & uix::MOUSE_LBUTTON) {
        }
        if (e.state & uix::MOUSE_MBUTTON) {
            float panScale = _controlData._boomLength * 0.001f;
            if (_cam->isOrtho()) {
                panScale = _cam->getOrthoHeight() / _cam->getViewportHeight();
            }
            else {
            }
            _cam->pan(-e.delta.x * panScale, e.delta.y * panScale);
        }
        if (e.state & uix::MOUSE_RBUTTON) {
            if ((e.state & uix::MOUSE_CONTROL) || (_cam->isOrtho() && _orthoNorthUp)) {
                float panScale = _controlData._boomLength * 0.001f;
                if (_cam->isOrtho()) {
                    panScale = _cam->getOrthoHeight() / _cam->getViewportHeight();
                }
                else {
                }
                _cam->pan(-e.delta.x * panScale, e.delta.y * panScale);
            }
            else {
                    float orbitScale = 0.01f;
                    _cam->orbit(_controlData._boomLength, orbitScale * (float)e.delta.x, orbitScale * (float)-e.delta.y);
            }
        }
    }
    else if (e.state & uix::MOUSE_WHEEL) {
        if (e.state & uix::MOUSE_CONTROL) {
            if (_cam->isOrtho()) {
                float dollyScale = -0.08f;
                float orbitLengthDelta = (e.wheel * dollyScale) * _controlData._boomLength;
                _controlData._boomLength = _cam->boom(_controlData._boomLength, orbitLengthDelta);
            }
            else {
                float zoomScale = 0.1f;
                _cam->setFocal(_cam->getFocal() * (1.0f + e.wheel * zoomScale));
            }
        }
        else {
            if (_cam->isOrtho()) {
                float zoomScale = 0.1f;
                auto m_e1 = _cam->eyeSpaceFromImageSpace2D(e.pos.x, _cam->getViewport().height() - e.pos.y);

                _cam->setOrthoHeight(_cam->getOrthoHeight() * (1.0f + e.wheel * zoomScale));

                auto m_e2 = _cam->eyeSpaceFromImageSpace2D(e.pos.x, _cam->getViewport().height() - e.pos.y);
                
                _cam->pan(m_e1.x - m_e2.x, m_e1.y - m_e2.y);
            }
            else {
                float dollyScale = 0.05f;
                float orbitLengthDelta = (e.wheel * dollyScale) * _controlData._boomLength;
                _controlData._boomLength = _cam->boom(_controlData._boomLength, orbitLengthDelta);
            }
        }
    }

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

float CameraController::zoomTo(const core::vec4& sphere) {
    _controlData._boomLength = _cam->zoomTo(sphere);
    return _controlData._boomLength;
}