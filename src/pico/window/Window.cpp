// Window.cpp
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
#include "Window.h"

#include <map>
#include <iostream>

using namespace pico;

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Winuser.h>
#include <Windowsx.h>

class WIN32WindowBackend : public WindowBackend {
public:
    static const uint8_t _NumKeyCodes{ 0xA6 };
    static const Key _KeyCodesToKey[_NumKeyCodes];

    HWND _sysWindow{ nullptr };
    uint32_t _width{ 0 };
    uint32_t _height{ 0 };
    core::vec2 _lastMouseEventPos{ 0.f };
    bool _unknownLastMousePos{ true };
    bool _didResize{ false };

    using SysWindowMap = std::map<HWND, WIN32WindowBackend*>;
    static SysWindowMap _sysWindowMap;

    static HMODULE getThisModuleHandle() {
        //Returns module handle where this function is running in: EXE or DLL
        HMODULE hModule = NULL;
        ::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCTSTR)getThisModuleHandle, &hModule);

        return hModule;
    }

    static std::string WINDOW_CLASS;
    static void initWinClass() {
        if (WINDOW_CLASS.empty()) {
            WINDOW_CLASS = "WIN_WINDOW";

            WNDCLASS wcex;
            wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
            wcex.lpfnWndProc = WIN32WindowBackend::windowProc;
            wcex.lpszClassName = "WIN_WINDOW";
            wcex.cbClsExtra = 0;
            wcex.cbWndExtra = 0;
            wcex.hInstance = getThisModuleHandle();
            wcex.hIcon = NULL;
            wcex.hCursor = NULL;
            wcex.hbrBackground = (HBRUSH)COLOR_BTNSHADOW;
            wcex.lpszMenuName = NULL;
            wcex.style = 0;

            ATOM atom = RegisterClass(&wcex);
            if (!atom) {
                std::cerr << "registerClass - Failed to create WindowClass" << std::endl;
            }
        }
    }
    static LRESULT CALLBACK windowProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
        switch (msg) {
        case WM_CREATE:
            createCallback(window);
            return 0;

        case WM_DESTROY:
            destroyCallback(window);
            PostQuitMessage(0);
            return 0;
        default:
            // Any other event, try to find the destination window and pass it on
            auto windowMapIt = _sysWindowMap.find(window);
            if (windowMapIt != _sysWindowMap.end()) {
                auto window = (*windowMapIt).second;
                if (window) {
                    return window->eventCallback(msg, wparam, lparam);
                }
            }
            break;
        }

        // If nothing catched default 
        return DefWindowProcA(window, msg, wparam, lparam);
    }

    static void createCallback(HWND syswindow) {
        std::cout << "creating this window" << std::endl;
    }
    static void destroyCallback(HWND syswindow) {
        _sysWindowMap.erase(syswindow);
        std::cout << "destroying that window" << std::endl;
    }

    LRESULT eventCallback(UINT msg, WPARAM wparam, LPARAM lparam) {
        switch (msg) {
        // These 2 events will never be caught here
        // case WM_CREATE:
        // case WM_DESTROY:

        // Pass on events to standard handler
     //   case WM_ENTERSIZEMOVE:
        case WM_SIZE: {
            ResizeEvent e { LOWORD(lparam), HIWORD(lparam) };
            _width = e.width;
            _height = e.height;
            _didResize = true;
            _ownerWindow->onResize({e});
           return 0;
        } break;
        case WM_EXITSIZEMOVE: {
            if (_didResize) {
                ResizeEvent e{ _width, _height, true };
                _ownerWindow->onResize({ e });
            }
            _didResize = false;
            return 0;
        } break;
        case WM_PAINT: {
            PaintEvent e{};
            _ownerWindow->onPaint(e);
            return 0;
        } break;
        case WM_MOUSEWHEEL: {
            core::vec2 newPos = { (float)GET_X_LPARAM(lparam),(float)GET_Y_LPARAM(lparam) };
            if (_unknownLastMousePos) {
                _lastMouseEventPos = newPos;
                _unknownLastMousePos = false;
            }
            auto wheelDelta = GET_WHEEL_DELTA_WPARAM(wparam) / (float) WHEEL_DELTA;
            MouseEvent e{ newPos, core::vec2(0.f), wheelDelta,
                              (uint8_t) ( (wparam & MK_CONTROL ? MouseState::MOUSE_CONTROL : 0)
                                        | (wparam & MK_SHIFT ? MouseState::MOUSE_SHIFT : 0)
                                        | (wparam & MK_LBUTTON ? MouseState::MOUSE_LBUTTON : 0)
                                        | (wparam & MK_RBUTTON ? MouseState::MOUSE_RBUTTON : 0)
                                        | (wparam & MK_MBUTTON ? MouseState::MOUSE_MBUTTON : 0))
                                        | MouseState::MOUSE_WHEEL };
           _ownerWindow->onMouse(e);
        } break;
        case WM_MOUSEMOVE: {
            core::vec2 newPos = { (float)GET_X_LPARAM(lparam),(float)GET_Y_LPARAM(lparam) };
            if (_unknownLastMousePos) {
                _lastMouseEventPos = newPos;
                _unknownLastMousePos = false;
            }
            MouseEvent e{ newPos, newPos - _lastMouseEventPos, 0,
                  (uint8_t) ( (wparam & MK_CONTROL ? MouseState::MOUSE_CONTROL : 0)
                            | (wparam & MK_SHIFT ? MouseState::MOUSE_SHIFT : 0)
                            | (wparam & MK_LBUTTON ? MouseState::MOUSE_LBUTTON : 0)
                            | (wparam & MK_RBUTTON ? MouseState::MOUSE_RBUTTON : 0)
                            | (wparam & MK_MBUTTON ? MouseState::MOUSE_MBUTTON : 0)  ) 
                            | MouseState::MOUSE_MOVE };
            _lastMouseEventPos = newPos;
            _ownerWindow->onMouse(e);
            return 0;
        } break;
        case WM_KEYDOWN: {
            if (wparam < _NumKeyCodes) {
            KeyboardEvent e{ _KeyCodesToKey[wparam], true };
            _ownerWindow->onKeyboard(e);
            }
            return 0;
        } break;
        case WM_KEYUP: {
            if (wparam < _NumKeyCodes) {
                KeyboardEvent e{ _KeyCodesToKey[wparam], false };
                _ownerWindow->onKeyboard(e);
            }
            return 0;
        } break;
        }

        // If nothing catched default 
        return DefWindowProcA(_sysWindow, msg, wparam, lparam);
    }

    static WIN32WindowBackend* create(Window* owner, const std::string& name = "", int32_t width = -1, int32_t height = -1) {
        initWinClass();
        width = (width == -1 ? 640 : width);
        height = (height == -1 ? 480 : height);
        auto sysWindow = CreateWindowA(WINDOW_CLASS.c_str(), name.c_str(),
            WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
            width, height, NULL, NULL, NULL, NULL);

        auto window = new WIN32WindowBackend(owner, sysWindow, name, width, height);

        _sysWindowMap[sysWindow] = window;
    
        return window;
    }

    WIN32WindowBackend(Window* owner, HWND sysWindow, const std::string& name = "", int32_t width = -1, int32_t height = -1) :
        WindowBackend(owner),
        _sysWindow(sysWindow), 
        _width(width), _height(height) {
    }

    ~WIN32WindowBackend() {
        DestroyWindow(_sysWindow);
    }

    bool runMessagePumpStep() {
        bool running = true;
        MSG msg;
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
            {
                running = false;
                break;
            }

            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        return running;
    }

    bool messagePump() override {
        return runMessagePumpStep();
    }

    void* nativeWindow() override { return _sysWindow; }
    uint32_t width() const override { return _width; }
    uint32_t height() const override { return _height; }

};

