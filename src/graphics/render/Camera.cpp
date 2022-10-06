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

#include "gpu/Device.h"
#include "gpu/Resource.h"
#include "core/Log.h"

using namespace graphics;

Camera::Camera(CameraStructBuffer::Handle handle) :
    _camData(handle)
{
}

Camera::~Camera() {

}

void Camera::setView(const core::View& view) {
    auto [d, l] = _camData.write();
    d->_view = view;
}

core::View Camera::getView() const {
    auto [d, l] = _camData.read();
    return d->_view;
}

void Camera::setEye(const core::vec3& pos) {
    auto [d, l] = _camData.write();
    d->_view.setEye(pos);
}
core::vec3 Camera::getEye() const {
    auto [d, l] = _camData.read();
   return d->_view.eye();

}

void Camera::setOrientationFromRightUp(const core::vec3& right, const core::vec3& up) {
    auto [d, l] = _camData.write();
    d->_view.setOrientationFromRightUp(right, up);

}
void Camera::setOrientationFromFrontUp(const core::vec3& front, const core::vec3& up) {
    auto [d, l] = _camData.write();
    d->_view.setOrientationFromFrontUp(front, up);

}

core::vec3 Camera::getRight() const {
    auto [d, l] = _camData.read();
   return d->_view.right();
}
core::vec3 Camera::getUp() const {
    auto [d, l] = _camData.read();
   return d->_view.up();
}
core::vec3 Camera::getBack() const {
    auto [d, l] = _camData.read();
   return d->_view.back();
}

core::vec3 Camera::getLeft() const {
    auto [d, l] = _camData.read();
    return -d->_view.right();
}
core::vec3 Camera::getDown() const {
    auto [d, l] = _camData.read();
    return -d->_view.up();
}
core::vec3 Camera::getFront() const {
    auto [d, l] = _camData.read();
    return -d->_view.back();
}


void Camera::setProjection(const core::Projection& proj) {
    auto [d, l] = _camData.write();
    d->_projection = proj;
}
core::Projection Camera::getProjection() const {
    auto [d, l] = _camData.read();
   return d->_projection;
}

void Camera::setFocal(float focal) {
    auto [d, l] = _camData.write();
    d->_projection.setFocal(focal);

}
float Camera::getFocal() const {
    auto [d, l] = _camData.read();
   return d->_projection.focal();
}

float Camera::getFov(bool vertical) const {
    auto [d, l] = _camData.read();
    auto fovHalftan = d->_projection.fovHalfTan(vertical);
    auto fov = 2.0f * atanf(fovHalftan);
    return fov;
}
float Camera::getFovDeg(bool vertical) const {
    const float radToDeg = 180.f / std::acosf(-1);
    return radToDeg * getFov(vertical);
}

void Camera::setProjectionHeight(float projHeight) {
    auto [d, l] = _camData.write();
    d->_projection.setHeight(projHeight);
}

float Camera::getProjectionHeight() const {
    auto [d, l] = _camData.read();
   return d->_projection._height;
}

float Camera::getProjectionWidth() const {
    auto [d, l] = _camData.read();
   return d->_projection.width();
}

void Camera::setAspectRatio(float aspectRatio) {
    auto [d, l] = _camData.write();
    d->_projection.setAspectRatio(aspectRatio);

}
float Camera::getAspectRatio() const {
    auto [d, l] = _camData.read();
   return d->_projection.aspectRatio();
}
bool Camera::isLandscape() const {
    auto [d, l] = _camData.read();
   return d->_projection.isLandscape();
}

void Camera::setFar(float pfar) {
    auto [d, l] = _camData.write();
    d->_projection.setFar(pfar);

}
float Camera::getFar() const {
    auto [d, l] = _camData.read();
   return d->_projection._persFar;
}

void Camera::setOrtho(bool enable) {
    auto [d, l] = _camData.write();
    d->_projection.setOrtho(enable);
}
bool Camera::isOrtho() const {
    auto [d, l] = _camData.read();
   return d->_projection.isOrtho();
}

void Camera::setOrthoHeight(float height) {
    auto [d, l] = _camData.write();
    d->_projection.setOrthoHeight(height);

}
float Camera::getOrthoHeight() const {
    auto [d, l] = _camData.read();
   return d->_projection.orthoHeight();
}
float Camera::getOrthoWidth() const {
    auto [d, l] = _camData.read();
   return d->_projection.orthoWidth();
}
void Camera::setOrthoSide(float orthoSide, bool vertical) {
    auto [d, l] = _camData.write();
    d->_projection.setOrthoSide(orthoSide, vertical);
}

void Camera::setOrthoNear(float pnear) {
    auto [d, l] = _camData.write();
    d->_projection.setOrthoNear(pnear);
}
float Camera::getOrthoNear() const {
    auto [d, l] = _camData.read();
   return d->_projection._orthoNear;
}

void Camera::setOrthoFar(float pfar) {
    auto [d, l] = _camData.write();
    d->_projection.setOrthoFar(pfar);
}
float Camera::getOrthoFar() const {
    auto [d, l] = _camData.read();
   return d->_projection._orthoFar;
}

void Camera::setViewport(const core::ViewportRect& viewport) {
    auto [d, l] = _camData.write();
    d->_viewport = viewport;
}

core::ViewportRect Camera::getViewport() const {
    auto [d, l] = _camData.read();
   return d->_viewport;
}

void Camera::setViewport(float width, float height, bool adjustAspectRatio) {
    setViewport(0.f, 0.f, width, height, adjustAspectRatio);
}

