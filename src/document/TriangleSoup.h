// TriangleSoup.h 
//
// Sam Gateau - May 2020
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
#include <document/dllmain.h>

namespace document
{
    class TriangleSoup;
    using TriangleSoupPointer = std::shared_ptr<TriangleSoup>;

    class DOCUMENT_API TriangleSoup {
    public:
        static TriangleSoupPointer createFromPLY(const std::string& filename);

        TriangleSoup();
        ~TriangleSoup();

        // Each point attributes 
        struct Point {
            core::vec3 pos;
           // core::vec3 nor;
            core::ucvec4 color;
        };

        // A continuous array of Points
        using Points = std::vector<Point>;
        
        // A continous array of the Indices describing the triangle soup
        using Indices = std::vector<uint32_t>;
        
#pragma warning(push)
#pragma warning(disable: 4251)
        // Here is the points data
        Points _points;

        // here is the indices data
        Indices _indices;

        // A transform to place the point cloud in world space
        core::mat4x3 _transform;
#pragma warning(pop)
    };

}