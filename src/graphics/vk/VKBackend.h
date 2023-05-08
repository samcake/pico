// VKBackend.h 
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

#include "gpu/Device.h"
#include "gpu/Swapchain.h"
#include "gpu/Framebuffer.h"
#include "gpu/Batch.h"
#include "gpu/Resource.h"
#include "gpu/Pipeline.h"
#include "gpu/Shader.h"
#include "gpu/Descriptor.h"
#include "gpu/Query.h"

#include <core/stl/IndexTable.h>


#define PICO_VULKAN
#ifdef PICO_VULKAN


/* Set platform defines at build time for volk to pick up. */
/*#if defined(_WIN32)
#   define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux__) || defined(__unix__)
#   define VK_USE_PLATFORM_XLIB_KHR
#elif defined(__APPLE__)
#   define VK_USE_PLATFORM_MACOS_MVK
#else
#   error "Platform not supported by this example."
#endif

#define VOLK_IMPLEMENTATION
*/
#include "volk.h"

#define VK_CHECK(result) graphics::VKBackend::VkCheck(__FILE__, __LINE__, __FUNCTION__, result);

namespace graphics {
   
    class VKDescriptorHeapBackend : public DescriptorHeap {
    public:
        VKDescriptorHeapBackend();
        virtual ~VKDescriptorHeapBackend();

        int32_t allocateDescriptors(int32_t numDescriptors) override;
        int32_t allocateSamplers(int32_t numSamplers) override;

        //core::IndexTable _descriptor_table;
        //core::IndexTable _sampler_table;

        //UINT _descriptor_increment_size;
        //UINT _sampler_increment_size;

        //ComPtr < IVKDescriptorHeap> _cbvsrvuav_heap;
        //ComPtr < IVKDescriptorHeap> _sampler_heap;

        //VK_GPU_DESCRIPTOR_HANDLE gpuHandle(uint32_t offset) const;
        //VK_CPU_DESCRIPTOR_HANDLE cpuHandle(uint32_t offset) const;

        //VK_GPU_DESCRIPTOR_HANDLE gpuSamplerHandle(uint32_t offset) const;
        //VK_CPU_DESCRIPTOR_HANDLE cpuSamplerHandle(uint32_t offset) const;
    };

    class VKBackend : public DeviceBackend {
    public:
        VKBackend();
        virtual ~VKBackend();

        static const uint8_t CHAIN_NUM_FRAMES = 2;

        void* nativeDevice() override { return _device; }

        // Vulkan Objects
        VkInstance _instance = 0;
        VkPhysicalDevice _physicalDevice = 0;
        uint32_t _familyIndex = 0;
        VkDevice _device = 0;
        VkQueue _queue = 0;
        VkCommandPool _commandPool = 0;

        double _commandQueueTimestampFrequency = 1.0;


        // Synchronization objects
        VkSemaphore _acquireSemaphores[VKBackend::CHAIN_NUM_FRAMES] = {};
        VkSemaphore _releaseSemaphores[VKBackend::CHAIN_NUM_FRAMES] = {};
        VkFence     _inFlightFences[VKBackend::CHAIN_NUM_FRAMES] = {};

      //  ComPtr<IVKFence> _fence;
        //uint64_t _fenceValue = 0;
        //uint64_t _frameFenceValues[VKBackend::CHAIN_NUM_FRAMES] = {};
        //HANDLE _fenceEvent;

        SwapchainPointer createSwapchain(const SwapchainInit& init) override;
        void resizeSwapchain(const SwapchainPointer& swapchain, uint32_t width, uint32_t height) override;

        FramebufferPointer createFramebuffer(const FramebufferInit& init) override;

        BatchPointer createBatch(const BatchInit& init) override;
        BatchTimerPointer createBatchTimer(const BatchTimerInit& init) override;
 
        BufferPointer _createBuffer(const BufferInit& init, const std::string& name) override;

        TexturePointer createTexture(const TextureInit& init) override;

        GeometryPointer createGeometry(const GeometryInit& init) override;

        ShaderPointer createShader(const ShaderInit& init) override;
        ShaderPointer createProgram(const ProgramInit& init) override;

        SamplerPointer createSampler(const SamplerInit& init) override;

        PipelineStatePointer createGraphicsPipelineState(const GraphicsPipelineStateInit& init) override;
        PipelineStatePointer createComputePipelineState(const ComputePipelineStateInit& init) override;
        PipelineStatePointer createRaytracingPipelineState(const RaytracingPipelineStateInit& init) override;
   
