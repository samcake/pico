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

#include <mutex>

#include <core/LinearAlgebra.h>
#include "render.h"


namespace pico {

    
    struct VISUALIZATION_API View {
#pragma warning(push)
#pragma warning(disable: 4251)
        core::mat4x3 _mat;
#pragma warning(pop)

        core::vec3 right() const {
            return _mat._columns[0];
        }
        core::vec3 up() const {
            return _mat._columns[1];
        }
        core::vec3 back() const {
            return _mat._columns[2];
        }
        core::vec3 eye() const {
            return _mat._columns[3];
        }

        void setOrientationFromRightUp(const core::vec3& right, const core::vec3& up) {
            _mat._columns[0] = normalize(right); // make sure Right is normalized
            _mat._columns[2] = normalize(cross(_mat._columns[0], normalize(up))); // compute Back as normalize(Right^Up)
            _mat._columns[1] = normalize(cross(_mat._columns[2], _mat._columns[0])); // make sure Up is orthogonal to XZ and normalized
        }

        void setOrientationFromFrontUp(const core::vec3& front, const core::vec3& up) {
            _mat._columns[2] = -normalize(front); // make sure Front (-Back) is normalized
            _mat._columns[0] = normalize(cross(normalize(up), _mat._columns[2])); // compute Right as normalize(Up^Back)
            _mat._columns[1] = normalize(cross(_mat._columns[2], _mat._columns[0])); // make sure Up is orthogonal to XZ and normalized
        }

        void setEye(const core::vec3& eyePos) {
            _mat._columns[3] = eyePos;
        }

        
        static core::vec3 worldFromEyeSpaceDir(const core::vec3& right, const core::vec3& up, const core::vec3& eyeDir) {
            return eyeDir * eyeDir.x + up * eyeDir.y + cross(right, up) * eyeDir.z;
        }
        static core::vec3 worldFromEyeSpace(const core::vec3& eye, const core::vec3& right, const core::vec3& up, const core::vec3& eyePos) {
            return worldFromEyeSpaceDir(right, up, eyePos) + eye;
        }

        static core::vec3 eyeFromWorldSpace(const core::vec3& eye, const core::vec3& right, const core::vec3& up, const core::vec3& worldPos) {
            core::vec3 eyeCenteredPos = worldPos - eye;
            return core::vec3(
                dot(right, eyeCenteredPos),
                dot(up, eyeCenteredPos),
                dot(cross(right, up), eyeCenteredPos)
            );
        }

    };

    struct VISUALIZATION_API Projection {
        float _aspectRatio{ 16.0f / 9.0f }; // aspectRatio = width / height of the projection rectangle
        float _height{ 0.056f }; // height of the projection rectangle at the focal plane, similar to the sensor height
        float _focal{ 0.056f };  // focal length between the focal plane and the origin of the projection
        float _persFar{ 100.0f }; // ersepective far, Infinite if 0f

        float _orthoEnabled { 0.0f }; 
        float _orthoHeight{ 1.0f };
        float _orthoNear{ 0.0f };
        float _orthoFar { 100.0f };

        void setAspectRatio(float aspectRatio) {
            _aspectRatio = core::max(core::FLOAT_EPSILON, aspectRatio);
        }
        void setHeight(float height) {
            _height = core::max(core::FLOAT_EPSILON, height);
        }

        void setFocal(float focal) {
            _focal = core::max(core::FLOAT_EPSILON, focal);
        }
        void setFar(float pfar) {
            _persFar = core::max(core::FLOAT_EPSILON, pfar);
        }

        float width() const { return _aspectRatio * _height; }

        void setOrtho(bool enable) {
            _orthoEnabled = (enable ? 1.0f : 0.0f);
        }
        bool isOrtho() const { return (_orthoEnabled > 0.0f); }
        void setOrthoHeight(float height) {
            _orthoHeight = core::max(core::FLOAT_EPSILON, height);
        }
        void setOrthoNear(float orthoNear) {
            _orthoNear = orthoNear;
        }
        void setOrthoFar(float orthoFar) {
            _orthoFar = orthoFar;
        }


        static core::vec3 eyeFromClipSpace(float focal, float sensorHeight, float aspectRatio, const core::vec2& clipPos) {
            return core::vec3(clipPos.x * aspectRatio * sensorHeight * 0.5f, clipPos.y * sensorHeight * 0.5f, -focal);
        }