void Camera::setViewport(float oriX, float oriY, float width, float height, bool adjustAspectRatio) {
    auto [d, l] = _camData.write();
    d->_viewport.setRect(core::vec4(0.f, 0.f, width, height));
    if (adjustAspectRatio) {
        d->_projection.setAspectRatio(d->_viewport.width() / d->_viewport.height());
    }
}

core::vec4 Camera::getViewportRect() const {
    auto [d, l] = _camData.read();
   return d->_viewport.rect();
}

float Camera::getViewportWidth() const {
    auto [d, l] = _camData.read();
   return d->_viewport.width();
}
float Camera::getViewportHeight() const {
    auto [d, l] = _camData.read();
   return d->_viewport.height();
}

float Camera::getOrthoPixelSize() const {
    auto [d, l] = _camData.read();
   return d->_projection.orthoHeight() / d->_viewport.height();
}
float Camera::getPerspPixelSize(float depth) const {
    auto [d, l] = _camData.read();
   return d->_projection.evalHeightAt(depth) / d->_viewport.height();
}

void Camera::pan(float deltaRight, float deltaUp) {
    auto [d, l] = _camData.write();
    auto& view = d->_view;
    view.setEye( view.eye() + view.right() * deltaRight + view.up() * deltaUp );
}

void Camera::dolly(float deltaBack) {
    auto [d, l] = _camData.write();
    auto oriView = d->_view;
    auto& nextView = d->_view;

    // PE is the vector from Pivot to Eye position
    auto boomVecWS = oriView.back() * (deltaBack);
    auto pivotWS = oriView.eye() + boomVecWS;

    // translate by the pivot point to recover world space
    nextView.setEye(pivotWS);

}

void Camera::orbit(float boomLength, float deltaRight, float deltaUp) {
    auto [d, l] = _camData.write();
    auto oriView = d->_view;
    auto& nextView = d->_view;

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

void Camera::orbitHorizontal(float boomLength, float deltaRight, float deltaUp) {
    auto [d, l] = _camData.write();
    d->_view.orbitHorizontal(boomLength, deltaRight, deltaUp);
}

float Camera::boom(float boomLength, float delta) {
    float nextBoomLength = boomLength + delta;
    if (nextBoomLength <= 0.0f) {
        nextBoomLength = 0.01f;
        delta = nextBoomLength - boomLength;
    }
    dolly(delta);

    return nextBoomLength;
}

float Camera::zoomTo(const core::vec4& sphere) {
    auto proj = getProjection();

    // zoom to translate the camera on the front ray toward the center of the sphere
    // at a 'distance' from the center of the sphere:
    float radius = sphere.w;

    float distance = radius;
    if (proj.isOrtho()) {
        // in ortho, the distance is the sphere radius
        distance = radius + proj._orthoNear;

        // Adjust size of the proj to fit the sphere along the shortest side
        auto v = proj.isLandscape();
        setOrthoHeight((v ? 1.0f : 1.0f / proj.aspectRatio()) * 2.0f * radius);
    } else {
        // in perspective, knowing the field of view, find the distance from the center of the sphere to see all of it
        // Compute the adjacent side of the fovHalfTan of opposite side 'sphere.radius',
        // then  the distance as the hypotenuse 
        auto fovHalfTanInv = 1.0f / proj.fovHalfTan(proj.isLandscape());
        auto distance2 = radius * radius * (fovHalfTanInv + 1.0f) * (fovHalfTanInv + 1.0f);
        distance = sqrt(distance2);

        // make sure the near is not crossing the sphere:
        auto radiusToNear = distance - radius - proj._focal;
        if (radiusToNear < 0.0f) {
             distance -= radiusToNear;
        }
    }

    // move the camera on the ray toward the sphere from the distance.
    setEye(core::vec3(sphere.x, sphere.y, sphere.z) + getBack() * distance);

    return distance;
}

void Camera::lookFrom(const core::vec3& lookDirection) {
    auto frontDir = core::normalize(lookDirection);

    // Configure the Camera to look from the required direction
    // this direction configure the front vector, we 'll assume the up vector is always y axis
    // except if look direction is actually along y... in that case up is  +/- z axis
    core::vec3 upAxis(0.f, 1.f, 0.f);
    float lookDotY = core::dot(frontDir, upAxis);
    if (lookDotY >= 1.0f || lookDotY <= -1.0f) {
        upAxis = core::vec3(0.0f, 0.0f, lookDotY);
    }
    setOrientationFromFrontUp(lookDirection, upAxis);
}

core::vec2 Camera::eyeSpaceFromImageSpace2D(float x, float y) const {
    auto [d, l] = _camData.read();
    const auto& vr = d->_viewport._rect;
    const auto& proj = d->_projection;

    auto m_nc = core::ViewportRect::normalizedSpaceFromImageSpace(vr, x, y);
    if (proj.isOrtho()) {
        return core::Projection::eyeFromClipSpace2D(proj._orthoHeight, proj._aspectRatio, m_nc);
    }
    else {
        return core::Projection::eyeFromClipSpace2D(proj._height, proj._aspectRatio, m_nc);
    }
}



void CameraStore::reserve(const DevicePointer& device, uint32_t capacity) {
    _cameraStructBuffer.reserve(device, capacity);
}

CameraPointer CameraStore::createCamera() {

    auto alloc = _indexTable.allocate();

    _cameraStructBuffer.allocate_element(alloc.index, &CameraData());

    CameraPointer camera(new Camera({ &_cameraStructBuffer, alloc.index }));

    if (alloc.recycle) {
        _cameras[alloc.index] = camera;
    } else {
        _cameras.push_back(camera);
    }

    return camera;
}


void CameraStore::syncGPUBuffer(const BatchPointer& batch) {
    _cameraStructBuffer.sync_gpu_from_cpu(batch);
}
