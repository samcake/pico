// Pipeline.h 
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
#include <core/FileWatcher.h>

#include "gpu.h"
#include "StreamLayout.h"

namespace graphics {

    struct VISUALIZATION_API PipelineStateInit {
        ShaderPointer program;
        StreamLayout streamLayout;
        PrimitiveTopology primitiveTopology{ PrimitiveTopology::POINT };
        DescriptorSetLayoutPointer descriptorSetLayout;
        bool depth { false };
        bool blend { false };

        std::string watch_name;
    };

    using PipelineRealizer = std::function<bool (PipelineState*)>;

    class VISUALIZATION_API PipelineState {
    protected:
        // PipelineState is created from the device
        friend class Device;
        PipelineState();

        PipelineStateInit _init;

        PipelineRealizer _pipelineRealizer;
    public:
        virtual ~PipelineState();

        ShaderPointer _program;

        DescriptorSetLayoutPointer getDescriptorSetLayout() const;


        bool realize();

        static void registerToWatcher(const PipelineStatePointer& pipeline, PipelineRealizer pipelineRealizer);

    };

    struct VISUALIZATION_API SamplerInit {

    };

    class VISUALIZATION_API Sampler {
    protected:
        // Sampler is created from the device
        friend class Device;
        Sampler();

    public:
        virtual ~Sampler();

        SamplerInit _init;
    };

    class PipelineWatcher {
    public:

        PipelineWatcher();
        ~PipelineWatcher();

        std::unique_ptr< core::FileWatcher> _fileWatcher;
        std::unordered_map<std::string, ShaderWeakPtr> _tokenToShaders;
        std::unordered_multimap<std::string, std::string> _shaderToPrograms;
        std::unordered_multimap<std::string, PipelineStateWeakPtr> _programToPipelines;

        void add(const ShaderPointer& shader);
        void add(const PipelineStatePointer& pipeline);

        void notifyShaderRecompiled(const std::string& key);
        void notifyProgramRelinked(const std::string& key);

        static PipelineWatcher* get();
        static std::unique_ptr<PipelineWatcher> _instance;
    };
}