        static float depthClipFromEyeSpace(float pnear, float pfar, float eyeZ) {
            // Standard z mapping [n, f] to [0, 1]
            // float b = pfar / (pfar - pnear);
            // float a = -pnear * b;
            // Inverted z mapping [n, f] to [1, 0], need depth function "greater than"
            // float b = pnear / (pnear- pfar);
            // float a = -pfar * b;

            // Infinite far mapping [n, infinity] to [0, 1]
            // float b = 1.0f; //lim at far  infinite of  pfar / (pfar - pnear);
            // float a = -pnear * b;
            // Infinite far inverted Z
            float b = 0.0f; //lim at far  infinite of  pnear / (pnear- pfar);;
            float a = pnear; // lim at far infinite of - pfar * pnear / (pnear - pfar);

            // float clipW = eyeZ;
            float clipZ = a + b * eyeZ;
            // float depthBuffer = clipZ/clipW = a * (1/eyeZ) + b;
            return clipZ;
        }

        static core::vec4 persClipFromEyeSpace(float aspectRatio, float sensorHeight, float focal, float pfar, const core::vec3& eyePos) {
            float ez = -eyePos.z;
            float pnear = focal;
            core::vec4 clipPos;
            clipPos.w = ez;
            clipPos.z = depthClipFromEyeSpace(pnear, pfar, ez);
            clipPos.x = eyePos.x * pnear * 2.0f / (sensorHeight * aspectRatio);
            clipPos. y = eyePos.y * pnear * 2.0f / (sensorHeight);
            return clipPos;
        }

        static core::vec4 orthoClipFromEyeSpace(float aspectRatio, float sensorHeight, float pnear, float pfar, const core::vec3& eyePos) {
            core::vec4 clipPos;
            clipPos.w = 1.0f;
            clipPos.z = (pfar - eyePos.z) / (pfar - pnear);
            clipPos.x = eyePos.x * 2.0f / (sensorHeight * aspectRatio);
            clipPos.y = eyePos.y * 2.0f / (sensorHeight);
            return clipPos;
        }

        
    };

    struct VISUALIZATION_API ViewportRect {
#pragma warning(push)
#pragma warning(disable: 4251)
        core::vec4 _rect{ 0.f, 0.f, 1280.f, 720.f}; // the rect of the viewport origin XY and Width Z & Height W
#pragma warning(pop)

        void setRect(const core::vec4& r) {
            // check that resolution is 1 at minimum
            _rect.x = r.x;
            _rect.y = r.y;
            _rect.z = core::max(1.0f, r.z);
            _rect.w = core::max(1.0f, r.w);
        }

        core::vec4 rect() const { return _rect; }

        core::vec2 origin() const { return core::vec2(_rect.x, _rect.y); }
        float originX() const { return _rect.x; }
        float originY() const { return _rect.y; }

        core::vec2 size() const { return core::vec2(_rect.z, _rect.w); }
        float width() const { return _rect.z; }
        float height() const { return _rect.w; }
    };

    struct VISUALIZATION_API CameraData {
        View _view;
        Projection _projection;
        ViewportRect _viewport;
    };

    class VISUALIZATION_API Camera {
        struct CameraCPU {
            uint32_t _version{ 0xFFFFFFFF };
            mutable std::mutex _access;
            CameraData _data;
        };

        struct CameraGPU {
            uint32_t _version{ 0xFFFFFFFF };
            mutable std::mutex _access;
            BufferPointer _buffer;
        };

        CameraCPU _camData;
        CameraGPU _gpuData;

    public:
        Camera();
        ~Camera();

        // View
        void setView(const View& view);
        View getView() const;

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
        void setProjection(const Projection& proj);
        Projection getProjection() const;

        void setFocal(float focal);
        float getFocal() const;
        float getFov() const;
        float getFovDeg() const;

        void setProjectionHeight(float projHeight);
        float getProjectionHeight() const;
        float getProjectionWidth() const;

        void setAspectRatio(float aspectRatio);
        float getAspectRatio() const;

        void setFar(float far);
        float getFar() const;

        void setOrtho(bool enable);
        bool isOrtho() const;

        void setOrthoHeight(float orthoHeight);
        float getOrthoHeight() const;
        void setOrthoNear(float orthoNear);
        float getOrthoNear() const;
        void setOrthoFar(float orthoFar);
        float getOrthoFar() const;

        // Viewport
        void setViewport(const ViewportRect& viewport);
        ViewportRect getViewport() const;

        void setViewport(float width, float height, bool adjustAspectRatio);
        void setViewport(float oriX, float oriY, float width, float height, bool adjustAspectRatio);
        core::vec4 getViewportRect() const;
        float getViewportWidth() const;
        float getViewportHeight() const;

        // Gpu version of the camera Data
        void allocateGPUData(const DevicePointer& device);
        bool updateGPUData();
        BufferPointer getGPUBuffer() const;

        // Some nice moves
        void pan(float deltaRight, float deltaUp);
        void dolly(float deltaBack);
        void orbit(float boomLength, float deltaRight, float deltaUp);

        float boom(float boomLength, float delta);
        void zoomTo(const core::vec4& sphere);
        void lookFrom(const core::vec3& lookDirection);
    };
}
