// Camera.h 
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

#include <core/math/CameraTransform.h>
#include <core/stl/IndexTable.h>

#include "render.h"
#include "gpu/StructuredBuffer.h"

namespace graphics {

    using CameraID = core::IndexTable::Index;
    using CameraIDs = core::IndexTable::Indices;
    static const CameraID INVALID_CAMERA_ID = core::IndexTable::INVALID_INDEX;

    struct VISUALIZATION_API CameraData {
        core::View _view;
        core::Projection _projection;
        core::ViewportRect _viewport;
    };

    struct VISUALIZATION_API CameraInit {
        CameraData cam;
    };

    using CameraStructBuffer = StructuredBuffer<CameraData>;

    class VISUALIZATION_API Camera {
    protected:

        // Camera is created from the CameraStore
        friend class CameraStore;
        Camera(CameraStructBuffer::Handle handle);

        CameraStructBuffer::Handle _camData;

    public:

        virtual ~Camera(); // not really expecting to derive Camera but who knows...

        inline CameraID id() const { return _camData._index; }

        // View
        void setView(const core::View& view);
        core::View getView() const;

        void setEye(const core::vec3& pos);
        core::vec3 getEye() const;

        void setOrientationFromRightUp(const core::vec3& right, const core::vec3& up);
        void setOrientationFromFrontUp(const core::vec3& front, const core::vec3& up);
        core::vec3 getRight() const;
        core::vec3 getUp() const;
        core::vec3 getBack() const;
        core::vec3 getLeft() const;
        core::vec3 getDown() const;
        core::vec3 getFront() const;

        // Projection
        void setProjection(const core::Projection& proj);
        core::Projection getProjection() const;

        void setFocal(float focal);
        float getFocal() const;
        float getFov(bool vertical = true) const;
        float getFovDeg(bool vertical = true) const;

        void setProjectionHeight(float projHeight);
        float getProjectionHeight() const;
        float getProjectionWidth() const;

        void setAspectRatio(float aspectRatio);
        float getAspectRatio() const;
        bool isLandscape() const; // true if aspect ratio > 1.0;

        void setFar(float far);
        float getFar() const;

        void setOrtho(bool enable);
        bool isOrtho() const;

        void setOrthoHeight(float orthoHeight);
        float getOrthoHeight() const;
        float getOrthoWidth() const;
        void setOrthoSide(float orthoSide, bool vertical);

        void setOrthoNear(float orthoNear);
        float getOrthoNear() const;
        void setOrthoFar(float orthoFar);
        float getOrthoFar() const;

        // Viewport
        void setViewport(const core::ViewportRect& viewport);
        core::ViewportRect getViewport() const;

        void setViewport(float width, float height, bool adjustAspectRatio);
        void setViewport(float oriX, float oriY, float width, float height, bool adjustAspectRatio);
        core::vec4 getViewportRect() const;
        float getViewportWidth() const;
        float getViewportHeight() const;

        // Eval size of a pixel
        // in ortho constant
        float getOrthoPixelSize() const;
        // in perspective at specified depth (positive in front of the camera)
        float getPerspPixelSize(float depth) const;

        // Some nice relative moves
        void pan(float deltaRight, float deltaUp);
        void dolly(float deltaBack);
        void orbit(float boomLength, float deltaRight, float deltaUp);
        void orbitHorizontal(float boomLength, float deltaRight, float deltaUp); // Orbit but keep the view horizontal, X axis is in horizontal plane
        float boom(float boomLength, float delta);

        // zoom to a sphere
        // translate the camera (keeping the current look direction and focal)
        // so the camera points to the center of the sphere at enough distance to fit the full sphere in the viewport
        // the distance from the eye to the center of the sphere is returned
        float zoomTo(const core::vec4& sphere);
        void lookFrom(const core::vec3& lookDirection);

        // convert between spaces

        // from 2d pos in the image space of the viewport to the 2d pos in eye space in the clipping plane
        core::vec2 eyeSpaceFromImageSpace2D(float x, float y) const;
    };
    using Camera_wp = std::weak_ptr<Camera>;

    class VISUALIZATION_API CameraStore
    {
    protected:
        core::IndexTable _indexTable;

        CameraStructBuffer _cameraStructBuffer;

        std::vector<Camera_wp>  _cameras;

        friend class Camera;
    public:
        void reserve(const DevicePointer& device, uint32_t capacity);

        CameraPointer createCamera();

        inline CameraPointer getCamera(CameraID index) const { return _cameras[index].lock(); }

        inline BufferPointer getGPUBuffer() const { return _cameraStructBuffer.gpu_buffer(); }
        void syncGPUBuffer(const BatchPointer& batch);
    };
}
