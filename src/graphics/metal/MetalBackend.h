// MetalBackend.h
// Metal GPU backend for pico
//
// Sam Gateau / pico - 2024
// MIT License
#pragma once

#ifdef __APPLE__

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "gpu/Device.h"
#include "gpu/Swapchain.h"
#include "gpu/Framebuffer.h"
#include "gpu/Batch.h"
#include "gpu/Resource.h"
#include "gpu/Pipeline.h"
#include "gpu/Shader.h"
#include "gpu/Descriptor.h"
#include "gpu/Query.h"

#include <dispatch/dispatch.h>
#include <vector>
#include <string>

namespace graphics {

// ---------------------------------------------------------------------------
// Swapchain
// ---------------------------------------------------------------------------
class MetalSwapchainBackend : public Swapchain {
public:
    MetalSwapchainBackend();
    virtual ~MetalSwapchainBackend();

    CAMetalLayer*        _layer      { nil };
    id<MTLTexture>       _depthTexture { nil };
    id<CAMetalDrawable>  _currentDrawable { nil };

    void acquireNextDrawable();
    void releaseDrawable();
};

// ---------------------------------------------------------------------------
// Framebuffer
// ---------------------------------------------------------------------------
class MetalFramebufferBackend : public Framebuffer {
public:
    MetalFramebufferBackend();
    virtual ~MetalFramebufferBackend();

    void initFromDesc(const FramebufferInit& d) { _init = d; }

    std::vector<id<MTLTexture>> _colorTextures;
    id<MTLTexture>              _depthTexture { nil };
};

// ---------------------------------------------------------------------------
// Buffer
// ---------------------------------------------------------------------------
class MetalBufferBackend : public Buffer {
public:
    MetalBufferBackend();
    virtual ~MetalBufferBackend();

    void setBufferSize(uint64_t sz) { _bufferSize = sz; }
    void setName(const std::string& n) { _name = n; }
    void setNeedUpload(bool v) { _needUpload = v; }

    id<MTLBuffer> _buffer { nil };
};

// ---------------------------------------------------------------------------
// Texture
// ---------------------------------------------------------------------------
class MetalTextureBackend : public Texture {
public:
    MetalTextureBackend();
    virtual ~MetalTextureBackend();

    id<MTLTexture> _texture { nil };
};

// ---------------------------------------------------------------------------
// Geometry (stub for raytracing BLAS/TLAS — not needed for pico_01..03)
// ---------------------------------------------------------------------------
class MetalGeometryBackend : public Geometry {
public:
    MetalGeometryBackend();
    virtual ~MetalGeometryBackend();
};

// ---------------------------------------------------------------------------
// Shader
// ---------------------------------------------------------------------------
class MetalShaderBackend : public Shader {
public:
    MetalShaderBackend();
    virtual ~MetalShaderBackend();

    void initShaderDesc(const ShaderInit& d)   { _shaderDesc = d; }
    void initProgramDesc(const ProgramInit& d) { _programDesc = d; }
    const ProgramInit& getProgramDesc() const  { return _programDesc; }

    id<MTLFunction>  _function { nil };    // for individual vertex/pixel/compute
    std::string      _mslSource;           // compiled MSL source (for debugging)
    std::string      _entryPoint;          // actual entry point name in MSL

    // Binding remap: maps HLSL register index → Metal buffer index
    // Built by parsing the HLSL ShaderIncludes to match names to HLSL registers,
    // then reflecting the MTLFunction arguments to find the Metal indices.
    std::unordered_map<uint32_t, uint32_t> _bufferBindingRemap;
    std::unordered_map<uint32_t, uint32_t> _textureBindingRemap;
};

// ---------------------------------------------------------------------------
// Sampler
// ---------------------------------------------------------------------------
class MetalSamplerBackend : public Sampler {
public:
    MetalSamplerBackend();
    virtual ~MetalSamplerBackend();

    id<MTLSamplerState> _sampler { nil };
};

// ---------------------------------------------------------------------------
// PipelineState
// ---------------------------------------------------------------------------
class MetalPipelineStateBackend : public PipelineState {
public:
    MetalPipelineStateBackend();
    virtual ~MetalPipelineStateBackend();

    void initGraphics(const GraphicsPipelineStateInit& init) {
        _type = PipelineType::GRAPHICS;
        _graphics = init;
        _program = init.program;
        _rootDescriptorLayout = init.rootDescriptorLayout;
    }
    void initCompute(const ComputePipelineStateInit& init) {
        _type = PipelineType::COMPUTE;
        _compute = init;
        _program = init.program;
        _rootDescriptorLayout = init.rootDescriptorLayout;
    }

