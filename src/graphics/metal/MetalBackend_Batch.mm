// MetalBackend_Batch.mm
// Command buffer, render/compute encoders, and draw calls for pico Metal backend
//
// Sam Gateau / pico - 2024
// MIT License
#ifdef __APPLE__

#include "MetalBackend.h"
#include <iostream>
#include <set>

using namespace graphics;

// Vertex buffers are placed at high slots to avoid conflicts with uniform buffers.
static const uint32_t VERTEX_BUFFER_SLOT_OFFSET = 24;

// ---------------------------------------------------------------------------
// createBatch
// ---------------------------------------------------------------------------
BatchPointer MetalBackend::createBatch(const BatchInit& init) {
    auto* b           = new MetalBatchBackend();
    b->_device        = _device;
    b->_commandQueue  = _commandQueue;
    b->_frameSemaphore = _frameSemaphore;

    // Create a default "no depth test" depth-stencil state
    MTLDepthStencilDescriptor* dsd = [[MTLDepthStencilDescriptor alloc] init];
    dsd.depthCompareFunction = MTLCompareFunctionAlways;
    dsd.depthWriteEnabled    = NO;
    b->_defaultDepthStencilState = [_device newDepthStencilStateWithDescriptor:dsd];

    return BatchPointer(b);
}

// ---------------------------------------------------------------------------
// Batch::begin — wait for previous frame, create command buffer
// ---------------------------------------------------------------------------
void MetalBatchBackend::begin(uint8_t currentIndex, const BatchTimerPointer& timer) {
    dispatch_semaphore_wait(_frameSemaphore, DISPATCH_TIME_FOREVER);
    _commandBuffer      = [_commandQueue commandBuffer];
    _hasPendingClear    = false;
    _drawableAcquired   = false;
    _currentDrawable    = nil;
    _currentPipeline    = nullptr;
    _indexBuffer        = nil;
    _indexBufferOffset  = 0;
}

// ---------------------------------------------------------------------------
// Batch::end — flush any pending clear, end open encoders
// ---------------------------------------------------------------------------
void MetalBatchBackend::end() {
    // If clear() was called but beginPass() was never called, apply the clear now
    if (_hasPendingClear && _currentDrawable) {
        MTLRenderPassDescriptor* rpd = [MTLRenderPassDescriptor renderPassDescriptor];
        rpd.colorAttachments[0].texture     = _currentDrawable.texture;
        rpd.colorAttachments[0].loadAction  = MTLLoadActionClear;
        rpd.colorAttachments[0].storeAction = MTLStoreActionStore;
        rpd.colorAttachments[0].clearColor  = MTLClearColorMake(
            _pendingClearColor.x, _pendingClearColor.y,
            _pendingClearColor.z, _pendingClearColor.w);
        if (_pendingClearDepthTex) {
            rpd.depthAttachment.texture     = _pendingClearDepthTex;
            rpd.depthAttachment.loadAction  = MTLLoadActionClear;
            rpd.depthAttachment.storeAction = MTLStoreActionDontCare;
            rpd.depthAttachment.clearDepth  = _pendingClearDepth;
        }
        auto* enc = [_commandBuffer renderCommandEncoderWithDescriptor:rpd];
        [enc endEncoding];
        _hasPendingClear = false;
    }
    _endCurrentEncoder();
}

// ---------------------------------------------------------------------------
// _endCurrentEncoder — close any open render or compute encoder
// ---------------------------------------------------------------------------
void MetalBatchBackend::_endCurrentEncoder() {
    if (_renderEncoder) {
        [_renderEncoder endEncoding];
        _renderEncoder = nil;
    }
    if (_computeEncoder) {
        [_computeEncoder endEncoding];
        _computeEncoder = nil;
    }
}

