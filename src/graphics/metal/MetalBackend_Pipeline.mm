// MetalBackend_Pipeline.mm
// Render and compute pipeline state creation for pico Metal backend
//
// Sam Gateau / pico - 2024
// MIT License
#ifdef __APPLE__

#include "MetalBackend.h"
#include <iostream>

using namespace graphics;

// ---------------------------------------------------------------------------
// Enum conversion helpers
// ---------------------------------------------------------------------------
static MTLPrimitiveType toMTLPrimitive(PrimitiveTopology t) {
    switch (t) {
        case PrimitiveTopology::POINT:          return MTLPrimitiveTypePoint;
        case PrimitiveTopology::LINE:           return MTLPrimitiveTypeLine;
        case PrimitiveTopology::TRIANGLE:       return MTLPrimitiveTypeTriangle;
        case PrimitiveTopology::TRIANGLE_STRIP: return MTLPrimitiveTypeTriangleStrip;
        default:                                return MTLPrimitiveTypeTriangle;
    }
}

static MTLCompareFunction toMTLCompare(ComparisonFunction f) {
    switch (f) {
        case ComparisonFunction::NEVER:         return MTLCompareFunctionNever;
        case ComparisonFunction::LESS:          return MTLCompareFunctionLess;
        case ComparisonFunction::EQUAL:         return MTLCompareFunctionEqual;
        case ComparisonFunction::LESS_EQUAL:    return MTLCompareFunctionLessEqual;
        case ComparisonFunction::GREATER:       return MTLCompareFunctionGreater;
        case ComparisonFunction::NOT_EQUAL:     return MTLCompareFunctionNotEqual;
        case ComparisonFunction::GREATER_EQUAL: return MTLCompareFunctionGreaterEqual;
        case ComparisonFunction::ALWAYS:        return MTLCompareFunctionAlways;
        default:                                return MTLCompareFunctionAlways;
    }
}

static MTLBlendFactor toMTLBlendFactor(BlendArg a) {
    switch (a) {
        case BlendArg::ZERO:           return MTLBlendFactorZero;
        case BlendArg::ONE:            return MTLBlendFactorOne;
        case BlendArg::SRC_COLOR:      return MTLBlendFactorSourceColor;
        case BlendArg::INV_SRC_COLOR:  return MTLBlendFactorOneMinusSourceColor;
        case BlendArg::SRC_ALPHA:      return MTLBlendFactorSourceAlpha;
        case BlendArg::INV_SRC_ALPHA:  return MTLBlendFactorOneMinusSourceAlpha;
        case BlendArg::DEST_ALPHA:     return MTLBlendFactorDestinationAlpha;
        case BlendArg::INV_DEST_ALPHA: return MTLBlendFactorOneMinusDestinationAlpha;
        case BlendArg::DEST_COLOR:     return MTLBlendFactorDestinationColor;
        case BlendArg::INV_DEST_COLOR: return MTLBlendFactorOneMinusDestinationColor;
        default:                       return MTLBlendFactorOne;
    }
}

static MTLBlendOperation toMTLBlendOp(BlendOp op) {
    switch (op) {
        case BlendOp::ADD:          return MTLBlendOperationAdd;
        case BlendOp::SUBTRACT:     return MTLBlendOperationSubtract;
        case BlendOp::REV_SUBTRACT: return MTLBlendOperationReverseSubtract;
        case BlendOp::MIN:          return MTLBlendOperationMin;
        case BlendOp::MAX:          return MTLBlendOperationMax;
        default:                    return MTLBlendOperationAdd;
    }
}

static MTLVertexFormat toMTLVertexFormat(AttribFormat fmt) {
    switch (fmt) {
        case AttribFormat::UINT32: return MTLVertexFormatUInt;
        case AttribFormat::VEC3:   return MTLVertexFormatFloat3;
        case AttribFormat::VEC4:   return MTLVertexFormatFloat4;
        case AttribFormat::CVEC4:  return MTLVertexFormatUChar4Normalized;
        default:                   return MTLVertexFormatFloat4;
    }
}

