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
#include "Rasterizer.h"
#include "DepthStencil.h"
#include "Blend.h"
#include "Sampler.h"

namespace graphics {

    struct VISUALIZATION_API GraphicsPipelineStateInit {
        ShaderPointer program;
        StreamLayout streamLayout;
        PrimitiveTopology primitiveTopology{ PrimitiveTopology::POINT };
        DescriptorSetLayoutPointer descriptorSetLayout;
        RasterizerState rasterizer;
        DepthStencilState depthStencil;
        BlendState blend;

        std::string watch_name;
    };

    struct VISUALIZATION_API ComputePipelineStateInit {
        ShaderPointer program;
        DescriptorSetLayoutPointer descriptorSetLayout;
        
        std::string watch_name;
    };

    using PipelineRealizer = std::function<bool (PipelineState*)>;

    class VISUALIZATION_API PipelineState {
    protected:
        // PipelineState is created from the device
        friend class Device;
        friend class PipelineWatcher;
        PipelineState();

        PipelineType _type;



        ShaderPointer _program;
        DescriptorSetLayoutPointer _descriptorSetLayout;

        PipelineRealizer _pipelineRealizer;

//        union {
            GraphicsPipelineStateInit _graphics;
            ComputePipelineStateInit _compute;
  //      };       

    public:
        virtual ~PipelineState();

        PipelineType getType() const;
        DescriptorSetLayoutPointer getDescriptorSetLayout() const;


        bool realize();

        static void registerToWatcher(const PipelineStatePointer& pipeline, PipelineRealizer pipelineRealizer);
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