// MetalBackend.mm
// Metal GPU backend — device initialization and factory function
//
// Sam Gateau / pico - 2024
// MIT License
#ifdef __APPLE__

#include "MetalBackend.h"
#include <iostream>

using namespace graphics;

// ---------------------------------------------------------------------------
// Factory function (called from Device.cpp via forward declaration)
// ---------------------------------------------------------------------------
namespace graphics {
    DeviceBackend* createMetalBackend() {
        return new MetalBackend();
    }
}

// ---------------------------------------------------------------------------
// PixelFormat → MTLPixelFormat table
// Must match the PixelFormat enum order in gpu.h exactly (66 entries).
// Note: R8G8B8A8_UNORM maps to BGRAunorm for CAMetalLayer compatibility.
// ---------------------------------------------------------------------------
const MTLPixelFormat MetalBackend::Format[(uint32_t)PixelFormat::COUNT] = {
    MTLPixelFormatInvalid,              // UNKNOWN
    MTLPixelFormatRGBA32Float,          // R32G32B32A32_TYPELESS
    MTLPixelFormatRGBA32Float,          // R32G32B32A32_FLOAT
    MTLPixelFormatRGBA32Uint,           // R32G32B32A32_UINT
    MTLPixelFormatRGBA32Sint,           // R32G32B32A32_SINT
    MTLPixelFormatInvalid,              // R32G32B32_TYPELESS (no 96-bit in Metal)
    MTLPixelFormatInvalid,              // R32G32B32_FLOAT
    MTLPixelFormatInvalid,              // R32G32B32_UINT
    MTLPixelFormatInvalid,              // R32G32B32_SINT
    MTLPixelFormatRGBA16Float,          // R16G16B16A16_TYPELESS
    MTLPixelFormatRGBA16Float,          // R16G16B16A16_FLOAT
    MTLPixelFormatRGBA16Unorm,          // R16G16B16A16_UNORM
    MTLPixelFormatRGBA16Uint,           // R16G16B16A16_UINT
    MTLPixelFormatRGBA16Snorm,          // R16G16B16A16_SNORM
    MTLPixelFormatRGBA16Sint,           // R16G16B16A16_SINT
    MTLPixelFormatRG32Float,            // R32G32_TYPELESS
    MTLPixelFormatRG32Float,            // R32G32_FLOAT
    MTLPixelFormatRG32Uint,             // R32G32_UINT
    MTLPixelFormatRG32Sint,             // R32G32_SINT
    MTLPixelFormatInvalid,              // R32G8X24_TYPELESS
    MTLPixelFormatDepth32Float_Stencil8,// D32_FLOAT_S8X24_UINT
    MTLPixelFormatInvalid,              // R32_FLOAT_X8X24_TYPELESS
    MTLPixelFormatInvalid,              // X32_TYPELESS_G8X24_UINT
    MTLPixelFormatRGB10A2Unorm,         // R10G10B10A2_TYPELESS
    MTLPixelFormatRGB10A2Unorm,         // R10G10B10A2_UNORM
    MTLPixelFormatRGB10A2Uint,          // R10G10B10A2_UINT
    MTLPixelFormatRGB10A2Unorm,         // R10G10B10_XR_BIAS_A2_UNORM
    MTLPixelFormatRG11B10Float,         // R11G11B10_FLOAT
    MTLPixelFormatBGRA8Unorm,           // R8G8B8A8_TYPELESS  → BGRA for Metal compat
    MTLPixelFormatBGRA8Unorm,           // R8G8B8A8_UNORM     → BGRA for CAMetalLayer
    MTLPixelFormatBGRA8Unorm_sRGB,      // R8G8B8A8_UNORM_SRGB → BGRA sRGB
    MTLPixelFormatRGBA8Uint,            // R8G8B8A8_UINT
    MTLPixelFormatRGBA8Snorm,           // R8G8B8A8_SNORM
    MTLPixelFormatRGBA8Sint,            // R8G8B8A8_SINT
    MTLPixelFormatRG16Float,            // R16G16_TYPELESS
    MTLPixelFormatRG16Float,            // R16G16_FLOAT
    MTLPixelFormatRG16Unorm,            // R16G16_UNORM
    MTLPixelFormatRG16Uint,             // R16G16_UINT
    MTLPixelFormatRG16Snorm,            // R16G16_SNORM
    MTLPixelFormatRG16Sint,             // R16G16_SINT
    MTLPixelFormatR32Float,             // R32_TYPELESS
    MTLPixelFormatDepth32Float,         // D32_FLOAT
    MTLPixelFormatR32Float,             // R32_FLOAT
    MTLPixelFormatR32Uint,              // R32_UINT
    MTLPixelFormatR32Sint,              // R32_SINT
    MTLPixelFormatInvalid,              // R24G8_TYPELESS
    MTLPixelFormatDepth32Float,         // D24_UNORM_S8_UINT (use D32 as fallback)
    MTLPixelFormatInvalid,              // R24_UNORM_X8_TYPELESS
    MTLPixelFormatInvalid,              // X24_TYPELESS_G8_UINT
    MTLPixelFormatRG8Unorm,             // R8G8_TYPELESS
    MTLPixelFormatRG8Unorm,             // R8G8_UNORM
    MTLPixelFormatRG8Uint,              // R8G8_UINT
    MTLPixelFormatRG8Snorm,             // R8G8_SNORM
    MTLPixelFormatRG8Sint,              // R8G8_SINT
    MTLPixelFormatR16Float,             // R16_TYPELESS
    MTLPixelFormatR16Float,             // R16_FLOAT
    MTLPixelFormatDepth16Unorm,         // D16_UNORM
    MTLPixelFormatR16Unorm,             // R16_UNORM
    MTLPixelFormatR16Uint,              // R16_UINT
    MTLPixelFormatR16Snorm,             // R16_SNORM
    MTLPixelFormatR16Sint,              // R16_SINT
    MTLPixelFormatR8Unorm,              // R8_TYPELESS
    MTLPixelFormatR8Unorm,              // R8_UNORM
    MTLPixelFormatR8Uint,               // R8_UINT
    MTLPixelFormatR8Snorm,              // R8_SNORM
    MTLPixelFormatR8Sint,               // R8_SINT
};

