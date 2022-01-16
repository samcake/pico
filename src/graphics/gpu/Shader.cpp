// Shader.cpp
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
#include "Shader.h"
#include <fstream>
#include <sstream>

#include "Pipeline.h"

using namespace graphics;

std::string shaderTypeNames[(uint8_t)ShaderType::COUNT] =
{
    "PROGRAM",
    "VERTEX",
    "PIXEL",

    "COMPUTE",

    "RAYTRACING",
};

Shader::Shader():
    _shaderDesc{},
    _programDesc{}
{

}

Shader::~Shader() {

}

bool Shader::hasWatcher() const {
    if (isProgram()) {
        // check for any false case
        for (auto& s : _programDesc.shaderLib) {
            if (!(s.second && s.second->hasWatcher())) {
                return false;
            }
        }
        return true;
    }
    else {
        return !_shaderDesc.watcher_file.empty();
    }
}

void Shader::registerToWatcher(const ShaderPointer& shader, ShaderCompiler shaderCompile, ProgramLinker programLinker) {
    shader->_shaderCompiler = shaderCompile;
    shader->_programLinker = programLinker;
    PipelineWatcher::get()->add(shader);
}

bool Shader::recompile(const std::string& src) {
    if (_shaderCompiler) {
        return _shaderCompiler(this, src);
    }
    return false;
}

bool Shader::relink() {
    if (_programLinker) {
        return _programLinker(this);
    }
    return false;
}
