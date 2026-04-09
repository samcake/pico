// MetalBackend_Descriptor.mm
// DescriptorSet creation and update for pico Metal backend
//
// Sam Gateau / pico - 2024
// MIT License
#ifdef __APPLE__

#include "MetalBackend.h"

using namespace graphics;

// ---------------------------------------------------------------------------
// createDescriptorSet
// ---------------------------------------------------------------------------
DescriptorSetPointer MetalBackend::createDescriptorSet(const DescriptorSetInit& init) {
    auto* ds  = new MetalDescriptorSetBackend();
    ds->_init = init;

    // Pre-size object storage
    int32_t total = 0;
    for (auto& l : init._descriptorSetLayout) total += (int32_t)l._count;
    if (init._bindSamplers && init._rootLayout) {
        for (auto& l : init._rootLayout->_init._samplerLayout) total += (int32_t)l._count;
    }
    ds->_numDescriptors = total;
    ds->_objects.resize((size_t)total);

    return DescriptorSetPointer(ds);
}

// ---------------------------------------------------------------------------
// updateDescriptorSet — populate MetalBinding list from DescriptorObjects
// ---------------------------------------------------------------------------
void MetalBackend::updateDescriptorSet(DescriptorSetPointer& descriptorSet,
                                        DescriptorObjects&    objects) {
    auto* ds = static_cast<MetalDescriptorSetBackend*>(descriptorSet.get());
    ds->_objects  = objects;
    ds->_bindings.clear();

    uint32_t objIdx = 0;

    // Resolve the descriptor set layout: use the one from the root layout if available
    DescriptorSetLayout setLayout = ds->_init._descriptorSetLayout;
    if (setLayout.empty() && ds->_init._rootLayout && ds->_init._bindSetSlot >= 0) {
        auto& setLayouts = ds->_init._rootLayout->_init._setLayouts;
        if (ds->_init._bindSetSlot < (int32_t)setLayouts.size()) {
            setLayout = setLayouts[ds->_init._bindSetSlot];
        }
    }

    // Main descriptor set layout entries
    for (auto& l : setLayout) {
        for (uint32_t c = 0; c < l._count; ++c, ++objIdx) {
            if (objIdx >= (uint32_t)objects.size()) break;
            const auto& obj = objects[objIdx];
            if (obj._type == DescriptorType::UNDEFINED) continue;

            MetalBinding b;
            b.slot  = l._binding + c;
            b.stage = l._shaderStage;

            switch (obj._type) {
                case DescriptorType::UNIFORM_BUFFER:
                case DescriptorType::RESOURCE_BUFFER:
                case DescriptorType::RW_RESOURCE_BUFFER:
                    if (obj._buffer) {
                        b.kind   = MetalBinding::Kind::Buffer;
                        b.buffer = static_cast<MetalBufferBackend*>(obj._buffer.get())->_buffer;
                        ds->_bindings.push_back(b);
                    }
                    break;
                case DescriptorType::RESOURCE_TEXTURE:
                case DescriptorType::RW_RESOURCE_TEXTURE:
                    if (obj._texture) {
                        b.kind    = MetalBinding::Kind::Texture;
                        b.texture = static_cast<MetalTextureBackend*>(obj._texture.get())->_texture;
                        ds->_bindings.push_back(b);
                    }
                    break;
                case DescriptorType::SAMPLER:
                    if (obj._sampler) {
                        b.kind    = MetalBinding::Kind::Sampler;
                        b.sampler = static_cast<MetalSamplerBackend*>(obj._sampler.get())->_sampler;
                        ds->_bindings.push_back(b);
                    }
                    break;
                default:
                    break;
            }
        }
    }

    // Sampler layout (when _bindSamplers = true)
    if (ds->_init._bindSamplers && ds->_init._rootLayout) {
        for (auto& l : ds->_init._rootLayout->_init._samplerLayout) {
            for (uint32_t c = 0; c < l._count; ++c, ++objIdx) {
                if (objIdx >= (uint32_t)objects.size()) break;
                const auto& obj = objects[objIdx];
                if (obj._type == DescriptorType::SAMPLER && obj._sampler) {
                    MetalBinding b;
                    b.kind    = MetalBinding::Kind::Sampler;
                    b.slot    = l._binding + c;
                    b.stage   = l._shaderStage;
                    b.sampler = static_cast<MetalSamplerBackend*>(obj._sampler.get())->_sampler;
                    ds->_bindings.push_back(b);
                }
            }
        }
    }
}

#endif // __APPLE__
