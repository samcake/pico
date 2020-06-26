// pico.cpp
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
#include "pico.h"

using namespace pico;

std::unique_ptr<api> api::_instance;


std::ostream& api::log(const char* file, int line, const char* functionName) {
    return std::clog << file << " - " << line << " - " << functionName << " : ";
}

void api::_assert(bool test, const char* file, int line, const char* functionName) {
    if (!test) {
        api::log(file, line, functionName);
    }
}

api::~api() {
    picoLog() << "pico api is destoyed, bye!\n";
}

bool api::create(const ApiInit& init) {
    if (_instance) {
        picoLog() << "pico::api::instance already exist, do not create a new instance and exit returning fail\n";
        return false;
    }
    if (!_instance) {
        _instance.reset(new api());
        _instance->_init = init;
    }

    return true;
}

void api::destroy() {
    if (_instance) {
        _instance.reset();
    }
}

#ifdef WIN32
#ifdef PICO_SUPPORT_MFC
HMODULE api::getResourceHandle() {
    return reinterpret_cast<HMODULE>(&__ImageBase);
}

// load from resources
std::string pico::api::loadTextResources(unsigned short resource_id)
{
    HMODULE instance = getResourceHandle();
    HRSRC hresource = FindResource(instance, MAKEINTRESOURCE(resource_id), _T("TEXT"));
    if (!hresource)
        return "";

    // load resource
    HGLOBAL hloadedresource = LoadResource(instance, hresource);
    if (!hloadedresource)
        return "";

    // lock and read 
    LPVOID plockedresource = LockResource(hloadedresource);
    if (!plockedresource)
        return "";

    DWORD resource_size = SizeofResource(instance, hresource);
    if (!resource_size)
        return "";

    return std::string((char*)plockedresource, resource_size);
}
#endif
#endif
