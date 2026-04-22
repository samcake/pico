// Profiler.h
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
#pragma once

#include "dllmain.h"

namespace pico {

    // RAII CPU scope — visible in any profiler attached to this thread.
    struct CORE_API CpuScope {
        explicit CpuScope(const char* name);
        ~CpuScope();
    };

    // RAII GPU scope — emits markers on a native command list (ID3D12GraphicsCommandList*).
    // Pass the native handle; the graphics layer provides typed helpers.
    struct CORE_API GpuScope {
        GpuScope(void* nativeCmdList, const char* name);
        ~GpuScope();
    private:
        void* _cmdList { nullptr };
    };

} // namespace pico

// -------------------------------------------------------------------------
// Macros — compile to nothing when PICO_PROFILING is not defined.
// Use a unique variable name via __LINE__ so nested scopes don't collide.
// -------------------------------------------------------------------------

#define PICO_SCOPE_NAME_(prefix, line) prefix##line
#define PICO_SCOPE_NAME(prefix, line)  PICO_SCOPE_NAME_(prefix, line)

#ifdef PICO_PROFILING
#  define PICO_CPU_SCOPE(name) \
        pico::CpuScope PICO_SCOPE_NAME(_pico_cpu_, __LINE__) { name };
#  define PICO_GPU_SCOPE(nativeCmdList, name) \
        pico::GpuScope PICO_SCOPE_NAME(_pico_gpu_, __LINE__) { nativeCmdList, name };
#else
#  define PICO_CPU_SCOPE(name)
#  define PICO_GPU_SCOPE(nativeCmdList, name)
#endif