// Vertex buffers are placed at buffer slots 16..31 to avoid conflicts
// with uniform/storage buffers bound at their HLSL register(bN) indices 0..15.
static const uint32_t VERTEX_BUFFER_SLOT_OFFSET = 24;

// ---------------------------------------------------------------------------
// createGraphicsPipelineState
// ---------------------------------------------------------------------------
PipelineStatePointer MetalBackend::createGraphicsPipelineState(const GraphicsPipelineStateInit& init) {
    auto* pso = new MetalPipelineStateBackend();
    pso->initGraphics(init);
    pso->_primitiveType = toMTLPrimitive(init.primitiveTopology);

    // Resolve vertex and pixel MTLFunctions from the program
    auto* prog  = static_cast<MetalShaderBackend*>(init.program.get());
    id<MTLFunction> vertFn  = nil;
    id<MTLFunction> pixelFn = nil;

    if (prog->isProgram()) {
        auto& lib = prog->getProgramDesc().shaderLib;
        if (lib.count("VERTEX"))
            vertFn  = static_cast<MetalShaderBackend*>(lib.at("VERTEX").get())->_function;
        if (lib.count("PIXEL"))
            pixelFn = static_cast<MetalShaderBackend*>(lib.at("PIXEL").get())->_function;
    } else {
        if (prog->getShaderType() == ShaderType::VERTEX)       vertFn  = prog->_function;
        else if (prog->getShaderType() == ShaderType::PIXEL)   pixelFn = prog->_function;
    }

    MTLRenderPipelineDescriptor* desc = [[MTLRenderPipelineDescriptor alloc] init];
    desc.vertexFunction   = vertFn;
    desc.fragmentFunction = pixelFn;
    desc.rasterSampleCount = 1;

    // Color attachment
    MTLPixelFormat colorFmt = MetalBackend::Format[(uint32_t)init.colorTargetFormat];
    if (colorFmt == MTLPixelFormatInvalid) colorFmt = MTLPixelFormatBGRA8Unorm;
    desc.colorAttachments[0].pixelFormat = colorFmt;

    // Blend state
    if (init.blend.blendFunc.isEnabled()) {
        MTLRenderPipelineColorAttachmentDescriptor* ca = desc.colorAttachments[0];
        ca.blendingEnabled             = YES;
        ca.sourceRGBBlendFactor        = toMTLBlendFactor(init.blend.blendFunc.getSourceColor());
        ca.destinationRGBBlendFactor   = toMTLBlendFactor(init.blend.blendFunc.getDestinationColor());
        ca.rgbBlendOperation           = toMTLBlendOp(init.blend.blendFunc.getOperationColor());
        ca.sourceAlphaBlendFactor      = toMTLBlendFactor(init.blend.blendFunc.getSourceAlpha());
        ca.destinationAlphaBlendFactor = toMTLBlendFactor(init.blend.blendFunc.getDestinationAlpha());
        ca.alphaBlendOperation         = toMTLBlendOp(init.blend.blendFunc.getOperationAlpha());
    }

    // Color write mask
    MTLColorWriteMask wm = 0;
    if (init.blend.colorWriteMask & COLOR_MASK_RED)   wm |= MTLColorWriteMaskRed;
    if (init.blend.colorWriteMask & COLOR_MASK_GREEN)  wm |= MTLColorWriteMaskGreen;
    if (init.blend.colorWriteMask & COLOR_MASK_BLUE)   wm |= MTLColorWriteMaskBlue;
    if (init.blend.colorWriteMask & COLOR_MASK_ALPHA)  wm |= MTLColorWriteMaskAlpha;
    if (wm == 0) wm = MTLColorWriteMaskAll;
    desc.colorAttachments[0].writeMask = wm;

    // Depth format
    MTLPixelFormat depthFmt = MetalBackend::Format[(uint32_t)init.depthStencilFormat];
    if (depthFmt != MTLPixelFormatInvalid) {
        desc.depthAttachmentPixelFormat = depthFmt;
    }

    // Vertex descriptor (for [[stage_in]] attribute fetching)
    const auto& sl = init.streamLayout;
    if (sl.numAttribs() > 0) {
        MTLVertexDescriptor* vd = [[MTLVertexDescriptor alloc] init];
        for (uint8_t a = 0; a < sl.numAttribs(); ++a) {
            const Attrib* attr  = sl.getAttrib(a);
            uint32_t bufIdx = attr->_bufferIndex + VERTEX_BUFFER_SLOT_OFFSET;
            uint32_t offset = sl.evalBufferViewByteOffsetForAttribute(a);

            vd.attributes[a].bufferIndex = bufIdx;
            vd.attributes[a].format      = toMTLVertexFormat(attr->_format);
            vd.attributes[a].offset      = offset;
        }
        for (uint8_t b = 0; b < sl.numBuffers(); ++b) {
            uint16_t stride = sl.evalBufferViewByteStride(b);
            uint32_t slot   = b + VERTEX_BUFFER_SLOT_OFFSET;
            vd.layouts[slot].stride       = stride;
            vd.layouts[slot].stepFunction = MTLVertexStepFunctionPerVertex;
            vd.layouts[slot].stepRate     = 1;
        }
        desc.vertexDescriptor = vd;
    }

    NSError* error = nil;
    pso->_renderPipeline = [_device newRenderPipelineStateWithDescriptor:desc error:&error];
    if (!pso->_renderPipeline) {
        std::cerr << "[Metal] Graphics PSO error: "
                  << [[error localizedDescription] UTF8String] << "\n";
    }

    // Cache rasterizer state for use at bind time
    switch (init.rasterizer.cullMode) {
        case CullMode::BACK:  pso->_cullMode = MTLCullModeBack;  break;
        case CullMode::FRONT: pso->_cullMode = MTLCullModeFront; break;
        default:              pso->_cullMode = MTLCullModeNone;  break;
    }
    pso->_winding = init.rasterizer.frontFaceClockwise
                    ? MTLWindingClockwise : MTLWindingCounterClockwise;

    // Depth-stencil state
    if (init.depthStencil.depthTest.isEnabled()) {
        MTLDepthStencilDescriptor* dsd = [[MTLDepthStencilDescriptor alloc] init];
        dsd.depthCompareFunction = toMTLCompare(init.depthStencil.depthTest.getFunction());
        dsd.depthWriteEnabled    = init.depthStencil.depthTest.isWriteEnabled() ? YES : NO;
        pso->_depthStencilState  = [_device newDepthStencilStateWithDescriptor:dsd];
    }

    return PipelineStatePointer(pso);
}

