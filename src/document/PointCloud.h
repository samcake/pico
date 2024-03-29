// PointCloud.h 
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

#include <string>
#include <vector>
#include <memory>
#include <core/math/Math3D.h>
#include "dllmain.h"

namespace document
{
    class PointCloud;
    using PointCloudPointer = std::shared_ptr<PointCloud>;

    class DOCUMENT_API PointCloud {
    public:
        static PointCloudPointer createFromJSON(const std::string& filename);
        static PointCloudPointer createFromPLY(const std::string& filename);

        PointCloud();
        ~PointCloud();

        // Each point attributes 
        struct Point {
            core::vec3 pos;
           // core::vec3 nor;
            core::ucvec4 color;
        };

        // A continuous array of Points
        using Points = std::vector<Point>;

        const std::string& getName() const { return _name; }

#pragma warning(push)
#pragma warning(disable: 4251)
        Points _points;

        // A transform to place the point cloud in world space
        core::mat4x3 _transform;

        // name assigned to the document
        std::string _name;

#pragma warning(pop)
    };

}
