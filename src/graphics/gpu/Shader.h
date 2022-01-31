// Shader.h 
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

#include <string>
#include <functional>

#include "gpu.h"


namespace graphics {

    typedef const std::string& (*ShaderSourceGetter)();
    using ShaderIncludeLib = std::unordered_map<std::string, ShaderSourceGetter>;
    using ShaderCompiler = std::function<bool(Shader*, const std::string&)>;
    using ProgramLinker = std::function<bool (Shader*)>;

    struct VISUALIZATION_API ShaderInit {
        ShaderType type { ShaderType::PROGRAM };
        std::string entryPoint;

        std::string url;
        ShaderSourceGetter sourceGetter = nullptr;
       
        std::string watcher_file;

        ShaderIncludeLib includes;

        ShaderInit() {}

        ShaderInit(ShaderType t, const std::string& e, const std::string& u, const ShaderIncludeLib& i = ShaderIncludeLib()) :
            type(t), entryPoint(e),
            url(u),
            //sourceGetter(g),
            watcher_file(u),
            includes(i)
        {}

        ShaderInit(ShaderType t, const std::string& e, ShaderSourceGetter g, const std::string& w = std::string(), const ShaderIncludeLib& i = ShaderIncludeLib()) :
            type(t), entryPoint(e),
            //url(u),
            sourceGetter(g),
            watcher_file(w),
            includes(i) {
        }

    };

    struct VISUALIZATION_API ProgramInit {
        ShaderPointer vertexShader;
        ShaderPointer pixelShader;
    };

    class VISUALIZATION_API Shader {
    protected:
        // Shader is created from the device
        friend class Device;
        Shader();
    public:

        virtual ~Shader();


        ShaderPointer getVertexShader() const { return _programDesc.vertexShader; }
        ShaderPointer getPixelShader() const { return _programDesc.pixelShader; }

        bool isProgram() const { return _shaderDesc.type == ShaderType::PROGRAM; }

        bool isCompute() const { return _shaderDesc.type == ShaderType::COMPUTE; }
        
        bool hasWatcher() const;

        const ShaderInit& getShaderDesc() const { return _shaderDesc; }
        const ProgramInit& getProgramDesc() const { return _programDesc; }

        // shader needs a recompile with a new source
        virtual bool recompile(const std::string& src);
        
        // program needs a relink
        virtual bool relink();

    protected:
        ShaderInit _shaderDesc;
        ProgramInit _programDesc;

        ShaderCompiler _shaderCompiler;
        ProgramLinker _programLinker;

        static void registerToWatcher(const ShaderPointer& shader, ShaderCompiler shaderCompiler, ProgramLinker programLinker = nullptr);
    };

}