// ---------------------------------------------------------------------------
// beginPass — acquire drawable, build render-pass descriptor, open encoder
// ---------------------------------------------------------------------------
void MetalBatchBackend::beginPass(const SwapchainPointer& swapchain, uint8_t currentIndex) {
    _endCurrentEncoder();

    auto* sw = static_cast<MetalSwapchainBackend*>(swapchain.get());
    if (!_drawableAcquired) {
        sw->acquireNextDrawable();
        _currentDrawable  = sw->_currentDrawable;
        _drawableAcquired = true;
    }
    if (!_currentDrawable) {
        std::cerr << "[Metal] beginPass: no drawable\n";
        return;
    }

    MTLLoadAction la = _hasPendingClear ? MTLLoadActionClear : MTLLoadActionLoad;
    auto* rpd = _makeSwapchainPassDescriptor(swapchain, currentIndex, la,
                                              _pendingClearColor, _pendingClearDepth);
    _hasPendingClear = false;
    _currentRenderPassDescriptor = rpd;

    _renderEncoder = [_commandBuffer renderCommandEncoderWithDescriptor:rpd];

    // Apply default depth-stencil state (no depth test) so encoder is valid
    [_renderEncoder setDepthStencilState:_defaultDepthStencilState];
}

// ---------------------------------------------------------------------------
// endPass
// ---------------------------------------------------------------------------
void MetalBatchBackend::endPass() {
    _endCurrentEncoder();
}

// ---------------------------------------------------------------------------
// clear(swapchain) — store pending clear; acquire drawable if not yet done
// ---------------------------------------------------------------------------
void MetalBatchBackend::clear(const SwapchainPointer& swapchain, uint8_t index,
                               const core::vec4& color, float depth) {
    auto* sw = static_cast<MetalSwapchainBackend*>(swapchain.get());
    if (!_drawableAcquired) {
        sw->acquireNextDrawable();
        _currentDrawable  = sw->_currentDrawable;
        _drawableAcquired = true;
    }
    _hasPendingClear      = true;
    _pendingClearColor    = color;
    _pendingClearDepth    = depth;
    _pendingClearDepthTex = sw->_depthTexture;
}

// ---------------------------------------------------------------------------
// clear(framebuffer)
// ---------------------------------------------------------------------------
void MetalBatchBackend::clear(const FramebufferPointer& framebuffer,
                               const core::vec4& color, float depth) {
    _endCurrentEncoder();

    auto* fb = static_cast<MetalFramebufferBackend*>(framebuffer.get());
    MTLRenderPassDescriptor* rpd = [MTLRenderPassDescriptor renderPassDescriptor];

    for (size_t i = 0; i < fb->_colorTextures.size(); ++i) {
        rpd.colorAttachments[i].texture     = fb->_colorTextures[i];
        rpd.colorAttachments[i].loadAction  = MTLLoadActionClear;
        rpd.colorAttachments[i].storeAction = MTLStoreActionStore;
        rpd.colorAttachments[i].clearColor  = MTLClearColorMake(color.x, color.y, color.z, color.w);
    }
    if (fb->_depthTexture) {
        rpd.depthAttachment.texture     = fb->_depthTexture;
        rpd.depthAttachment.loadAction  = MTLLoadActionClear;
        rpd.depthAttachment.storeAction = MTLStoreActionDontCare;
        rpd.depthAttachment.clearDepth  = depth;
    }

    auto* enc = [_commandBuffer renderCommandEncoderWithDescriptor:rpd];
    [enc endEncoding];
}

// ---------------------------------------------------------------------------
// bindFramebuffer — start a new render pass targeting framebuffer textures
// ---------------------------------------------------------------------------
void MetalBatchBackend::bindFramebuffer(const FramebufferPointer& framebuffer) {
    _endCurrentEncoder();
    if (!framebuffer) return;

    auto* fb = static_cast<MetalFramebufferBackend*>(framebuffer.get());
    MTLRenderPassDescriptor* rpd = [MTLRenderPassDescriptor renderPassDescriptor];

    for (size_t i = 0; i < fb->_colorTextures.size(); ++i) {
        rpd.colorAttachments[i].texture     = fb->_colorTextures[i];
        rpd.colorAttachments[i].loadAction  = MTLLoadActionLoad;
        rpd.colorAttachments[i].storeAction = MTLStoreActionStore;
    }
    if (fb->_depthTexture) {
        rpd.depthAttachment.texture     = fb->_depthTexture;
        rpd.depthAttachment.loadAction  = MTLLoadActionLoad;
        rpd.depthAttachment.storeAction = MTLStoreActionDontCare;
    }

    _renderEncoder = [_commandBuffer renderCommandEncoderWithDescriptor:rpd];
    [_renderEncoder setDepthStencilState:_defaultDepthStencilState];
}

