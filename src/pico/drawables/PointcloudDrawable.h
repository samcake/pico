// PointCloud_Drawable.h 
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

#include "Forward.h"

namespace core {
    struct mat4x3;
}
namespace document {
    class PointCloud;
    using PointCloudPointer = std::shared_ptr<PointCloud>;
}
namespace pico {
    class DrawcallObject;
    using DrawcallObjectPointer = std::shared_ptr<DrawcallObject>;
    class Device;
    using DevicePointer = std::shared_ptr<Device>;
    class Camera;
    using CameraPointer = std::shared_ptr<Camera>;

    /*
    const pico::DrawcallObjectPointer& getDrawable(const PointCloudDrawable& x) {
        return x.getDrawable();
    }*/
    class VISUALIZATION_API PointCloudDrawable {
    public:
        PointCloudDrawable();
        ~PointCloudDrawable();
        
        pico::DrawcallObjectPointer allocateDocumentDrawcallObject(const pico::DevicePointer& device, const pico::CameraPointer& camera, const document::PointCloudPointer& pointcloud);
        pico::DrawcallObjectPointer getDrawable() const;

#pragma warning(push)
#pragma warning(disable: 4251) // class 'std::shared_ptr<pico::DrawcallObject>' needs to have dll-interface to be used by clients of class 'pico::PointCloudDrawable'
        pico::DrawcallObjectPointer _drawcall;     
#pragma warning(pop)
    };


} // !namespace pico
