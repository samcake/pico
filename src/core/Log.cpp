// Log.cpp 
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
#include "Log.h"
#include <iostream>

#include <codecvt>
#include <locale>

void core::Log::_output(const char* message) {
    std::clog << message << std::endl;
}

void core::Log::_log(const char* file, int line, const char* functionName, const char* message, int level) {
    std::clog << /*file << " - " << line << " - " << */ functionName << " : " << message << std::endl;
}

void core::Log::_assert(bool test, const char* file, int line, const char* functionName, const char* message) {
    if (!test) {
        Log::_log(file, line, functionName, message);
    }
}

void core::Log::_error(const char* file, int line, const char* functionName, const char* message, int level) {
    std::clog << /*file << " - " << line << " - " << */ functionName << " : " << message << std::endl;
    std::clog << "******** ERROR ^^^^^^^^^ " << std::endl;
}



using convert_t = std::codecvt_utf8<wchar_t>;

std::string core::to_string(std::wstring wstr) {
    static std::wstring_convert<convert_t, wchar_t> strconverter;
    return strconverter.to_bytes(wstr);
}

std::wstring core::to_wstring(std::string str) {
    static std::wstring_convert<convert_t, wchar_t> strconverter;
    return strconverter.from_bytes(str);
}