// ---------------------------------------------------------------------------
// MetalBackend constructor / destructor
// ---------------------------------------------------------------------------
MetalBackend::MetalBackend() {
    _device = MTLCreateSystemDefaultDevice();
    if (!_device) {
        std::cerr << "[Metal] No Metal device found!\n";
        return;
    }
    std::cout << "[Metal] Device: " << [[_device name] UTF8String] << "\n";

    _commandQueue = [_device newCommandQueue];
    _frameSemaphore = dispatch_semaphore_create(1); // sequential: 1 frame in flight

    _descriptorHeap = std::make_shared<MetalDescriptorHeapBackend>();
}

MetalBackend::~MetalBackend() {
    _commandQueue = nil;
    _device = nil;
}

// ---------------------------------------------------------------------------
// Misc DeviceBackend methods
// ---------------------------------------------------------------------------
DescriptorHeapPointer MetalBackend::getDescriptorHeap() {
    return _descriptorHeap;
}

DescriptorHeapPointer MetalBackend::createDescriptorHeap(const DescriptorHeapInit& init) {
    return _descriptorHeap;
}

void MetalBackend::flush() {
    // no-op; Metal handles sync via semaphores
}

// ---------------------------------------------------------------------------
// Geometry (stub)
// ---------------------------------------------------------------------------
GeometryPointer MetalBackend::createGeometry(const GeometryInit& init) {
    return GeometryPointer(new MetalGeometryBackend());
}

// ---------------------------------------------------------------------------
// ShaderTable (stub for raytracing)
// ---------------------------------------------------------------------------
ShaderTablePointer MetalBackend::createShaderTable(const ShaderTableInit& init) {
    return nullptr;
}
ShaderEntry MetalBackend::getShaderEntry(const PipelineStatePointer& pipeline, const std::string& entry) {
    return {};
}

// ---------------------------------------------------------------------------
// Raytacing pipeline (stub)
// ---------------------------------------------------------------------------
PipelineStatePointer MetalBackend::createRaytracingPipelineState(const RaytracingPipelineStateInit& init) {
    return nullptr;
}

// ---------------------------------------------------------------------------
// BatchTimer (stub)
// ---------------------------------------------------------------------------
BatchTimerPointer MetalBackend::createBatchTimer(const BatchTimerInit& init) {
    return BatchTimerPointer(new MetalBatchTimerBackend());
}

