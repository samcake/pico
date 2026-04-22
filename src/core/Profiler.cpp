// Profiler.cpp
//
// Sam Gateau - April 2026
//
// MIT License
//
// Copyright (c) 2026 Sam Gateau
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
#include "Profiler.h"

// -------------------------------------------------------------------------
// PIX GPU event markers — WinPixEventRuntime (D3D12/Windows).
// pix3.h must see __d3d12_h__ to enable the D3D12 overloads.
// -------------------------------------------------------------------------

#ifdef PICO_PROFILER_PIX
#define USE_PIX
#include <d3d12.h>
#include <WinPixEventRuntime/pix3.h>
#endif

// -------------------------------------------------------------------------
// Superluminal backend — zero link-time dependency.
// LoadLibraryW to the known install path; graceful no-op if absent.
// -------------------------------------------------------------------------

#ifdef PICO_PROFILER_SUPERLUMINAL
#include <windows.h>
#include <Superluminal/PerformanceAPI_capi.h>

namespace {
    PerformanceAPI_Functions s_slApi = {};

    struct SuperluminalLoader {
        HMODULE _mod { nullptr };

        SuperluminalLoader() {
            _mod = LoadLibraryW(L"C:/Program Files/Superluminal/Performance/API/dll/x64/PerformanceAPI.dll");
            if (!_mod) return;
            auto getAPI = (PerformanceAPI_GetAPI_Func)(void*)GetProcAddress(_mod, "PerformanceAPI_GetAPI");
            if (getAPI) getAPI(PERFORMANCEAPI_VERSION, &s_slApi);
        }

        ~SuperluminalLoader() {
            if (_mod) FreeLibrary(_mod);
        }
    } s_slLoader;
}
#endif // PICO_PROFILER_SUPERLUMINAL

// -------------------------------------------------------------------------
// ETW backend via TraceLogging — Windows SDK only, no external dep.
// Recognized by WPA, PerfView, and any ETW consumer.
// -------------------------------------------------------------------------

#ifdef PICO_PROFILER_ETW
#ifndef _WINDOWS_
#include <windows.h>
#endif
#include <TraceLoggingProvider.h>

#ifndef WINEVENT_OPCODE_START
#  define WINEVENT_OPCODE_START 1
#  define WINEVENT_OPCODE_STOP  2
#endif

// Provider GUID — unique to Pico; generate once with uuidgen and keep stable.
// {5F1A2B3C-4D5E-6F7A-8B9C-0D1E2F3A4B5C}
TRACELOGGING_DEFINE_PROVIDER(
    g_picoProvider,
    "Pico",
    (0x5f1a2b3c, 0x4d5e, 0x6f7a, 0x8b, 0x9c, 0x0d, 0x1e, 0x2f, 0x3a, 0x4b, 0x5c)
);

namespace {
    struct PicoProviderLifetime {
        PicoProviderLifetime()  { TraceLoggingRegister(g_picoProvider); }
        ~PicoProviderLifetime() { TraceLoggingUnregister(g_picoProvider); }
    } s_etwLifetime;
}
#endif // PICO_PROFILER_ETW

namespace pico {

    // -------------------------------------------------------------------------
    // CpuScope
    // -------------------------------------------------------------------------

    CpuScope::CpuScope(const char* name) {
#ifdef PICO_PROFILER_SUPERLUMINAL
        if (s_slApi.BeginEvent) s_slApi.BeginEvent(name, nullptr, PERFORMANCEAPI_DEFAULT_COLOR);
#endif
#ifdef PICO_PROFILER_ETW
        TraceLoggingWrite(g_picoProvider, "CpuScope",
            TraceLoggingOpcode(WINEVENT_OPCODE_START),
            TraceLoggingString(name, "Name"));
#endif
    }

    CpuScope::~CpuScope() {
#ifdef PICO_PROFILER_SUPERLUMINAL
        if (s_slApi.EndEvent) s_slApi.EndEvent();
#endif
#ifdef PICO_PROFILER_ETW
        TraceLoggingWrite(g_picoProvider, "CpuScope",
            TraceLoggingOpcode(WINEVENT_OPCODE_STOP));
#endif
    }

    // -------------------------------------------------------------------------
    // GpuScope — PIX GPU event markers on a D3D12 command list.
    // -------------------------------------------------------------------------

    GpuScope::GpuScope(void* nativeCmdList, const char* name)
        : _cmdList(nativeCmdList)
    {
#ifdef PICO_PROFILER_PIX
        if (_cmdList)
            PIXBeginEvent(static_cast<ID3D12GraphicsCommandList*>(_cmdList), 0, name);
#endif
    }

    GpuScope::~GpuScope() {
#ifdef PICO_PROFILER_PIX
        if (_cmdList)
            PIXEndEvent(static_cast<ID3D12GraphicsCommandList*>(_cmdList));
#endif
    }

} // namespace pico
