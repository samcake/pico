// MetalBackend_Resource.mm
// Buffer, Texture, and Sampler creation for pico Metal backend
//
// Sam Gateau / pico - 2024
// MIT License
#ifdef __APPLE__

#include "MetalBackend.h"
#include <iostream>

using namespace graphics;

// ---------------------------------------------------------------------------
// Buffer
// ---------------------------------------------------------------------------
BufferPointer MetalBackend::_createBuffer(const BufferInit& init, const std::string& name) {
    auto* buf   = new MetalBufferBackend();
    buf->_init  = init;
    buf->setBufferSize(init.bufferSize);
    buf->setName(name);

    NSUInteger len = (NSUInteger)(init.bufferSize > 0 ? init.bufferSize : 4);

    MTLResourceOptions opts;
    if (init.hostVisible) {
        // Shared: both CPU and GPU can access directly
        opts = MTLResourceStorageModeShared | MTLResourceCPUCacheModeDefaultCache;
    } else {
        // Private: GPU only, uploaded via staging
        opts = MTLResourceStorageModePrivate;
    }

    buf->_buffer = [_device newBufferWithLength:len options:opts];
    if (!buf->_buffer) {
        std::cerr << "[Metal] Failed to allocate buffer of size " << len << "\n";
    }

    if (init.hostVisible && buf->_buffer) {
        buf->_cpuMappedAddress = buf->_buffer.contents;
        buf->setNeedUpload(false);  // already visible to GPU
    }

    if (!name.empty() && buf->_buffer) {
        buf->_buffer.label = [NSString stringWithUTF8String:name.c_str()];
    }

    return BufferPointer(buf);
}

// ---------------------------------------------------------------------------
// Texture
// ---------------------------------------------------------------------------
TexturePointer MetalBackend::createTexture(const TextureInit& init) {
    auto* tex   = new MetalTextureBackend();
    tex->_init  = init;

    MTLTextureDescriptor* desc = [[MTLTextureDescriptor alloc] init];
    desc.width          = init.width  > 0 ? init.width  : 1;
    desc.height         = init.height > 0 ? init.height : 1;
    desc.mipmapLevelCount = 1;

    MTLPixelFormat mtlFmt = MetalBackend::Format[(uint32_t)init.format];
    desc.pixelFormat    = (mtlFmt != MTLPixelFormatInvalid) ? mtlFmt : MTLPixelFormatRGBA8Unorm;

    if (init.numSlices > 0) {
        desc.textureType = MTLTextureType2DArray;
        desc.arrayLength = init.numSlices;
    } else {
        desc.textureType = MTLTextureType2D;
        desc.arrayLength = 1;
    }

    // Determine usage flags
    MTLTextureUsage usage = 0;
    if (init.usage & ResourceUsage::RESOURCE_TEXTURE)     usage |= MTLTextureUsageShaderRead;
    if (init.usage & ResourceUsage::RW_RESOURCE_TEXTURE)  usage |= MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
    if (init.usage & ResourceUsage::RENDER_TARGET)        usage |= MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    if (usage == 0) usage = MTLTextureUsageShaderRead;
    desc.usage = usage;

    // Storage mode
    bool gpuOnly = (init.usage & (ResourceUsage::RENDER_TARGET | ResourceUsage::RW_RESOURCE_TEXTURE)) != 0
                    && !init.initData.empty() == false;
    desc.storageMode = gpuOnly ? MTLStorageModePrivate : MTLStorageModeShared;

    tex->_texture = [_device newTextureWithDescriptor:desc];
    if (!tex->_texture) {
        std::cerr << "[Metal] Failed to create texture " << init.width << "x" << init.height << "\n";
    }

    // Upload initData if provided (Shared storage allows direct CPU upload)
    if (!init.initData.empty() && tex->_texture) {
        NSUInteger bpp = 4; // assumes RGBA8
        NSUInteger w = desc.width;
        NSUInteger h = desc.height;
        for (NSUInteger slice = 0; slice < init.initData.size(); ++slice) {
            const auto& pixels = init.initData[slice];
            if (pixels.empty()) continue;
            MTLRegion region = MTLRegionMake2D(0, 0, w, h);
            [tex->_texture replaceRegion:region
                             mipmapLevel:0
                                   slice:slice
                               withBytes:pixels.data()
                             bytesPerRow:w * bpp
                           bytesPerImage:w * h * bpp];
        }
        tex->notifyUploaded();
    }

    return TexturePointer(tex);
}

// ---------------------------------------------------------------------------
// Sampler
// ---------------------------------------------------------------------------
SamplerPointer MetalBackend::createSampler(const SamplerInit& init) {
    auto* samp  = new MetalSamplerBackend();
    samp->_state = init;

    MTLSamplerDescriptor* desc = [[MTLSamplerDescriptor alloc] init];

    bool useLin = (init._filter == Filter::MIN_MAG_MIP_LINEAR  ||
                   init._filter == Filter::MIN_MAG_LINEAR_MIP_POINT ||
                   init._filter == Filter::MIN_LINEAR_MAG_MIP_POINT ||
                   init._filter == Filter::MIN_POINT_MAG_MIP_LINEAR ||
                   init._filter == Filter::MIN_POINT_MAG_LINEAR_MIP_POINT);
    bool useMipLin = (init._filter == Filter::MIN_MAG_MIP_LINEAR ||
                      init._filter == Filter::MIN_MAG_POINT_MIP_LINEAR ||
                      init._filter == Filter::MIN_POINT_MAG_MIP_LINEAR);

    desc.minFilter  = useLin    ? MTLSamplerMinMagFilterLinear  : MTLSamplerMinMagFilterNearest;
    desc.magFilter  = useLin    ? MTLSamplerMinMagFilterLinear  : MTLSamplerMinMagFilterNearest;
    desc.mipFilter  = useMipLin ? MTLSamplerMipFilterLinear     : MTLSamplerMipFilterNearest;
    if (init._filter == Filter::ANISOTROPIC) {
        desc.maxAnisotropy = 16;
        desc.minFilter     = MTLSamplerMinMagFilterLinear;
        desc.magFilter     = MTLSamplerMinMagFilterLinear;
        desc.mipFilter     = MTLSamplerMipFilterLinear;
    }

    auto cvtAddr = [](AddressMode m) -> MTLSamplerAddressMode {
        switch (m) {
            case AddressMode::WRAP:        return MTLSamplerAddressModeRepeat;
            case AddressMode::MIRROR:      return MTLSamplerAddressModeMirrorRepeat;
            case AddressMode::CLAMP:       return MTLSamplerAddressModeClampToEdge;
            case AddressMode::BORDER:      return MTLSamplerAddressModeClampToBorderColor;
            case AddressMode::MIRROR_ONCE: return MTLSamplerAddressModeMirrorClampToEdge;
            default:                       return MTLSamplerAddressModeRepeat;
        }
    };
    desc.sAddressMode = cvtAddr(init._addressU);
    desc.tAddressMode = cvtAddr(init._addressV);
    desc.rAddressMode = cvtAddr(init._addressW);

    samp->_sampler = [_device newSamplerStateWithDescriptor:desc];
    return SamplerPointer(samp);
}

#endif // __APPLE__
