// Pipeline.cpp
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
#include "Pipeline.h"
#include "Shader.h"

#include <fstream>
#include <sstream>

using namespace graphics;

PipelineState::PipelineState() {

}

PipelineState::~PipelineState() {

}

Sampler::Sampler() {

}

Sampler::~Sampler() {

}

PipelineType PipelineState::getType() const {
    return _type;
}

DescriptorSetLayoutPointer PipelineState::getDescriptorSetLayout() const {
    return _descriptorSetLayout;
}

bool PipelineState::realize() {
    if (_pipelineRealizer) {
        return _pipelineRealizer(this);
    }
    return false;
}

void PipelineState::registerToWatcher(const PipelineStatePointer& pipeline, PipelineRealizer pipelineRealizer) {
    pipeline->_pipelineRealizer = pipelineRealizer;
    PipelineWatcher::get()->add(pipeline);
}


std::unique_ptr<PipelineWatcher> PipelineWatcher::_instance;

PipelineWatcher* PipelineWatcher::get() {
    if (!_instance) {
        _instance.reset(new PipelineWatcher());
    }

    return _instance.get();
}

PipelineWatcher::PipelineWatcher() {
    // Create a FileWatcher instance that will check the current folder for changes every 2 seconds
    _fileWatcher.reset(new core::FileWatcher(std::chrono::milliseconds(2000)));
}

void PipelineWatcher::add(const ShaderPointer& shader) {

    if (!shader->isProgram()) {
        std::string shader_token = shader->getShaderDesc().watcher_file;
        if (!shader_token.empty()) {
            auto shaderWeakPtr = ShaderWeakPtr(shader);
            _tokenToShaders[shader_token] = shaderWeakPtr;

            _fileWatcher->watchFile(shader_token, [&, shaderWeakPtr](const std::string& k, core::FileStatus s) {
                if (s == core::FileStatus::modified) {
                    auto shader = shaderWeakPtr.lock();
                    if (shader) {
                        // Let's load the file source here
                        std::string source;
                        if (!k.empty()) {
                            std::ifstream file(k, std::ifstream::in);
                            source = (std::stringstream() << file.rdbuf()).str();
                        }

                        if (shader->recompile(source)) {
                            picoLog() << "SUCCESS recompiling shader <" + k + ">\n";

                            // shader successfully recompiled, let's trigger the programs about it
                            notifyShaderRecompiled(k);
                        } else {
                            picoLog() << "FAIL recompiling shader <" + k + ">\n";
                        }
                    }
                }
                });

        }
    }
    else {
        auto programWeakPtr = ShaderWeakPtr(shader);
        std::string program_token = std::to_string((uint64_t) shader.get());
        _tokenToShaders[program_token] = programWeakPtr;

        // this is a program, we just add the program as dependent on the subshader
        auto sub_shader = shader->getProgramDesc().vertexShader;
        if (sub_shader) {
            auto sub_shader_filename = sub_shader->getShaderDesc().watcher_file;
            if (!sub_shader_filename.empty()) {
                _shaderToPrograms.emplace(sub_shader_filename, program_token);
            }
        }
        sub_shader = shader->getProgramDesc().pixelShader;
        if (sub_shader) {
            auto sub_shader_filename = sub_shader->getShaderDesc().watcher_file;
            if (!sub_shader_filename.empty()) {
                _shaderToPrograms.emplace(sub_shader_filename, program_token);
            }
        }
    }
}


void PipelineWatcher::add(const PipelineStatePointer& pipeline) {
    // find the program used by the pipeline and associate an extra dependency
    std::string program_token = std::to_string((uint64_t)pipeline->_program.get());

    auto pipelineWeakPtr = PipelineStateWeakPtr(pipeline);

    _programToPipelines.emplace(program_token, pipelineWeakPtr);

}

void PipelineWatcher::notifyShaderRecompiled(const std::string& key) {
    // a shader just got recompiled, let's see who is affected
    auto found_bucket = _shaderToPrograms.equal_range(key);
    std::for_each(found_bucket.first, found_bucket.second, [&](std::unordered_multimap<std::string, std::string>::value_type& x) {
        // from the associated program_token, find the program (as a shader)
        auto program_wptr = _tokenToShaders[x.second];
        auto program = program_wptr.lock();
        if (program) {
            if (program->relink()) {
                picoLog() << "SUCCESS relinking program <" + x.second + ">\n";
                notifyProgramRelinked(x.second);
            } else {
                picoLog() << "FAIL relinking program <" + x.second + ">\n";
            }
        }
    });
}


void PipelineWatcher::notifyProgramRelinked(const std::string& key) {
    // a program just got relinked, let's see who is affected
    auto found_bucket = _programToPipelines.equal_range(key);
    std::for_each(found_bucket.first, found_bucket.second, [](std::unordered_multimap<std::string, PipelineStateWeakPtr>::value_type& x) {
        auto pipeline = x.second.lock();
        if (pipeline) {
            if (pipeline->realize()) {
                picoLog() << "SUCCESS realizing pipeline <" + x.first + ">\n";
            } else {
                picoLog() << "FAIL realizing pipeline <" + x.first + ">\n";
            }
        }
    });
}

PipelineWatcher::~PipelineWatcher() {
    _fileWatcher.reset();
}

