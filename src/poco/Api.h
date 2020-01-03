// Api.h 
//
// Sam Gateau - 2020/1/1
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

// Supposed to be included from poco.h, so this one is not needed
#include "Forward.h"
#include <iostream>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace poco {
    // Desc struct creating the api
    struct ApiInit {
#ifdef WIN32
        HINSTANCE hInstance;
#endif
    };

    // Singleton Api
    class api {
    public:
        ~api();
        static bool create(const ApiInit& init);
        static void destroy();
        static std::ostream& log(const char* file, int line, const char* functionName);

        // Factory
        static DevicePointer createDevice(const DeviceInit& init);
        static WindowPointer createWindow(const WindowInit& init);

    private: 
        static std::unique_ptr<api> _instance;
        ApiInit _init;

    };
}


