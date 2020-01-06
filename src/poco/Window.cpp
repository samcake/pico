// Window.cpp
//
// Sam Gateau - 2020/1/1
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

using namespace poco;

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class WIN32WindowBackend : public WindowBackend {
public:
    HWND _sysWindow{ nullptr };
    uint32_t _width{ 0 };
    uint32_t _height{ 0 };

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
        case WM_PAINT:
            _ownerWindow->onPaint();
           return 0;

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

    void* nativeWindow() override {
        return _sysWindow;
    }

};

WIN32WindowBackend::SysWindowMap WIN32WindowBackend::_sysWindowMap;
std::string WIN32WindowBackend::WINDOW_CLASS;


#endif

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


void Window::onPaint() {
    _handler->onPaint();
}