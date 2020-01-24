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

#include <functional>
#include "mas.h"

namespace pico {

//--------------------------------------------------------------------------------------

enum Key : uint8_t {
    KEY_0 = 0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_NOPE,

    KEY_LMB,
    KEY_RMB,
    KEY_MMB,
    KEY_SPACE,
    KEY_A,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,

    KEY_COUNT,
};

struct ResizeEvent {
};
struct PaintEvent {
};
struct MouseEvent {
    vec2 pos;
    bool state;
    bool dblclick;
    
};
struct KeyboardEvent {
    Key key;
    bool state;
};

class WindowHandler {
public:
    WindowHandler() {}
    virtual ~WindowHandler() {}

    virtual void onResize(const ResizeEvent & e) {}
    virtual void onPaint(const PaintEvent& e) {}
    virtual void onMouse(const MouseEvent& e) {}
    virtual void onKeyboard(const KeyboardEvent& e) {}

};

class WindowHandlerDelegate : public pico::WindowHandler {
public:
    std::function<void(const pico::ResizeEvent&)> _onResizeDelegate;
    std::function<void(const pico::PaintEvent&)> _onPaintDelegate;
    std::function<void(const pico::MouseEvent&)> _onMouseDelegate;
    std::function<void(const pico::KeyboardEvent&)> _onKeyboardDelegate;

    WindowHandlerDelegate() {
    }

    void onResize(const pico::ResizeEvent& e) override {
        if (_onResizeDelegate) _onResizeDelegate(e);
    }
    void onPaint(const pico::PaintEvent& e) override {
        if (_onPaintDelegate) _onPaintDelegate(e);
    }
    void onMouse(const pico::MouseEvent& e) override {
        if (_onMouseDelegate) _onMouseDelegate(e);
    }
    void onKeyboard(const pico::KeyboardEvent& e) override {
        if (_onKeyboardDelegate) _onKeyboardDelegate(e);
    }
};

//--------------------------------------------------------------------------------------

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

    void onResize(const ResizeEvent & e);
    void onPaint(const PaintEvent& e);
    void onMouse(const MouseEvent& e);
    void onKeyboard(const KeyboardEvent& e);

protected:
    std::unique_ptr<WindowHandler> _handler;
    std::unique_ptr<WindowBackend> _backend;
};
}