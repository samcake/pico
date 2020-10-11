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

#include <functional>
#include <vector>

#include <core/math/LinearAlgebra.h>
#include <core/api.h>

#include "dllmain.h"


namespace uix {

    //--------------------------------------------------------------------------------------
    class Window;
    using WindowPointer = std::shared_ptr<Window>;
    struct WindowInit;

//--------------------------------------------------------------------------------------

enum Key : uint8_t {
    KEY_NOPE = 0,

    KEY_LMB,
    KEY_RMB,
    KEY_MMB,
    KEY_BACK,
    KEY_TAB,
    KEY_CLEAR,
    KEY_RETURN,
    KEY_SHIFT,
    KEY_CONTROL,
    KEY_ALT,
    KEY_PAUSE,
    KEY_CAPSLOCK,
    KEY_ESC,
    KEY_SPACE,
    KEY_PAGEUP,
    KEY_PAGEDOWN,
    KEY_END,
    KEY_HOME,
    KEY_LEFT,
    KEY_UP,
    KEY_RIGHT,
    KEY_DOWN,
    KEY_PRINT,
    KEY_SNAPSHOT,
    KEY_INSERT,
    KEY_DELETE,
    KEY_HELP,

    KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,

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

    KEY_LOS,
    KEY_ROS,

    KEY_MULTIPLY,
    KEY_ADD,
    KEY_SEPARATOR,
    KEY_SUBSTRACT,
    KEY_DECIMAL,
    KEY_DIVIDE,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_F13,
    KEY_F14,
    KEY_F15,
    KEY_F16,
    KEY_F17,
    KEY_F18,
    KEY_F19,
    KEY_F20,
    KEY_F21,
    KEY_F22,
    KEY_F23,
    KEY_F24,

    KEY_NUMLOCK,
    KEY_SCROLL,
    KEY_LSHIFT,
    KEY_RSHIFT,
    KEY_LCONTROL,
    KEY_RCONTROL,
    KEY_LALT,
    KEY_RALT,

    KEY_COUNT,
};

enum MouseState : uint8_t {
    MOUSE_LBUTTON = 0x01,
    MOUSE_RBUTTON = 0x02,
    MOUSE_MBUTTON = 0x04,

    MOUSE_SHIFT = 0x10,
    MOUSE_CONTROL = 0x20,

    MOUSE_MOVE = 0x40,
    MOUSE_WHEEL = 0x80,
};

struct ResizeEvent {
    uint32_t width;
    uint32_t height;
    bool over{ false };
};
struct PaintEvent {
};
struct MouseEvent {
    core::vec2 pos;
    core::vec2 delta; // (moving ? pos - last mouse event pos : 0,0)
    float  wheel;
    uint8_t state; // bitfield of MouseState 
};

struct KeyboardEvent {
    Key key;
    bool state;
};

struct DropFilesEvent {
    std::vector<std::string> fileUrls;
};

class WindowHandler {
public:
    WindowHandler() {}
    virtual ~WindowHandler() {}

    virtual void onResize(const ResizeEvent & e) {}
    virtual void onPaint(const PaintEvent& e) {}
    virtual void onMouse(const MouseEvent& e) {}
    virtual void onKeyboard(const KeyboardEvent& e) {}
    virtual void onDropFiles(const DropFilesEvent& e) {}

};

class WindowHandlerDelegate : public uix::WindowHandler {
public:
    std::function<void(const uix::ResizeEvent&)> _onResizeDelegate;
    std::function<void(const uix::PaintEvent&)> _onPaintDelegate;
    std::function<void(const uix::MouseEvent&)> _onMouseDelegate;
    std::function<void(const uix::KeyboardEvent&)> _onKeyboardDelegate;
    std::function<void(const uix::DropFilesEvent&)> _onDropFilesDelegate;

    WindowHandlerDelegate() {
    }

    void onResize(const uix::ResizeEvent& e) override {
        if (_onResizeDelegate) _onResizeDelegate(e);
    }
    void onPaint(const uix::PaintEvent& e) override {
        if (_onPaintDelegate) _onPaintDelegate(e);
    }
    void onMouse(const uix::MouseEvent& e) override {
        if (_onMouseDelegate) _onMouseDelegate(e);
    }
    void onKeyboard(const uix::KeyboardEvent& e) override {
        if (_onKeyboardDelegate) _onKeyboardDelegate(e);
    }
    void onDropFiles(const uix::DropFilesEvent& e) override {
        if (_onDropFilesDelegate) _onDropFilesDelegate(e);
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
    virtual uint32_t width() const = 0;
    virtual uint32_t height() const = 0;

    virtual void setTitle(const std::string& title) = 0;
};


struct WindowInit {
    WindowHandler* handler {nullptr};
    std::string title;
};

class Window {
    Window(WindowHandler* handler);
public:

    static WindowPointer createWindow(const WindowInit& init);

    ~Window();

    bool messagePump();

    void* nativeWindow() {
        return _backend->nativeWindow();
    }

    uint32_t width() const;
    uint32_t height() const;

    void onResize(const ResizeEvent & e);
    void onPaint(const PaintEvent& e);
    void onMouse(const MouseEvent& e);
    void onKeyboard(const KeyboardEvent& e);
    void onDropFiles(const DropFilesEvent& e);

    void setTitle(const std::string& title);
protected:
    std::unique_ptr<WindowHandler> _handler;
    std::unique_ptr<WindowBackend> _backend;
};
}