WIN32WindowBackend::SysWindowMap WIN32WindowBackend::_sysWindowMap;
std::string WIN32WindowBackend::WINDOW_CLASS;

const Key WIN32WindowBackend::_KeyCodesToKey[] = {
    KEY_NOPE,
    KEY_LMB, //  VK_LBUTTON
    KEY_RMB, //  VK_RBUTTON
    KEY_NOPE, //  VK_CANCEL
    KEY_MMB, //  VK_MBUTTON
    KEY_NOPE, // VK_XBUTTON1
    KEY_NOPE, // VK_XBUTTON2
    KEY_NOPE, //  0x07 : reserved
    KEY_BACK, // VK_BACK
    KEY_TAB, // VK_TAB
    KEY_NOPE, // 0x0A : reserved
    KEY_NOPE, // 0x0B : reserved
    KEY_CLEAR, // VK_CLEAR
    KEY_RETURN, // VK_RETURN
    KEY_NOPE, // 0x0E : reserved
    KEY_NOPE, // 0x0F : reserved
    KEY_SHIFT, // VK_SHIFT          0x10
    KEY_CONTROL, // VK_CONTROL        0x11
    KEY_ALT, // VK_MENU           0x12
    KEY_PAUSE, // VK_PAUSE          0x13
    KEY_CAPSLOCK, // VK_CAPITAL        0x14
    KEY_NOPE, // VK_KANA           0x15
    KEY_NOPE, // 0x16 : unassigned
    KEY_NOPE, // VK_JUNJA          0x17
    KEY_NOPE, // VK_FINAL          0x18
    KEY_NOPE, // VK_HANJA          0x19

    KEY_NOPE, // 0x1A : unassigned
    KEY_ESC, // VK_ESCAPE         0x1B
    KEY_NOPE, // VK_CONVERT        0x1C
    KEY_NOPE, // VK_NONCONVERT     0x1D
    KEY_NOPE, // VK_ACCEPT         0x1E
    KEY_NOPE, // VK_MODECHANGE     0x1F

    KEY_SPACE, // VK_SPACE          0x20
    KEY_PAGEUP, // VK_PRIOR          0x21
    KEY_PAGEDOWN, // VK_NEXT           0x22
    KEY_END, // VK_END            0x23
    KEY_HOME, // VK_HOME           0x24
    KEY_LEFT, // VK_LEFT           0x25
    KEY_UP, // VK_UP             0x26
    KEY_RIGHT, // VK_RIGHT          0x27
    KEY_DOWN, // VK_DOWN           0x28
    KEY_NOPE, // VK_SELECT         0x29
    KEY_PRINT, // VK_PRINT          0x2A
    KEY_NOPE, // VK_EXECUTE        0x2B
    KEY_SNAPSHOT, // VK_SNAPSHOT       0x2C
    KEY_INSERT, // VK_INSERT         0x2D
    KEY_DELETE, // VK_DELETE         0x2E
    KEY_HELP, // VK_HELP           0x2F
    KEY_0, //  * VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
    KEY_1, //  * VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
    KEY_2, //  * VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
    KEY_3, //  * VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
    KEY_4, //  * VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
    KEY_5, //  * VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
    KEY_6, //  * VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
    KEY_7, //  * VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
    KEY_8, //  * VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
    KEY_9, //  * VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
    KEY_NOPE, // 0x3A - 0x40 : unassigned
    KEY_NOPE, // 0x3B : unassigned
    KEY_NOPE, // 0x3C : unassigned
    KEY_NOPE, // 0x3D : unassigned
    KEY_NOPE, // 0x3E : unassigned
    KEY_NOPE, // 0x3F : unassigned
    KEY_NOPE, // 0x40 : unassigned
    KEY_A, //  VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
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

    KEY_LOS, // VK_LWIN           0x5B
    KEY_ROS, // VK_RWIN           0x5C
    KEY_NOPE, // VK_APPS           0x5D
    KEY_NOPE, //           0x5E

    KEY_NOPE, // VK_SLEEP          0x5F

    KEY_NOPE, // VK_NUMPAD0        0x60
    KEY_NOPE, // VK_NUMPAD1        0x61
    KEY_NOPE, // VK_NUMPAD2        0x62
    KEY_NOPE, // VK_NUMPAD3        0x63
    KEY_NOPE, // VK_NUMPAD4        0x64
    KEY_NOPE, // VK_NUMPAD5        0x65
    KEY_NOPE, // VK_NUMPAD6        0x66
    KEY_NOPE, // VK_NUMPAD7        0x67
    KEY_NOPE, // VK_NUMPAD8        0x68
    KEY_NOPE, // VK_NUMPAD9        0x69

    KEY_MULTIPLY, // VK_MULTIPLY       0x6A
    KEY_ADD, // VK_ADD            0x6B
    KEY_SEPARATOR, // VK_SEPARATOR      0x6C
    KEY_SUBSTRACT, // VK_SUBTRACT       0x6D
    KEY_DECIMAL, // VK_DECIMAL        0x6E
    KEY_DIVIDE, // VK_DIVIDE         0x6F
    KEY_F1, // VK_F1             0x70
    KEY_F2, // VK_F2             0x71
    KEY_F3, // VK_F3             0x72
    KEY_F4, // VK_F4             0x73
    KEY_F5, // VK_F5             0x74
    KEY_F6, // VK_F6             0x75
    KEY_F7, // VK_F7             0x76
    KEY_F8, // VK_F8             0x77
    KEY_F9, // VK_F9             0x78
    KEY_F10, // VK_F10            0x79
    KEY_F11, // VK_F11            0x7A
    KEY_F12, // VK_F12            0x7B
    KEY_F13, // VK_F13            0x7C
    KEY_F14, // VK_F14            0x7D
    KEY_F15, // VK_F15            0x7E
    KEY_F16, // VK_F16            0x7F
    KEY_F17, // VK_F17            0x80
    KEY_F18, // VK_F18            0x81
    KEY_F19, // VK_F19            0x82
    KEY_F20, // VK_F20            0x83
    KEY_F21, // VK_F21            0x84
    KEY_F22, // VK_F22            0x85
    KEY_F23, // VK_F23            0x86
    KEY_F24, // VK_F24            0x87

    KEY_NOPE, // VK_NAVIGATION_VIEW     0x88 // reserved
    KEY_NOPE, // VK_NAVIGATION_MENU     0x89 // reserved
    KEY_NOPE, // VK_NAVIGATION_UP       0x8A // reserved
    KEY_NOPE, // VK_NAVIGATION_DOWN     0x8B // reserved
    KEY_NOPE, // VK_NAVIGATION_LEFT     0x8C // reserved
    KEY_NOPE, // VK_NAVIGATION_RIGHT    0x8D // reserved
    KEY_NOPE, // VK_NAVIGATION_ACCEPT   0x8E // reserved
    KEY_NOPE, // VK_NAVIGATION_CANCEL   0x8F // reserved


    KEY_NUMLOCK, // VK_NUMLOCK        0x90
    KEY_SCROLL, // VK_SCROLL         0x91

    KEY_NOPE, // VK_OEM_NEC_EQUAL  0x92   // '=' key on numpad
              // VK_OEM_FJ_JISHO   0x92   // 'Dictionary' key
    KEY_NOPE, // VK_OEM_FJ_MASSHOU 0x93   // 'Unregister word' key
    KEY_NOPE, // VK_OEM_FJ_TOUROKU 0x94   // 'Register word' key
    KEY_NOPE, // VK_OEM_FJ_LOYA    0x95   // 'Left OYAYUBI' key
    KEY_NOPE, // VK_OEM_FJ_ROYA    0x96   // 'Right OYAYUBI' key

    KEY_NOPE, // 0x97 - 0x9F : unassigned
    KEY_NOPE, // 0x97 - 0x9F : unassigned
    KEY_NOPE, // 0x97 - 0x9F : unassigned
    KEY_NOPE, // 0x97 - 0x9F : unassigned
    KEY_NOPE, // 0x97 - 0x9F : unassigned
    KEY_NOPE, // 0x97 - 0x9F : unassigned
    KEY_NOPE, // 0x97 - 0x9F : unassigned
    KEY_NOPE, // 0x97 - 0x9F : unassigned
    KEY_NOPE, // 0x97 - 0x9F : unassigned


    KEY_LSHIFT, // VK_LSHIFT         0xA0
    KEY_RSHIFT, // VK_RSHIFT         0xA1
    KEY_LCONTROL, // VK_LCONTROL       0xA2
    KEY_RCONTROL, // VK_RCONTROL       0xA3
    KEY_LALT, // VK_LMENU          0xA4
    KEY_RALT, // VK_RMENU          0xA5

};

#endif

WindowPointer Window::createWindow(const WindowInit& init) {
    WindowPointer window(new Window((init.handler ? init.handler : new WindowHandler())));
    return window;
}

Window::Window(WindowHandler* handler) :
    _handler(handler),
#ifdef WIN32
    _backend(WIN32WindowBackend::create(this))
#else
    _backend()
#endif
{

}

Window::~Window() {

}


bool Window::messagePump() {
    return _backend->messagePump();
}
uint32_t Window::width() const {
    return _backend->width();
}
uint32_t Window::height() const {
    return _backend->height();
}

void Window::onResize(const ResizeEvent& e) {
    _handler->onResize(e);
}
void Window::onPaint(const PaintEvent& e) {
    _handler->onPaint(e);
}
void Window::onMouse(const MouseEvent& e) {
    _handler->onMouse(e);
}
void Window::onKeyboard(const KeyboardEvent& e) {
    _handler->onKeyboard(e);
}