        RootDescriptorLayoutPointer createRootDescriptorLayout(const RootDescriptorLayoutInit& init) override;
        DescriptorSetPointer createDescriptorSet(const DescriptorSetInit& init) override;

        ShaderEntry          getShaderEntry(const PipelineStatePointer& pipeline, const std::string& entry) override;
        ShaderTablePointer   createShaderTable(const ShaderTableInit& init) override;

        void updateDescriptorSet(DescriptorSetPointer& descriptorSet, DescriptorObjects& objects) override;
        DescriptorHeapPointer createDescriptorHeap(const DescriptorHeapInit& init) override;

        void acquireSwapchain(const SwapchainPointer& swapchain) override;
        void executeBatch(const BatchPointer& batch) override;
        void presentSwapchain(const SwapchainPointer& swapchain) override;

        void flush() override;

        // Default empty pipeline layout
        RootDescriptorLayoutPointer _emptyRootDescriptorLayout;

        // Global Descriptor Heap
        DescriptorHeapPointer getDescriptorHeap() override;
        DescriptorHeapPointer _descriptorHeap;

        // Separate shader and pipeline state compilation as functions in order
        // to be able to live edit the shaders
        bool compileShader(Shader* shader, const std::string& src);
        bool compileShaderLib(Shader* shader, const std::string& src);
        bool realizePipelineState(PipelineState* pipeline);

        // When we modify d3d12 objects already in use, we need to garbage collect
        // them to be destroyed at a later time
      //  void garbageCollect(const ComPtr<IVKDeviceChild>& child);
        //void flushGarbage();
      //  std::list< ComPtr<IVKDeviceChild> > _garbageObjects;

        // Enum translation tables
        static const VkFormat Format[uint32_t(PixelFormat::COUNT)];
        static const VkFormat FormatBGRA[uint32_t(PixelFormat::COUNT)];
  
        static void VkCheck(const char* file, int line, const char* functionName, VkResult result);
    };

    class VKSwapchainBackend : public Swapchain {
    public:
        friend class VKBackend;
        VKSwapchainBackend();
        virtual ~VKSwapchainBackend();

        VkSurfaceKHR _surface = 0;
        VkRenderPass _renderPass = 0;
        VkSwapchainKHR _swapchain = 0;
        VkFormat _swapchainFormat;

        std::vector<VkImage> _images;
        std::vector<VkImageView> _imageViews;
        std::vector<VkFramebuffer> _framebuffers;

        uint32_t _imageCount = 0;

     //   ComPtr<IVKResource> _backBuffers[VKBackend::CHAIN_NUM_FRAMES];
       // ComPtr<IVKDescriptorHeap> _rtvDescriptorHeap;
      //  UINT _rtvDescriptorSize;

     //   ComPtr<IVKResource> _depthBuffer;
       // ComPtr<IVKDescriptorHeap> _dsvDescriptorHeap;
       // UINT _dsvDescriptorSize { 0 };
    };

    class VKFramebufferBackend : public Framebuffer {
    public:
        friend class VKBackend;
        VKFramebufferBackend();
        virtual ~VKFramebufferBackend();

        VkFramebuffer _framebuffer = 0;
        VkRenderPass _renderPass = 0;

        // Gather color buffer rtvs in one heap
        UINT _numRenderTargets = 0;
      //  ComPtr<IVKDescriptorHeap> _rtvDescriptorHeap;
        UINT _rtvDescriptorSize = 0;
      //  VK_CPU_DESCRIPTOR_HANDLE _rtvs;

      //  ComPtr<IVKDescriptorHeap> _dsvDescriptorHeap;
        UINT _dsvDescriptorSize = 0;
      //  VK_CPU_DESCRIPTOR_HANDLE _dsv;
    };

    class VKPipelineStateBackend;
    class VKDescriptorSetBackend;
    class VKBatchBackend : public Batch {
    public:
        friend class VKBackend;
        VKBatchBackend();
        virtual ~VKBatchBackend();

        void begin(uint8_t currentIndex, const BatchTimerPointer& timer) override;
        void end() override;

        void beginPass(const SwapchainPointer& swapchain, uint8_t currentIndex) override;
        void endPass() override;

        void clear(const SwapchainPointer& swapchain, uint8_t index, const core::vec4& color, float depth) override;
        void clear(const FramebufferPointer& framebuffer, const core::vec4& color, float depth) override;

        void resourceBarrierTransition(
            ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
            const SwapchainPointer& swapchain, uint8_t currentIndex, uint32_t subresource) override;
        void resourceBarrierTransition(
            ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
            const BufferPointer& buffer) override;
        void resourceBarrierTransition(
            ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
            const TexturePointer& texture, uint32_t subresource) override;

