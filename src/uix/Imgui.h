// Imgui.h 
// Encapsulation of Dear Imgui for pico
// 
// Dear ImGui by Omar Cornut: Bloat-free Graphical User interface for C++ with minimal dependencies
// https://github.com/ocornut/imgui
// 
// Sam Gateau - July 2021
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

#include <graphics/gpu/gpu.h>
#include "Window.h"

#include "imgui/imgui.h"

namespace uix {
class Imgui {
public:

    static void create();
    static void destroy();
    static void setup(const WindowPointer& win, const graphics::DevicePointer& gpudevice);

    static void newFrame();
    static void draw(const graphics::BatchPointer& batch);

    static void createDeviceObjects();
    static void invalidateDeviceObjects();

#ifdef WIN32
    static bool customEventCallback(HWND win, UINT msg, WPARAM wparam, LPARAM lparam);
#endif
};
}
