// StreamLayout.h 
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

#include "pico.h"
#include "gpu.h"

#include <array>

namespace pico {

    struct Attrib {

        static uint8_t sizeOf(AttribFormat format) {
            const uint8_t formatSizes[int(AttribFormat::COUNT)] = { 4, 12, 16, 4 };
            return formatSizes[(int)format];
        }

        static const uint8_t AUTO_PACKING_OFFSET{ 0xFF };

        AttribSemantic _semantic{ AttribSemantic::A };
        AttribFormat _format{ AttribFormat::UINT32 };
        uint8_t _bufferIndex{ 0 };
        uint8_t _packingOffset{ AUTO_PACKING_OFFSET };

        Attrib() {}
        Attrib(AttribSemantic sem) : _semantic(sem) {}
        Attrib(AttribSemantic sem, AttribFormat fmt) : _semantic(sem), _format(fmt) {}
        Attrib(AttribSemantic sem, AttribFormat fmt, uint8_t bi) : _semantic(sem),  _format(fmt), _bufferIndex(bi) {}
        Attrib(AttribSemantic sem, AttribFormat fmt, uint8_t bi, uint8_t po) : _semantic(sem), _format(fmt), _bufferIndex(bi), _packingOffset(po) {}
    };

    template <int A> using Attribs = std::array<Attrib, A>;
    using Attribs_0 = Attribs<0>;

    struct AttribBufferView {
        static const uint32_t AUTO_BYTE_LENGTH{ 0xFFFFFFFF };
        static const uint16_t AUTO_BYTE_STRIDE{ 0xFFFF };

        uint32_t _byteOffset{ 0 };
        uint32_t _byteLength{ AUTO_BYTE_LENGTH };
        uint16_t _byteStride{ AUTO_BYTE_STRIDE };

        uint32_t evalViewByteLength(uint32_t concreteBufferSize) const {
            return (_byteLength == AUTO_BYTE_LENGTH ? concreteBufferSize - _byteOffset : _byteLength);
        }

        uint16_t evalViewByteStride(uint16_t concreteStreamAttribSize) const {
            picoAssert(_byteStride >= concreteStreamAttribSize);
            return (_byteStride == AUTO_BYTE_STRIDE ? concreteStreamAttribSize : _byteStride);
        }

        uint16_t evalViewByteoffset(uint16_t concreteStreamAttribOffset) const {
           return _byteOffset + concreteStreamAttribOffset;
        }

        AttribBufferView() {}
        AttribBufferView(uint32_t offset) : _byteOffset(offset) {}
        AttribBufferView(uint32_t offset, uint32_t length) : _byteOffset(offset), _byteLength(length) {}
        AttribBufferView(uint32_t offset, uint32_t length, uint16_t stride) : _byteOffset(offset), _byteLength(length), _byteStride(stride) {}
    };

    template <int B> using AttribBufferViews = std::array<AttribBufferView, B>;
    using AttribBufferViews_0 = AttribBufferViews<0>;

    class StreamLayout {
    public:
        static const uint8_t INVALID_ATTRIB_INDEX{ 0xFF };

    protected:
        class Base {
        public:
            virtual ~Base() {};

            virtual uint8_t numAttribs() const = 0;
            virtual uint8_t numBuffers() const = 0;

            virtual const Attrib* getAttrib(uint8_t a) const = 0;
            virtual const AttribBufferView* getBufferView(uint8_t b) const = 0;


            uint8_t attribSize(uint8_t a) const { return Attrib::sizeOf(getAttrib(a)->_format); }
            uint16_t attribOffset(uint8_t a) const {
                if (a <= 0 ) {
                    return  0;
                }
                auto attrib = getAttrib(a);
                auto attribBufferIndex = attrib->_bufferIndex;
                uint16_t streamOffset = 0;
                for (int i = (a - 1); i >= 0; i--) {
                    attrib--;
                    streamOffset += (attrib->_bufferIndex == attribBufferIndex ? Attrib::sizeOf(attrib->_format) : 0);
                }
                return streamOffset;
            }


