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

void Camera::setEye(const core::vec3& pos) {
    WriteLock();
    _camData._data._view.setEye(pos);
}
core::vec3 Camera::getEye() const {
    ReadLock();
    return _camData._data._view.eye();

}

void Camera::setOrientationFromRightUp(const core::vec3& right, const core::vec3& up) {
    WriteLock();
    _camData._data._view.setOrientationFromRightUp(right, up);

}
void Camera::setOrientationFromFrontUp(const core::vec3& front, const core::vec3& up) {
    WriteLock();
    _camData._data._view.setOrientationFromFrontUp(front, up);

}

core::vec3 Camera::getRight() const {
    ReadLock();
    return _camData._data._view.right();
}
core::vec3 Camera::getUp() const {
    ReadLock();
    return _camData._data._view.up();
}
core::vec3 Camera::getBack() const {
    ReadLock();
    return _camData._data._view.back();
}

core::vec3 Camera::getLeft() const {
    ReadLock();
    return -_camData._data._view.right();
}
core::vec3 Camera::getDown() const {
    ReadLock();
    return -_camData._data._view.up();
}
core::vec3 Camera::getFront() const {
    ReadLock();
    return -_camData._data._view.back();
}


void Camera::setProjection(const Projection& proj) {
    WriteLock();
    _camData._data._projection = proj;
}
Projection Camera::getProjection() const {
    ReadLock();
    return _camData._data._projection;
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

void Camera::setViewport(const ViewportRect& viewport) {
    WriteLock();
    _camData._data._viewport = viewport;
}

ViewportRect Camera::getViewport() const {
    ReadLock();
    return _camData._data._viewport;
}

void Camera::setViewport(float width, float height, bool adjustAspectRatio) {
    setViewport(0.f, 0.f, width, height, adjustAspectRatio);
}

void Camera::setViewport(float oriX, float oriY, float width, float height, bool adjustAspectRatio) {
    WriteLock();
    _camData._data._viewport.setRect(core::vec4(0.f, 0.f, width, height));
    if (adjustAspectRatio) {
        _camData._data._projection.setAspectRatio(_camData._data._viewport.width() / _camData._data._viewport.height());
    }
}

core::vec4 Camera::getViewportRect() const {
    ReadLock();
    return _camData._data._viewport.rect();
}

float Camera::getViewportWidth() const {
    ReadLock();
    return _camData._data._viewport.width();
}
float Camera::getViewportHeight() const {
    ReadLock();
    return _camData._data._viewport.height();
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

void Camera::pan(float deltaRight, float deltaUp) {
    WriteLock();
    auto& view = _camData._data._view;
    view.setEye( view.eye() + view.right() * deltaRight + view.up() * deltaUp );
}

void Camera::boom(float deltaBack) {
    WriteLock();
    auto oriView = _camData._data._view;
    auto& nextView = _camData._data._view;

    // PE is the vector from Pivot to Eye position
    auto boomVecWS = oriView.back() * (deltaBack);
    auto pivotWS = oriView.eye() + boomVecWS;

     // translate by the pivot point to recover world space
    nextView.setEye(pivotWS);

}

void Camera::orbit(float boomLength, float deltaRight, float deltaUp) {
    WriteLock();
    auto oriView = _camData._data._view;
    auto& nextView = _camData._data._view;

    // PE is the vector from Pivot to Eye position
    auto boomVecWS = oriView.back() * (-boomLength);
    auto pivotWS = oriView.eye() + boomVecWS;

    // the initial Frame Centered on Pivot using the Camera axes
    // THen Eye expressed relative to that frame is:
    auto eyeOS = core::vec3(0.0f, 0.0f, -boomLength);

    // Rotate frame by delta right and delta up
    auto newRight = normalize(oriView.right() + oriView.back() * deltaRight);
    auto newUp = normalize( oriView.up() + oriView.back() * deltaUp);
    nextView.setOrientationFromRightUp(newRight, newUp);
    
    // Compute PE' in world space form the new frame orientation
    auto PEr = nextView.right() * eyeOS.x + nextView.up() * eyeOS.y + nextView.back() * eyeOS.z;

    // translate by the pivot point to recover world space
    nextView.setEye( pivotWS - PEr );

}

void Camera::zoomTo(const core::vec4& sphere) {
    setEye(core::vec3(sphere.x, sphere.y, sphere.z) + getBack() * sphere.w);
    setFar(10.0f * sphere.w);
}