        void resourceBarrierRW(
            ResourceBarrierFlag flag, const BufferPointer& buffer) override;
        void resourceBarrierRW(
            ResourceBarrierFlag flag, const TexturePointer& texture, uint32_t subresource) override;

        void _setViewport(const core::vec4& viewport) override;
        void _setScissor(const core::vec4& scissor) override;

        void bindDescriptorHeap(const DescriptorHeapPointer& descriptorHeap);

        void bindFramebuffer(const FramebufferPointer& framebuffer) override;

        void bindRootDescriptorLayout(PipelineType type, const RootDescriptorLayoutPointer& rootDescriptorLayout) override;
        void bindPipeline(const PipelineStatePointer& pipeline) override;

        void bindDescriptorSet(PipelineType type, const DescriptorSetPointer& descriptorSet) override;
        void bindPushUniform(PipelineType type, uint32_t slot, uint32_t size, const uint8_t* data) override;

        void bindIndexBuffer(const BufferPointer& buffer) override;
        void bindVertexBuffers(uint32_t num, const BufferPointer* buffers) override;

        void draw(uint32_t numPrimitives, uint32_t startIndex) override;
        void drawIndexed(uint32_t numPrimitives, uint32_t startIndex) override;

        void uploadTexture(const TexturePointer& dest, const UploadSubresourceLayoutArray& subresourceLayout, const BufferPointer& src) override;

        void copyBufferRegion(const BufferPointer& dest, uint32_t destOffset, const BufferPointer& src, uint32_t srcOffset, uint32_t size) override;
        void uploadBuffer(const BufferPointer& dest) override;

        void dispatch(uint32_t numThreadsX, uint32_t numThreadsY, uint32_t numThreadsZ) override;

        void dispatchRays(const DispatchRaysArgs& args) override;

    //    ComPtr<IVKGraphicsCommandList4> _commandList;
    //    ComPtr<IVKCommandAllocator> _commandAllocators[VKBackend::CHAIN_NUM_FRAMES];

        VkCommandBuffer _commandBuffers[VKBackend::CHAIN_NUM_FRAMES];

        DescriptorHeapPointer _descriptorHeap;

        UINT _currentBackBufferIndex{ 0 };

        UINT _currentGraphicsRootLayout_setRootIndex = 0;
        UINT _currentGraphicsRootLayout_samplerRootIndex = 0;
        UINT _currentComputeRootLayout_setRootIndex = 0;
        UINT _currentComputeRootLayout_samplerRootIndex = 0;

        VKPipelineStateBackend* _currentGraphicsPipeline = nullptr;

        VKDescriptorSetBackend* _currentGraphicsDescriptorSets[4] = { nullptr, nullptr, nullptr, nullptr };

        BatchTimerPointer _timer;

    //    static const VK_RESOURCE_STATES ResourceStates[uint32_t(ResourceState::COUNT)];
   //     static const VK_RESOURCE_BARRIER_FLAGS  ResourceBarrieFlags[uint32_t(ResourceBarrierFlag::COUNT)];

    //    static const VK_PRIMITIVE_TOPOLOGY_TYPE  PrimitiveTopologyTypes[uint32_t(PrimitiveTopology::COUNT)];
    //    static const VK_PRIMITIVE_TOPOLOGY  PrimitiveTopologies[uint32_t(PrimitiveTopology::COUNT)];

    };

    class VKBufferBackend : public Buffer {
    public:
        friend class VKBackend;
        VKBufferBackend();
        virtual ~VKBufferBackend();

    //    ComPtr<IVKResource> _resource;
    //    ComPtr<IVKResource> _cpuDataResource;


     //   VK_CONSTANT_BUFFER_VIEW_DESC _uniformBufferView;
      //  VK_VERTEX_BUFFER_VIEW _vertexBufferView;
     //   VK_INDEX_BUFFER_VIEW _indexBufferView;
     //   VK_SHADER_RESOURCE_VIEW_DESC _resourceBufferView;
     //   VK_UNORDERED_ACCESS_VIEW_DESC _rwResourceBufferView;
    };

    class VKTextureBackend : public Texture {
    public:
        friend class VKBackend;
        VKTextureBackend();
        virtual ~VKTextureBackend();

    //    ComPtr<IVKResource> _resource;

    //    VK_SHADER_RESOURCE_VIEW_DESC   _shaderResourceViewDesc;
    //    VK_UNORDERED_ACCESS_VIEW_DESC   _unorderedAccessViewDesc;
    };