// ---------------------------------------------------------------------------
// Resource barriers — no-ops in Metal (automatic hazard tracking)
// ---------------------------------------------------------------------------
void MetalBatchBackend::resourceBarrierTransition(
    ResourceBarrierFlag, ResourceState, ResourceState,
    const SwapchainPointer&, uint8_t, uint32_t) {}

void MetalBatchBackend::resourceBarrierTransition(
    ResourceBarrierFlag, ResourceState, ResourceState,
    const BufferPointer&) {}

void MetalBatchBackend::resourceBarrierTransition(
    ResourceBarrierFlag, ResourceState, ResourceState,
    const TexturePointer&, uint32_t) {}

void MetalBatchBackend::resourceBarrierRW(ResourceBarrierFlag, const BufferPointer&) {}
void MetalBatchBackend::resourceBarrierRW(ResourceBarrierFlag, const TexturePointer&, uint32_t) {}

// ---------------------------------------------------------------------------
// bindRootDescriptorLayout — no-op in Metal
// ---------------------------------------------------------------------------
void MetalBatchBackend::bindRootDescriptorLayout(PipelineType,
                                                   const RootDescriptorLayoutPointer&) {}

// ---------------------------------------------------------------------------
// bindPipeline
// ---------------------------------------------------------------------------
void MetalBatchBackend::bindPipeline(const PipelineStatePointer& pipeline) {
    auto* pso = static_cast<MetalPipelineStateBackend*>(pipeline.get());
    _currentPipeline = pso;

    if (pso->_renderPipeline) {
        // If we were in compute mode, switch back to render
        if (!_renderEncoder && _currentRenderPassDescriptor) {
            _endCurrentEncoder();
            // Restart the render pass with LoadAction::Load to preserve existing content
            _currentRenderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionLoad;
            if (_currentRenderPassDescriptor.depthAttachment.texture)
                _currentRenderPassDescriptor.depthAttachment.loadAction = MTLLoadActionLoad;
            _renderEncoder = [_commandBuffer renderCommandEncoderWithDescriptor:_currentRenderPassDescriptor];
            [_renderEncoder setDepthStencilState:_defaultDepthStencilState];
        }
        if (_renderEncoder) {
            [_renderEncoder setRenderPipelineState:pso->_renderPipeline];
            if (pso->_depthStencilState)
                [_renderEncoder setDepthStencilState:pso->_depthStencilState];
            else
                [_renderEncoder setDepthStencilState:_defaultDepthStencilState];
            [_renderEncoder setCullMode:pso->_cullMode];
            [_renderEncoder setFrontFacingWinding:pso->_winding];
        }
    } else if (pso->_computePipeline) {
        // Create compute encoder if needed
        if (!_computeEncoder) {
            _endCurrentEncoder();
            _computeEncoder = [_commandBuffer computeCommandEncoder];
        }
        [_computeEncoder setComputePipelineState:pso->_computePipeline];
    }
}