            uint16_t streamAttribSize(uint8_t b) const {
                uint16_t streamSize = 0;
                auto attrib = getAttrib(0);
                for (uint8_t a = 0; a < numAttribs(); a++) {
                    // TODO: this code assume the attrib packing offset is always "AUTO"
                    streamSize += (attrib->_bufferIndex == b ? Attrib::sizeOf(attrib->_format) : 0);
                    attrib++;
                }
                return streamSize;
            }

            uint32_t evalBufferViewByteLength(uint8_t b, uint32_t concreteBufferSize) const { return getBufferView(b)->evalViewByteLength(concreteBufferSize); }

            uint16_t evalBufferViewByteStride(uint8_t b) const { return getBufferView(b)->evalViewByteStride(streamAttribSize(b)); }

            uint8_t findAttribAt(AttribSemantic semantic) const {
                auto attrib = getAttrib(0);
                for (uint8_t a = 0; a < numAttribs(); a++) {
                    if (attrib->_semantic == semantic) {
                        return a;
                    }
                    attrib++;
                }
                return INVALID_ATTRIB_INDEX;
            }
        };


        template <int A, int B> class Instanced : public Base {
        public:
            Instanced() {}
            Instanced(Attribs<A> a, AttribBufferViews<B> b) : _attribs(a), _bufferViews(b) {}
            virtual ~Instanced() {}

            constexpr uint8_t numAttribs() const override { return A; }
            constexpr uint8_t numBuffers() const override { return B; }
            const Attrib* getAttrib(uint8_t a) const override { return _attribs.data() + a; }
            const AttribBufferView* getBufferView(uint8_t b) const override { return _bufferViews.data() + b; }

            Attribs<A> _attribs;
            AttribBufferViews<B> _bufferViews;
        };

        // Private constructor from the row base pointer used by the build function
        StreamLayout(Base* base) : _base(base) {}

    public:
        StreamLayout() : _base(new Instanced<0, 0>(Attribs_0(), AttribBufferViews_0())) {} // Simple constructor build an ampty stream layout

        std::shared_ptr<Base> _base{ nullptr };

        uint8_t numAttribs() const { return _base->numAttribs(); }
        uint8_t numBuffers() const { return _base->numBuffers(); }

        const Attrib* getAttrib(uint8_t a) const { return _base->getAttrib(a); }
        const AttribBufferView* getBufferView(uint8_t b) const { return _base->getBufferView(b); }

        uint8_t attribSize(uint8_t a) const { return _base->attribSize(a); }
        uint16_t streamAttribSize(uint8_t b) const { return _base->streamAttribSize(b); }
        uint32_t evalBufferViewByteLength(uint8_t b, uint32_t concreteBufferSize) const { return _base->evalBufferViewByteLength(b, concreteBufferSize); }
        uint16_t evalBufferViewByteStride(uint8_t b) const { return _base->evalBufferViewByteStride(b); }

        uint16_t evalBufferViewByteOffsetForAttribute(uint8_t a) const { return _base->attribOffset(a) + getBufferView(getAttrib(a)->_bufferIndex)->_byteOffset; }

        uint8_t findAttribAt(AttribSemantic semantic) const { return _base->findAttribAt(semantic); }

        template <int A, int B>
        static bool check(const Attribs<A>& attribs, const AttribBufferViews<B> bufferViews) {
            // TODO: implement a bunch of checks to make sure the attribs and stream work together
            return true;
        }

        template <int A, int B>
        static StreamLayout build(const Attribs<A>& attribs, const AttribBufferViews<B> bufferViews) {
            picoAssert(check(attribs, bufferViews));
            return StreamLayout(new Instanced<A, B>(attribs, bufferViews));
        }
    };
}

