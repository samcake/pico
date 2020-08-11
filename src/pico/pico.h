// pico.h 
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

#include <iostream>
#define picoLog() ::pico::api::log(__FILE__, __LINE__, __FUNCTION__)
#define picoAssert(t) ::pico::api::_assert((t), __FILE__, __LINE__, __FUNCTION__)

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


#ifdef _WINDOWS
        static HMODULE getResourceHandle();
        static std::string loadTextResources(unsigned short resource_id);
#endif

    private:
#pragma warning(push)
#pragma warning(disable: 4251)
        static std::unique_ptr<api> _instance;
#pragma warning(pop)
        ApiInit _init;

    };
}


