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

#include "../Forward.h"
#include "../mas.h"

#include <mutex>

namespace pico {

    
    struct View {
        mat4x3 _mat;

        vec3 right() const {
            return _mat._columns[0];
        }
        vec3 up() const {
            return _mat._columns[1];
        }
        vec3 back() const {
            return _mat._columns[2];
        }
        vec3 eye() const {
            return _mat._columns[3];
        }

        void setOrientation(const vec3& right, const vec3& up) {
           /* _mat._columns[0] = normalize(right); // make sure Right is normalized
            _mat._columns[2] = normalize(cross(right, up)); // compute Back as normalize(Right^Up)
            _mat._columns[1] = cross(_mat._columns[2], _mat._columns[0]); // make sure Up is orthogonal
*/
            _mat._columns[0] = normalize(right); // make sure Right is normalized
            _mat._columns[1] = normalize(up); // make sure Up is normalized
            _mat._columns[2] = normalize(cross(right, up)); // compute Back as normalize(Right^Up)
         //   _mat._columns[1] = cross(_mat._columns[2], _mat._columns[0]); // make sure Up is normalized

        }

        void setEye(const vec3& eyePos) {
            _mat._columns[3] = eyePos;
        }

        
        static vec3 worldFromEyeSpaceDir(const vec3& right, const vec3& up, const vec3& eyeDir) {
            return eyeDir * eyeDir.x + up * eyeDir.y + cross(right, up) * eyeDir.z;
        }
        static vec3 worldFromEyeSpace(const vec3& eye, const vec3& right, const vec3& up, const vec3& eyePos) {
            return worldFromEyeSpaceDir(right, up, eyePos) + eye;
        }

        static vec3 eyeFromWorldSpace(const vec3& eye, const vec3& right, const vec3& up, const vec3& worldPos) {
            vec3 eyeCenteredPos = worldPos - eye;
            return vec3(
                dot(right, eyeCenteredPos),
                dot(up, eyeCenteredPos),
                dot(cross(right, up), eyeCenteredPos)
            );
        }

    };

    struct Projection {
        float _focal{ 0.056f };  // focal length between the focal plane and the origin of the projection
        float _height{ 0.056f }; // height of the projection rectangle at the focal plane, similar to the sensor height
        float _aspectRatio{ 16.0f / 9.0f }; // aspectRatio = width / height of the projection rectangle
        float _far{ 10.0f };

        void setFocal(float focal) {
            _focal = max(FLOAT_EPSILON, focal);
        }
        void setHeight(float height) {
            _height = max(FLOAT_EPSILON, height);
        }
        void setAspectRatio(float aspectRatio) {
            _aspectRatio = max(FLOAT_EPSILON, aspectRatio);
        }
        void setFar(float pfar) {
            _far = max(FLOAT_EPSILON, pfar);
        }

        float width() const { return _aspectRatio * _height; }

        static vec3 eyeFromClipSpace(float focal, float sensorHeight, float aspectRatio, const vec2& clipPos) {
            return vec3(clipPos.x * aspectRatio * sensorHeight * 0.5, clipPos.y * sensorHeight * 0.5, -focal);
        }

        static vec4 clipFromEyeSpace(float focal, float sensorHeight, float aspectRatio, float pfar, const vec3& eyePos) {
            float foc = focal;
            float w = foc - eyePos.z;
            float z = (-eyePos.z) * pfar / (pfar - foc);
            float x = eyePos.x * foc / (0.5f * sensorHeight * aspectRatio);
            float y = eyePos.y * foc / (0.5f * sensorHeight);
            return vec4(x, y, z, w);
        }
    };


    struct CameraData {
        View _view;
        Projection _projection;
    };

    class Camera {
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

        void setView(const View& view);
        View getView() const;

        void setProjection(const Projection& proj);
        Projection getProjection() const;

        void setEye(const vec3& pos);
        vec3 getEye() const;

        void setOrientation(const vec3& right, const vec3& up);
        vec3 getRight() const;
        vec3 getUp() const;
        vec3 getBack() const;
        vec3 getLeft() const;
        vec3 getDown() const;
        vec3 getFront() const;

        void setFocal(float focal);
        float getFocal() const;

        void setProjectionHeight(float projHeight);
        float getProjectionHeight() const;
        float getProjectionWidth() const;

        void setAspectRatio(float aspectRatio);
        float getAspectRatio() const;

        void setFar(float far);
        float getFar() const;

        // Gpu version of the camera Data
        void allocateGPUData(const DevicePointer& device);
        bool updateGPUData();
        BufferPointer getGPUBuffer() const;

    };
}