// ---------------------------------------------------------------------------
// createComputePipelineState
// ---------------------------------------------------------------------------
PipelineStatePointer MetalBackend::createComputePipelineState(const ComputePipelineStateInit& init) {
    auto* pso = new MetalPipelineStateBackend();
    pso->initCompute(init);

    // Find the MTLFunction for the compute kernel
    auto* prog = static_cast<MetalShaderBackend*>(init.program.get());
    id<MTLFunction> fn = prog->_function;

    if (!fn && prog->isProgram()) {
        auto& lib = prog->getProgramDesc().shaderLib;
        if (lib.count("COMPUTE"))
            fn = static_cast<MetalShaderBackend*>(lib.at("COMPUTE").get())->_function;
        else if (!lib.empty())
            fn = static_cast<MetalShaderBackend*>(lib.begin()->second.get())->_function;
    }

    if (!fn) {
        std::cerr << "[Metal] No function for compute PSO\n";
        return PipelineStatePointer(pso);
    }

    NSError* error = nil;
    pso->_computePipeline = [_device newComputePipelineStateWithFunction:fn error:&error];
    if (!pso->_computePipeline) {
        std::cerr << "[Metal] Compute PSO error: "
                  << [[error localizedDescription] UTF8String] << "\n";
    }

    return PipelineStatePointer(pso);
}

#endif // __APPLE__
