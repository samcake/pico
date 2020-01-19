// Shader.h 
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
#pragma once

#include "../Forward.h"

#include <string>
#include "gpu.h"

namespace poco {

    struct ShaderInit {
        ShaderType type;

        std::string entryPoint;

        std::string url;
    };

    struct ProgramInit {
        ShaderPointer vertexShader;
        ShaderPointer pixelShader;
    };

    class Shader {
    protected:
        // Shader is created from the device
        friend class Device;
        Shader();
    public:

        virtual ~Shader();


        ShaderPointer getVertexShader() const { return _programDesc.vertexShader; }
        ShaderPointer getPixelShader() const { return _programDesc.pixelShader; }

    protected:
        ShaderInit _shaderDesc;
        ProgramInit _programDesc;
    };
}