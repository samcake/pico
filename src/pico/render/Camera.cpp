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
#include "Camera.h"

#include "../gpu/Device.h"
#include "../gpu/Resource.h"

using namespace pico;

Camera::Camera() {

}

Camera::~Camera() {

}

#define useLocks 1
#ifdef useLocks 
#define WriteLock() const std::lock_guard<std::mutex> cpulock(_camData._access); _camData._version++;
#define ReadLock() const std::lock_guard<std::mutex> cpulock(_camData._access);

#define WriteGPULock() const std::lock_guard<std::mutex> gpulock(_gpuData._access); _gpuData._version++;
#define ReadGPULock() const std::lock_guard<std::mutex> gpulock(_gpuData._access);
#else
#define WriteLock()  _camData._version++;
#define ReadLock() 

#define WriteGPULock() _gpuData._version++;
#define ReadGPULock()
#endif

void Camera::setView(const View& view) {
    WriteLock();
    _camData._data._view = view;
}

View Camera::getView() const {
    ReadLock();
    return _camData._data._view;
}

void Camera::setProjection(const Projection& proj) {
    WriteLock();
    _camData._data._projection = proj;
}
Projection Camera::getProjection() const {
    ReadLock();
    return _camData._data._projection;
}


void Camera::setEye(const vec3& pos) {
    WriteLock();
    _camData._data._view.setEye(pos);
}
vec3 Camera::getEye() const {
    ReadLock();
    return _camData._data._view.eye();

}

void Camera::setOrientation(const vec3& right, const vec3& up) {
    WriteLock();
    _camData._data._view.setOrientation(right, up);

}
vec3 Camera::getRight() const {
    ReadLock();
    return _camData._data._view.right();
}
vec3 Camera::getUp() const {
    ReadLock();
    return _camData._data._view.up();
}
vec3 Camera::getBack() const {
    ReadLock();
    return _camData._data._view.back();
}

vec3 Camera::getLeft() const {
    ReadLock();
    return -_camData._data._view.right();
}
vec3 Camera::getDown() const {
    ReadLock();
    return -_camData._data._view.up();
}
vec3 Camera::getFront() const {
    ReadLock();
    return -_camData._data._view.back();
}

void Camera::setFocal(float focal) {
    WriteLock();
    _camData._data._projection.setFocal(focal);

}
float Camera::getFocal() const {
    ReadLock();
    return _camData._data._projection._focal;
}

void Camera::setProjectionHeight(float projHeight) {
    WriteLock();
    _camData._data._projection.setHeight(projHeight);
}

float Camera::getProjectionHeight() const {
    ReadLock();
    return _camData._data._projection._height;
}

float Camera::getProjectionWidth() const {
    ReadLock();
    return _camData._data._projection.width();
}

void Camera::setAspectRatio(float aspectRatio) {
    WriteLock();
    _camData._data._projection.setAspectRatio(aspectRatio);

}
float Camera::getAspectRatio() const {
    ReadLock();
    return _camData._data._projection._aspectRatio;
}

void Camera::setFar(float pfar) {
    WriteLock();
    _camData._data._projection.setFar(pfar);

}
float Camera::getFar() const {
    ReadLock();
    return _camData._data._projection._far;
}

void Camera::allocateGPUData(const DevicePointer& device) {
    // CReate a gpu buffer to hold the camera
    pico::BufferInit uboInit;
    uboInit.usage = ResourceUsage::UNIFORM_BUFFER;
    uboInit.bufferSize = sizeof(CameraData);
    uboInit.hostVisible = true;
    _gpuData._buffer = device->createBuffer(uboInit);

    // and then copy data there
    ReadLock();
    memcpy(_gpuData._buffer->_cpuMappedAddress, &_camData._data, sizeof(CameraData));

    // sync the data version
    _gpuData._version = _camData._version;
}

bool Camera::updateGPUData() {
    // TODO make the version an atomic to be thread safe...
    bool needCopy = (_gpuData._version != _camData._version);
     // copy data from tcpu to gpu here if versions are different
    if (needCopy) {
        ReadLock();
        WriteGPULock();
        memcpy(_gpuData._buffer->_cpuMappedAddress, &_camData._data, sizeof(CameraData));

        // sync the data version
        _gpuData._version = _camData._version;
    }
    return needCopy;
}

BufferPointer Camera::getGPUBuffer() const {
    ReadGPULock();
    return _gpuData._buffer;
}


#include "../Window.h"

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

    float time = min(0.2f, (0.001f * duration.count()));

    float translationSpeed = 2.0f;
    float rotationSpeed = 0.5f;

    auto view = _cam->getView();
    auto projection = _cam->getProjection();

    vec3 back = view.back();
    vec3 right = view.right();
    vec3 up = view.up();

    vec3 translation{ 0.0f };
    translation = translation + back * -(control._translateFront - control._translateBack) * translationSpeed * time;
    translation = translation + right * (control._translateRight - control._translateLeft) * translationSpeed * time;

    view.setEye(view.eye() + translation);

    
    float rotation = - (control._rotateLeft - control._rotateRight) * rotationSpeed * time;
    if (rotation != 0) {
        vec3 rotatedRight = right + back * rotation;
        view.setOrientation(rotatedRight, up);
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

    return false;
}