    id<MTLRenderPipelineState>  _renderPipeline  { nil };
    id<MTLComputePipelineState> _computePipeline { nil };
    id<MTLDepthStencilState>    _depthStencilState { nil };
    MTLPrimitiveType            _primitiveType { MTLPrimitiveTypeTriangle };
    MTLSize                     _threadGroupSize { 1, 1, 1 };
    MTLCullMode                 _cullMode { MTLCullModeNone };
    MTLWinding                  _winding  { MTLWindingCounterClockwise };

    // Binding remap: HLSL register → Metal buffer/texture index
    // Built from MTLFunction reflection at pipeline creation time.
    // Key = HLSL name, Value = Metal buffer/texture index assigned by spirv-cross
    std::unordered_map<std::string, uint32_t> _vsBufferBindings;
    std::unordered_map<std::string, uint32_t> _fsBufferBindings;
    std::unordered_map<std::string, uint32_t> _vsTextureBindings;
    std::unordered_map<std::string, uint32_t> _fsTextureBindings;
};

// ---------------------------------------------------------------------------
// RootDescriptorLayout — stores binding info for Metal
// ---------------------------------------------------------------------------
class MetalRootDescriptorLayoutBackend : public RootDescriptorLayout {
public:
    MetalRootDescriptorLayoutBackend();
    virtual ~MetalRootDescriptorLayoutBackend();
    // Nothing extra needed — we use the _init data directly at bind time
};

// ---------------------------------------------------------------------------
// DescriptorHeap — Metal has no global descriptor heap; this is a stub
// ---------------------------------------------------------------------------
class MetalDescriptorHeapBackend : public DescriptorHeap {
public:
    MetalDescriptorHeapBackend();
    virtual ~MetalDescriptorHeapBackend();

    int32_t allocateDescriptors(int32_t numDescriptors) override { return 0; }
    int32_t allocateSamplers(int32_t numSamplers) override { return 0; }
};

// ---------------------------------------------------------------------------
// DescriptorSet — pre-baked binding list
// ---------------------------------------------------------------------------
struct MetalBinding {
    enum class Kind { Buffer, Texture, Sampler };
    Kind     kind;
    uint32_t slot;
    bool     isUBO { false };  // true for UNIFORM_BUFFER, false for RESOURCE_BUFFER
    id<MTLBuffer>       buffer  { nil };
    uint32_t            bufferOffset { 0 };
    id<MTLTexture>      texture { nil };
    id<MTLSamplerState> sampler { nil };
    ShaderStage         stage { ShaderStage::VERTEX };
};

class MetalDescriptorSetBackend : public DescriptorSet {
public:
    MetalDescriptorSetBackend();
    virtual ~MetalDescriptorSetBackend();

    std::vector<MetalBinding> _bindings;
};

// ---------------------------------------------------------------------------
// BatchTimer stub
// ---------------------------------------------------------------------------
class MetalBatchTimerBackend : public BatchTimer {
public:
    MetalBatchTimerBackend();
    virtual ~MetalBatchTimerBackend();
};

// ---------------------------------------------------------------------------
// Batch
// ---------------------------------------------------------------------------
class MetalBatchBackend : public Batch {
public:
    MetalBatchBackend();
    virtual ~MetalBatchBackend();

    id<MTLCommandBuffer>         _commandBuffer  { nil };
    id<MTLRenderCommandEncoder>  _renderEncoder  { nil };
    id<MTLComputeCommandEncoder> _computeEncoder { nil };
    id<CAMetalDrawable>          _currentDrawable { nil };

    // Bound pipeline state
    MetalPipelineStateBackend*   _currentPipeline { nullptr };

    // Keep swapchain/framebuffer so we can end encoders correctly
    bool _drawableAcquired { false };

    // The Metal device (needed for some operations)
    id<MTLDevice>  _device { nil };

    // Command queue + semaphore stored here for begin()
    id<MTLCommandQueue>   _commandQueue  { nil };
    dispatch_semaphore_t  _frameSemaphore { nullptr };

    // Current render pass descriptor (set by beginPass, used by ImGui)
    MTLRenderPassDescriptor* _currentRenderPassDescriptor { nil };

    // Pending clear info (set by clear(), consumed by beginPass() or end())
    bool         _hasPendingClear     { false };
    core::vec4   _pendingClearColor   { 0, 0, 0, 1 };
    float        _pendingClearDepth   { 0.0f };
    id<MTLTexture> _pendingClearDepthTex { nil };

    // Bound index buffer for drawIndexed
    id<MTLBuffer> _indexBuffer        { nil };
    uint32_t      _indexBufferOffset  { 0 };

    // Accumulated draw state
    id<MTLDepthStencilState> _defaultDepthStencilState { nil };