// ---------------------------------------------------------------------------
// Present / Execute
// ---------------------------------------------------------------------------
void MetalBackend::executeBatch(const BatchPointer& batch) {
    auto* mb = static_cast<MetalBatchBackend*>(batch.get());
    if (!mb->_commandBuffer) return;

    // Present the drawable if we acquired one
    if (mb->_currentDrawable) {
        [mb->_commandBuffer presentDrawable:mb->_currentDrawable];
    }

    // Signal semaphore when GPU finishes
    dispatch_semaphore_t sem = _frameSemaphore;
    [mb->_commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull) {
        dispatch_semaphore_signal(sem);
    }];

    [mb->_commandBuffer commit];
    mb->_commandBuffer = nil;
    mb->_currentDrawable = nil;
    mb->_drawableAcquired = false;
}

void MetalBackend::presentSwapchain(const SwapchainPointer& swapchain) {
    // Present is handled in executeBatch; update currentIndex here
    auto* sw = static_cast<MetalSwapchainBackend*>(swapchain.get());
    sw->_currentIndex = (sw->_currentIndex + 1) % 3;
}

// ---------------------------------------------------------------------------
// RootDescriptorLayout
// ---------------------------------------------------------------------------
RootDescriptorLayoutPointer MetalBackend::createRootDescriptorLayout(const RootDescriptorLayoutInit& init) {
    auto* rdl = new MetalRootDescriptorLayoutBackend();
    rdl->_init = init;
    return RootDescriptorLayoutPointer(rdl);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace graphics {
id<MTLRenderCommandEncoder> MetalBatch_getCurrentEncoder(const BatchPointer& batch) {
    auto* mb = static_cast<MetalBatchBackend*>(batch.get());
    return mb->_renderEncoder;
}
id<MTLCommandBuffer> MetalBatch_getCurrentCommandBuffer(const BatchPointer& batch) {
    auto* mb = static_cast<MetalBatchBackend*>(batch.get());
    return mb->_commandBuffer;
}
MTLRenderPassDescriptor* MetalBatch_getCurrentRenderPassDescriptor(const BatchPointer& batch) {
    auto* mb = static_cast<MetalBatchBackend*>(batch.get());
    return mb->_currentRenderPassDescriptor;
}
} // namespace graphics

// ---------------------------------------------------------------------------
// Stubs for constructors/destructors of backend objects
// ---------------------------------------------------------------------------
MetalSwapchainBackend::MetalSwapchainBackend()  {}
MetalSwapchainBackend::~MetalSwapchainBackend() {}

MetalFramebufferBackend::MetalFramebufferBackend()  {}
MetalFramebufferBackend::~MetalFramebufferBackend() {}

MetalBufferBackend::MetalBufferBackend()  {}
MetalBufferBackend::~MetalBufferBackend() {}

MetalTextureBackend::MetalTextureBackend()  {}
MetalTextureBackend::~MetalTextureBackend() {}

MetalGeometryBackend::MetalGeometryBackend()  {}
MetalGeometryBackend::~MetalGeometryBackend() {}

MetalShaderBackend::MetalShaderBackend()  {}
MetalShaderBackend::~MetalShaderBackend() {}

MetalSamplerBackend::MetalSamplerBackend()  {}
MetalSamplerBackend::~MetalSamplerBackend() {}

MetalPipelineStateBackend::MetalPipelineStateBackend()  {}
MetalPipelineStateBackend::~MetalPipelineStateBackend() {}

MetalRootDescriptorLayoutBackend::MetalRootDescriptorLayoutBackend()  {}
MetalRootDescriptorLayoutBackend::~MetalRootDescriptorLayoutBackend() {}

MetalDescriptorHeapBackend::MetalDescriptorHeapBackend()  {}
MetalDescriptorHeapBackend::~MetalDescriptorHeapBackend() {}

MetalDescriptorSetBackend::MetalDescriptorSetBackend()  {}
MetalDescriptorSetBackend::~MetalDescriptorSetBackend() {}

MetalBatchTimerBackend::MetalBatchTimerBackend()  {}
MetalBatchTimerBackend::~MetalBatchTimerBackend() {}

MetalBatchBackend::MetalBatchBackend()  {}
MetalBatchBackend::~MetalBatchBackend() {}

#endif // __APPLE__
