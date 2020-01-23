// Api.cpp
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
#include "Api.h"

#include "gpu/Device.h"
#include "Window.h"

using namespace poco;

std::unique_ptr<api> api::_instance;


std::ostream& api::log(const char* file, int line, const char* functionName) {
    return std::clog << file << " - " << line << " - " << functionName << " : ";
}

void api::assert(bool test, const char* file, int line, const char* functionName) {
    if (!test) {
        api::log(file, line, functionName);
    }
}

api::~api() {
    pocoLog() << "poco api is destoyed, bye!\n";
}

bool api::create(const ApiInit& init) {
    if (_instance) {
        pocoLog() << "poco::api::instance already exist, do not create a new instance and exit returning fail\n";
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

DevicePointer api::createDevice(const DeviceInit& init) {
    DevicePointer device(new Device());

    return device;
}


WindowPointer api::createWindow(const WindowInit& init) {
    WindowPointer window(new Window((init.handler ? init.handler : new WindowHandler())));

    return window;
}
