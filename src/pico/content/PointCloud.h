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
#ifndef _POINTCLOUD_H_
#define _POINTCLOUD_H_

#include <Forward.h>

#include <vector>

#include <core/LinearAlgebra.h>


namespace document
{
    class PointCloud;
    using PointCloudPointer = std::shared_ptr<PointCloud>;

    class VISUALIZATION_API PointCloud {
    public:
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


#pragma warning(push)
#pragma warning(disable: 4251)
        // Here is the PointCloud data
        Points _points;

        // A transform to place the point cloud in world space
        core::mat4x3 _transform;
#pragma warning(pop)
    };

}

#endif