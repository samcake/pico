// Api.h 
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

#ifdef WIN32
#ifdef PICO_SUPPORT_MFC
#include "stdafx.h"
#else
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#endif

namespace pico {
    // Desc struct creating the api
    struct ApiInit {
    };

    // Singleton Api
    class VISUALIZATION_API api {
    public:
        ~api();
        static bool create(const ApiInit& init);
        static void destroy();
        static std::ostream& log(const char* file, int line, const char* functionName);
        static void _assert(bool test, const char* file, int line, const char* functionName);


#ifdef WIN32
        static HMODULE getResourceHandle();
#endif

    private: 
#pragma warning(push)
#pragma warning(disable: 4251)
        static std::unique_ptr<api> _instance;
#pragma warning(pop)
        ApiInit _init;

    };
}