// ---------------------------------------------------------------------------
// bindDescriptorSet — apply Metal bindings to the current encoder
// ---------------------------------------------------------------------------
void MetalBatchBackend::bindDescriptorSet(PipelineType type,
                                           const DescriptorSetPointer& descriptorSet) {
    if (!descriptorSet) return;
    auto* ds = static_cast<MetalDescriptorSetBackend*>(descriptorSet.get());

    // SSBO bindings are shifted by 9 in the HLSL→MSL conversion (scribe -msl)
    // to match the glslang --shift-ssbo-binding flag. UBOs keep original bindings.
    static const uint32_t SSBO_BINDING_SHIFT = 9;

    for (auto& b : ds->_bindings) {
        uint16_t stageRaw = (uint16_t)b.stage;
        bool isVS  = (stageRaw & (uint16_t)ShaderStage::VERTEX)  != 0;
        bool isPS  = (stageRaw & (uint16_t)ShaderStage::PIXEL)   != 0;
        bool isAll = (b.stage == ShaderStage::ALL_GRAPHICS);

        // SSBOs (RESOURCE_BUFFER) are shifted, UBOs keep their HLSL register binding
        uint32_t slot = b.slot + (b.isUBO ? 0 : SSBO_BINDING_SHIFT);

        if (_renderEncoder) {
            switch (b.kind) {
                case MetalBinding::Kind::Buffer:
                    if (isVS || isAll)
                        [_renderEncoder setVertexBuffer:b.buffer offset:b.bufferOffset atIndex:slot];
                    if (isPS || isAll)
                        [_renderEncoder setFragmentBuffer:b.buffer offset:b.bufferOffset atIndex:slot];
                    // Also bind as texture for Buffer<T> types (spirv-cross maps to texture2d)
                    if (b.texture) {
                        if (isVS || isAll)
                            [_renderEncoder setVertexTexture:b.texture atIndex:b.slot];
                        if (isPS || isAll)
                            [_renderEncoder setFragmentTexture:b.texture atIndex:b.slot];
                    }
                    break;
                case MetalBinding::Kind::Texture:
                    if (isVS || isAll)
                        [_renderEncoder setVertexTexture:b.texture atIndex:b.slot];
                    if (isPS || isAll)
                        [_renderEncoder setFragmentTexture:b.texture atIndex:b.slot];
                    break;
                case MetalBinding::Kind::Sampler:
                    if (isVS || isAll)
                        [_renderEncoder setVertexSamplerState:b.sampler atIndex:b.slot];
                    if (isPS || isAll)
                        [_renderEncoder setFragmentSamplerState:b.sampler atIndex:b.slot];
                    break;
            }
        } else if (_computeEncoder) {
            switch (b.kind) {
                case MetalBinding::Kind::Buffer:
                    [_computeEncoder setBuffer:b.buffer offset:b.bufferOffset atIndex:slot];
                    // Also bind texture view for Buffer<T>/RWBuffer<T>
                    if (b.texture)
                        [_computeEncoder setTexture:b.texture atIndex:b.slot];
                    break;
                case MetalBinding::Kind::Texture:
                    [_computeEncoder setTexture:b.texture atIndex:b.slot];
                    break;
                case MetalBinding::Kind::Sampler:
                    [_computeEncoder setSamplerState:b.sampler atIndex:b.slot];
                    break;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// bindPushUniform — inline constant data via setXxxBytes
// ---------------------------------------------------------------------------
void MetalBatchBackend::bindPushUniform(PipelineType type, uint32_t slot,
                                         uint32_t size, const uint8_t* data) {
    // The 'slot' parameter is the index into the root descriptor layout's push layout.
    // Resolve to the HLSL register binding (UBOs are not shifted).
    uint32_t metalSlot = slot;
    if (_currentPipeline) {
        auto rdl = _currentPipeline->getRootDescriptorLayout();
        if (rdl && slot < (uint32_t)rdl->_init._pushLayout.size()) {
            metalSlot = rdl->_init._pushLayout[slot]._binding;
        }
    }

    if (_renderEncoder) {
        [_renderEncoder setVertexBytes:data   length:size atIndex:metalSlot];
        [_renderEncoder setFragmentBytes:data length:size atIndex:metalSlot];
    } else if (_computeEncoder) {
        [_computeEncoder setBytes:data length:size atIndex:metalSlot];
    }
}

// ---------------------------------------------------------------------------
// bindVertexBuffers — vertex attribute data goes to high buffer slots
// ---------------------------------------------------------------------------
void MetalBatchBackend::bindVertexBuffers(uint32_t num, const BufferPointer* buffers) {
    if (!_renderEncoder) return;
    for (uint32_t i = 0; i < num; ++i) {
        if (!buffers[i]) continue;
        auto* buf = static_cast<MetalBufferBackend*>(buffers[i].get());
        NSUInteger slot = VERTEX_BUFFER_SLOT_OFFSET + i;
        [_renderEncoder setVertexBuffer:buf->_buffer offset:0 atIndex:slot];
    }
}

// ---------------------------------------------------------------------------
// bindIndexBuffer
// ---------------------------------------------------------------------------
void MetalBatchBackend::bindIndexBuffer(const BufferPointer& buffer) {
    if (!buffer) return;
    auto* buf = static_cast<MetalBufferBackend*>(buffer.get());
    _indexBuffer       = buf->_buffer;
    _indexBufferOffset = 0;
}

// ---------------------------------------------------------------------------
// draw
// ---------------------------------------------------------------------------
void MetalBatchBackend::draw(uint32_t numPrimitives, uint32_t startIndex) {
    if (!_renderEncoder || !_currentPipeline) return;
    [_renderEncoder drawPrimitives:_currentPipeline->_primitiveType
                       vertexStart:startIndex
                       vertexCount:numPrimitives];
}

// ---------------------------------------------------------------------------
// drawIndexed  (indices are uint32)
// ---------------------------------------------------------------------------
void MetalBatchBackend::drawIndexed(uint32_t numPrimitives, uint32_t startIndex) {
    if (!_renderEncoder || !_currentPipeline || !_indexBuffer) return;
    [_renderEncoder drawIndexedPrimitives:_currentPipeline->_primitiveType
                               indexCount:numPrimitives
                                indexType:MTLIndexTypeUInt32
                              indexBuffer:_indexBuffer
                        indexBufferOffset:_indexBufferOffset + startIndex * sizeof(uint32_t)];
}

// ---------------------------------------------------------------------------
// Viewport and Scissor
// ---------------------------------------------------------------------------
void MetalBatchBackend::_setViewport(const core::vec4& vp) {
    if (!_renderEncoder) return;
    MTLViewport mtlVP;
    mtlVP.originX = vp.x;
    mtlVP.originY = vp.y;
    mtlVP.width   = vp.z;
    mtlVP.height  = vp.w;
    mtlVP.znear   = 0.0;
    mtlVP.zfar    = 1.0;
    [_renderEncoder setViewport:mtlVP];
}

void MetalBatchBackend::_setScissor(const core::vec4& sc) {
    if (!_renderEncoder) return;
    MTLScissorRect rect;
    rect.x      = (NSUInteger)sc.x;
    rect.y      = (NSUInteger)sc.y;
    rect.width  = (NSUInteger)sc.z;
    rect.height = (NSUInteger)sc.w;
    [_renderEncoder setScissorRect:rect];
}

// ---------------------------------------------------------------------------
// Compute dispatch
// ---------------------------------------------------------------------------
void MetalBatchBackend::dispatch(uint32_t x, uint32_t y, uint32_t z) {
    if (!_computeEncoder || !_currentPipeline || !_currentPipeline->_computePipeline) return;
    MTLSize threadgroups = MTLSizeMake(x, y, z);
    // Thread group size is baked into the compute pipeline from [numthreads(X,Y,Z)]
    MTLSize threadsPerGroup = _currentPipeline->_threadGroupSize;
    [_computeEncoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadsPerGroup];
}

void MetalBatchBackend::dispatchRays(const DispatchRaysArgs&) {
    // Raytracing stub
}

// ---------------------------------------------------------------------------
// Upload operations
// ---------------------------------------------------------------------------
void MetalBatchBackend::uploadTexture(const TexturePointer& dest,
                                       const UploadSubresourceLayoutArray& layout,
                                       const BufferPointer& src) {
    if (!dest || !src) return;
    auto* tex  = static_cast<MetalTextureBackend*>(dest.get());
    auto* buf  = static_cast<MetalBufferBackend*>(src.get());
    if (!tex->_texture || !buf->_buffer) return;

    _endCurrentEncoder();
    id<MTLBlitCommandEncoder> blit = [_commandBuffer blitCommandEncoder];
    for (auto& sub : layout) {
        NSUInteger sliceIdx = 0;
        NSUInteger mipIdx   = 0;
        NSUInteger w = tex->_init.width;
        NSUInteger h = tex->_init.height;
        NSUInteger bpp = 4; // assume 4 bytes per pixel (RGBA8)

        [blit copyFromBuffer:buf->_buffer
                sourceOffset:(NSUInteger)sub.byteOffset
           sourceBytesPerRow:w * bpp
         sourceBytesPerImage:w * h * bpp
                  sourceSize:MTLSizeMake(w, h, 1)
                   toTexture:tex->_texture
            destinationSlice:sliceIdx
            destinationLevel:mipIdx
           destinationOrigin:MTLOriginMake(0, 0, 0)];
    }
    [blit endEncoding];
    dest->notifyUploaded();
}

void MetalBatchBackend::uploadTexture(const TexturePointer& dest) {
    if (!dest) return;
    auto* tex = static_cast<MetalTextureBackend*>(dest.get());
    if (!tex->_texture || dest->_cpuDataBuffer == nullptr) return;

    // Upload from the CPU data buffer
    auto* cpuBuf = static_cast<MetalBufferBackend*>(dest->_cpuDataBuffer.get());
    if (!cpuBuf || !cpuBuf->_buffer) return;

    _endCurrentEncoder();
    id<MTLBlitCommandEncoder> blit = [_commandBuffer blitCommandEncoder];
    NSUInteger w   = tex->_init.width;
    NSUInteger h   = tex->_init.height;
    NSUInteger bpp = 4;

    [blit copyFromBuffer:cpuBuf->_buffer
            sourceOffset:0
       sourceBytesPerRow:w * bpp
     sourceBytesPerImage:w * h * bpp
              sourceSize:MTLSizeMake(w, h, 1)
               toTexture:tex->_texture
        destinationSlice:0
        destinationLevel:0
       destinationOrigin:MTLOriginMake(0, 0, 0)];
    [blit endEncoding];
    dest->notifyUploaded();
}

void MetalBatchBackend::uploadTextureFromInitdata(const DevicePointer& device,
                                                   const TexturePointer& dest,
                                                   const std::vector<uint32_t>& subresources) {
    if (!dest || dest->_init.initData.empty()) return;
    auto* tex = static_cast<MetalTextureBackend*>(dest.get());
    if (!tex->_texture) return;

    _endCurrentEncoder();
    id<MTLBlitCommandEncoder> blit = [_commandBuffer blitCommandEncoder];
    NSUInteger slice = 0;
    for (auto& mipData : dest->_init.initData) {
        if (mipData.empty()) { ++slice; continue; }
        NSUInteger w   = tex->_init.width;
        NSUInteger h   = tex->_init.height;
        NSUInteger bpp = 4;

        // Create a temporary shared buffer
        id<MTLBuffer> tmp = [_device newBufferWithBytes:mipData.data()
                                                 length:mipData.size()
                                                options:MTLResourceStorageModeShared];
        [blit copyFromBuffer:tmp
                sourceOffset:0
           sourceBytesPerRow:w * bpp
         sourceBytesPerImage:w * h * bpp
                  sourceSize:MTLSizeMake(w, h, 1)
                   toTexture:tex->_texture
            destinationSlice:slice
            destinationLevel:0
           destinationOrigin:MTLOriginMake(0, 0, 0)];
        ++slice;
    }
    [blit endEncoding];
    dest->notifyUploaded();
}

void MetalBatchBackend::uploadBuffer(const BufferPointer& dest) {
    // For hostVisible (shared) buffers the data is already in the GPU-visible
    // MTLBuffer, so no explicit upload is needed.
    if (dest) dest->notifyUploaded();
}

void MetalBatchBackend::copyBufferRegion(const BufferPointer& dst, uint32_t dstOff,
                                          const BufferPointer& src, uint32_t srcOff,
                                          uint32_t size) {
    if (!dst || !src) return;
    auto* d = static_cast<MetalBufferBackend*>(dst.get());
    auto* s = static_cast<MetalBufferBackend*>(src.get());
    if (!d->_buffer || !s->_buffer) return;

    _endCurrentEncoder();
    id<MTLBlitCommandEncoder> blit = [_commandBuffer blitCommandEncoder];
    [blit copyFromBuffer:s->_buffer
            sourceOffset:srcOff
                toBuffer:d->_buffer
       destinationOffset:dstOff
                    size:size];
    [blit endEncoding];
}

// ---------------------------------------------------------------------------
// _makeSwapchainPassDescriptor helper
// ---------------------------------------------------------------------------
MTLRenderPassDescriptor*
MetalBatchBackend::_makeSwapchainPassDescriptor(const SwapchainPointer& swapchain,
                                                  uint8_t index,
                                                  MTLLoadAction loadAction,
                                                  const core::vec4& clearColor,
                                                  float clearDepth) {
    auto* sw = static_cast<MetalSwapchainBackend*>(swapchain.get());
    MTLRenderPassDescriptor* rpd = [MTLRenderPassDescriptor renderPassDescriptor];

    rpd.colorAttachments[0].texture     = _currentDrawable.texture;
    rpd.colorAttachments[0].loadAction  = loadAction;
    rpd.colorAttachments[0].storeAction = MTLStoreActionStore;
    rpd.colorAttachments[0].clearColor  =
        MTLClearColorMake(clearColor.x, clearColor.y, clearColor.z, clearColor.w);

    if (sw->_depthTexture) {
        rpd.depthAttachment.texture     = sw->_depthTexture;
        rpd.depthAttachment.loadAction  = loadAction;
        rpd.depthAttachment.storeAction = MTLStoreActionDontCare;
        rpd.depthAttachment.clearDepth  = clearDepth;
    }

    return rpd;
}

#endif // __APPLE__