    class VKGeometryBackend : public Geometry {
    public:
        friend class VKBackend;
        VKGeometryBackend();
        virtual ~VKGeometryBackend();

        // Acceleration structure
    //    ComPtr<IVKResource> _bottomLevelAccelerationStructure;
    //    ComPtr<IVKResource> _topLevelAccelerationStructure;
    };

    class VKShaderBackend : public Shader {
    public:
        friend class VKBackend;
        VKShaderBackend();
        virtual ~VKShaderBackend();

       // ComPtr<ID3DBlob> _shaderBlob;
     //   ComPtr<IDxcBlob> _shaderBlob;
    //    ComPtr<IDxcBlob> _shaderLibBlob;

        static const std::string ShaderTypes[uint32_t(ShaderType::COUNT)];
    };

    class VKSamplerBackend : public Sampler {
    public:
        friend class VKBackend;
        VKSamplerBackend();
        virtual ~VKSamplerBackend();

     //   VK_SAMPLER_DESC _samplerDesc;
    };

    class VKPipelineStateBackend : public PipelineState {
    public:
        friend class VKBackend;
        VKPipelineStateBackend();
        virtual ~VKPipelineStateBackend();

    //    ComPtr<IVKRootSignature> _rootSignature;
//        ComPtr<IVKPipelineState> _pipelineState;

  //      ComPtr<IVKStateObject> _stateObject; // raytracing
    //    ComPtr<IVKRootSignature> _localRootSignature; // raytracing

      //  VK_PRIMITIVE_TOPOLOGY _primitive_topology;

    //    static void fill_rasterizer_desc(const RasterizerState& src, VK_RASTERIZER_DESC& dst);
     //   static void fill_depth_stencil_desc(const DepthStencilState& src, VK_DEPTH_STENCIL_DESC& dst);
     //   static void fill_blend_desc(const BlendState& src, VK_BLEND_DESC& dst);
    };


    class VKRootDescriptorLayoutBackend : public RootDescriptorLayout {
    public:
        friend class VKBackend;
        VKRootDescriptorLayoutBackend();
        virtual ~VKRootDescriptorLayoutBackend();

     //   ComPtr<IVKRootSignature> _rootSignature;
      
     //   std::vector< uint32_t > _dxPushParamIndices;
        
        int32_t _cbvsrvuav_rootIndex = -1;
        std::vector< uint32_t > _dxSetParamIndices;

        int32_t _sampler_rootIndex = -1;

        uint32_t _push_count = 0;
        uint32_t _cbvsrvuav_count = 0;
        std::vector< uint32_t > _cbvsrvuav_counts;
        uint32_t _sampler_count = 0;
    };

    class VKDescriptorSetBackend : public DescriptorSet {
    public:
        friend class VKBackend;
        VKDescriptorSetBackend();
        virtual ~VKDescriptorSetBackend();

        int32_t _cbvsrvuav_rootIndex = -1;
        int32_t _sampler_rootIndex = -1;

        uint32_t _cbvsrvuav_count = 0;
        uint32_t _sampler_count = 0;

//        VK_GPU_DESCRIPTOR_HANDLE _cbvsrvuav_GPUHandle;
  //      VK_GPU_DESCRIPTOR_HANDLE _sampler_GPUHandle;
    };

    class VKShaderTableBackend : public ShaderTable {
    public:
        friend class VKBackend;
        VKShaderTableBackend();
        virtual ~VKShaderTableBackend();

   //     ComPtr<IVKResource> _shaderTable;
    };

    class VKBatchTimerBackend : public BatchTimer {
    public:
        friend class VKBackend;
        VKBatchTimerBackend();
        virtual ~VKBatchTimerBackend();

     //   void begin(IVKGraphicsCommandList* _commandList, INT index);
    //    void end(IVKGraphicsCommandList* _commandList, INT index);

     //   ComPtr<IVKQueryHeap> _queryHeap;
     //   ComPtr<IVKResource> _queryResult;
    //    ComPtr<IVKResource> _queryResultMapped;
        void* _queryResultCPUMappedAddress = nullptr;

        double _commandQueueTimestampFrequency = 1.0;
    };

    class VK
    {
    public:
        static VkRenderPass createRenderPass(VkDevice device, VkFormat format);
        static VkFramebuffer createFramebuffer(VkDevice device, VkRenderPass renderPass, VkImageView imageView, uint32_t width, uint32_t height);
        static VkImageView createImageView(VkDevice device, VkImage image, VkFormat format);

    };
}

#endif PICO_VULKAN
