// Log.h 
//
// Sam Gateau - September 2020
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
#include <format>

#include "dllmain.h"

#define picoError(message) ::core::Log::_error(__FILE__, __LINE__, __FUNCTION__, message)
#define picoOut(...) ::core::Log::_output(std::format(__VA_ARGS__))
#define picoLog(message) ::core::Log::_log(__FILE__, __LINE__, __FUNCTION__, message)
#define picoLogf(...) ::core::Log::_log(__FILE__, __LINE__, __FUNCTION__, std::format(__VA_ARGS__))
#define picoAssert(t) ::core::Log::_assert((t), __FILE__, __LINE__, __FUNCTION__, #t)

namespace core {
    class CORE_API Log {
        public:
        static void _output(const char* message);
        static void _output(const std::string& message) { _output(message.c_str()); }
        static void _log(const char* file, int line, const char* functionName, const char* message, int level = 0);
        static void _log(const char* file, int line, const char* functionName, const std::string& message, int level = 0) { _log(file, line, functionName, message.c_str(), level); }
        static void _assert(bool test, const char* file, int line, const char* functionName, const char* message);

        static void _error(const char* file, int line, const char* functionName, const char* message, int level = 0);
        static void _error(const char* file, int line, const char* functionName, const std::string& message, int level = 0) { _error(file, line, functionName, message.c_str(), level); }

    };

    extern std::string to_string(std::wstring wstr);
    extern std::wstring to_wstring(std::string str);
}