    // --- Batch API ---
    void begin(uint8_t currentIndex, const BatchTimerPointer& timer = nullptr) override;
    void end() override;
    void beginPass(const SwapchainPointer& swapchain, uint8_t currentIndex) override;
    void endPass() override;
    void clear(const SwapchainPointer& swapchain, uint8_t index, const core::vec4& color, float depth = 0.0f) override;
    void clear(const FramebufferPointer& framebuffer, const core::vec4& color, float depth = 0.0f) override;
    void resourceBarrierTransition(ResourceBarrierFlag, ResourceState, ResourceState,
                                   const SwapchainPointer&, uint8_t, uint32_t) override;
    void resourceBarrierTransition(ResourceBarrierFlag, ResourceState, ResourceState,
                                   const BufferPointer&) override;
    void resourceBarrierTransition(ResourceBarrierFlag, ResourceState, ResourceState,
                                   const TexturePointer&, uint32_t subresource) override;
    void resourceBarrierRW(ResourceBarrierFlag, const BufferPointer&) override;
    void resourceBarrierRW(ResourceBarrierFlag, const TexturePointer&, uint32_t) override;
    void bindFramebuffer(const FramebufferPointer& framebuffer) override;
    void bindRootDescriptorLayout(PipelineType type, const RootDescriptorLayoutPointer&) override;
    void bindPipeline(const PipelineStatePointer& pipeline) override;
    void bindDescriptorSet(PipelineType type, const DescriptorSetPointer& descriptorSet) override;
    void bindPushUniform(PipelineType type, uint32_t slot, uint32_t size, const uint8_t* data) override;
    void bindIndexBuffer(const BufferPointer& buffer) override;
    void bindVertexBuffers(uint32_t num, const BufferPointer* buffers) override;
    void draw(uint32_t numPrimitives, uint32_t startIndex) override;
    void drawIndexed(uint32_t numPrimitives, uint32_t startIndex) override;
    void uploadTexture(const TexturePointer& dest, const UploadSubresourceLayoutArray& layout, const BufferPointer& src) override;
    void uploadTexture(const TexturePointer& dest) override;
    void uploadTextureFromInitdata(const DevicePointer& device, const TexturePointer& dest, const std::vector<uint32_t>& subresources) override;
    void uploadBuffer(const BufferPointer& dest) override;
    void copyBufferRegion(const BufferPointer& dest, uint32_t destOffset, const BufferPointer& src, uint32_t srcOffset, uint32_t size) override;
    void dispatch(uint32_t numThreadsX, uint32_t numThreadsY, uint32_t numThreadsZ) override;
    void dispatchRays(const DispatchRaysArgs& args) override;

protected:
    void _setViewport(const core::vec4& viewport) override;
    void _setScissor(const core::vec4& scissor) override;

    // Helpers
    MTLRenderPassDescriptor* _makeSwapchainPassDescriptor(const SwapchainPointer& swapchain, uint8_t index, MTLLoadAction loadAction, const core::vec4& clearColor = {0,0,0,1}, float clearDepth = 0.0f);
    void _endCurrentEncoder();
};

// ---------------------------------------------------------------------------
// Device Backend
// ---------------------------------------------------------------------------
class MetalBackend : public DeviceBackend {
public:
    MetalBackend();
    virtual ~MetalBackend();

    void* nativeDevice() override { return (__bridge void*)_device; }

    id<MTLDevice>       _device       { nil };
    id<MTLCommandQueue> _commandQueue { nil };
    dispatch_semaphore_t _frameSemaphore;  // count=1, sequential frames

    // Global descriptor heap stub
    std::shared_ptr<MetalDescriptorHeapBackend> _descriptorHeap;

    // DeviceBackend interface
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
    DescriptorHeapPointer createDescriptorHeap(const DescriptorHeapInit& init) override;
    DescriptorHeapPointer getDescriptorHeap() override;
    DescriptorSetPointer createDescriptorSet(const DescriptorSetInit& init) override;
    void updateDescriptorSet(DescriptorSetPointer& descriptorSet, DescriptorObjects& objects) override;
    ShaderEntry getShaderEntry(const PipelineStatePointer& pipeline, const std::string& entry) override;
    ShaderTablePointer createShaderTable(const ShaderTableInit& init) override;
    void executeBatch(const BatchPointer& batch) override;
    void presentSwapchain(const SwapchainPointer& swapchain) override;
    void flush() override;

    // Helpers
    static MTLPixelFormat pixelFormatToMTL(PixelFormat fmt);

    // PixelFormat → MTLPixelFormat table
    static const MTLPixelFormat Format[(uint32_t)PixelFormat::COUNT];
};

// ---------------------------------------------------------------------------
// Helper functions for Imgui_mac.mm
// ---------------------------------------------------------------------------
id<MTLRenderCommandEncoder> MetalBatch_getCurrentEncoder(const BatchPointer& batch);
id<MTLCommandBuffer>        MetalBatch_getCurrentCommandBuffer(const BatchPointer& batch);
MTLRenderPassDescriptor*    MetalBatch_getCurrentRenderPassDescriptor(const BatchPointer& batch);

} // namespace graphics

#endif // __APPLE__
