// Window.h 
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

namespace poco {


    // Window concrete backend implementation
    class WindowBackend {
    protected:
        Window* _ownerWindow;
        WindowBackend(Window* owner) : _ownerWindow(owner) {}

    public:
        virtual ~WindowBackend() {}

        virtual bool messagePump() = 0;
        virtual void* nativeWindow() = 0;
    };

    class WindowHandler {
        public:
            WindowHandler() {}
        virtual ~WindowHandler() {}

        virtual void onPaint() {}

    };

    struct WindowInit {
        WindowHandler* handler {nullptr};
    };

    class Window {
    public:
        Window(WindowHandler* handler);
        ~Window();

        bool messagePump();

        void* nativeWindow() {
            return _backend->nativeWindow();
        }

        void onPaint();


    protected:
        std::unique_ptr<WindowHandler> _handler;
        std::unique_ptr<WindowBackend> _backend;
    };
}