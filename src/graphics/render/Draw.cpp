// Draw.cpp
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
#include "Draw.h"
#include "gpu/Resource.h"
#include "gpu/Device.h"

#include <algorithm>

namespace graphics {

    Draw Draw::null;

    void DrawStore::reserve(const DevicePointer& device, uint32_t capacity) {
        _drawInfos.reserve(device, capacity);
    }

    DrawID DrawStore::allocate(DrawObjectCallback drawcall, const DrawBound& bound, const Draw& draw) {
        auto [new_id, recycle] = _indexTable.allocate();

        //auto bound = draw.getBound();

        DrawInfo info = { bound };
        _drawInfos.allocate_element(new_id, &info);
        _touchedElements.push_back(new_id);

        if (recycle) {
            _drawcalls[new_id] = drawcall;
            _drawConcepts[new_id] = draw._self;
        }
        else {
            _drawcalls.emplace_back(drawcall);
            _drawConcepts.emplace_back(draw._self);
        }

        return new_id;
    }


    DrawID DrawStore::createDraw(DrawObjectCallback drawcall, const DrawBound& bound, const Draw& draw) {
        return allocate(drawcall, bound, draw);
    }

    void DrawStore::free(DrawID id) {
        if (_indexTable.isValid(id)) {
            _indexTable.free(id);
            _drawInfos.set_element(id, nullptr);
            _touchedElements.push_back(id);
            _drawcalls[id] = nullptr;
            _drawConcepts[id].reset();
        }
    }

    DrawIDs DrawStore::fetchValidDraws() const {
        // lock the store info array as a whole
        // so we can use the unsafe data accessor in the search loop
        auto [begin_info, l] = _drawInfos.read(0);

        // Pre allocate the result with the expected size
        DrawIDs validDraws(numValidDraws(), INVALID_DRAW_ID);

        // Collect all the valid draw ids
        auto drawCount = numAllocatedDraws();
        for (DrawID i = 0; i < drawCount; ++i) {
            if ((begin_info + i)->isValid())
                validDraws.emplace_back(i);
        }

        // done
        return validDraws;
    }

    DrawInfos DrawStore::fetchDrawInfos() const {
        // lock the item store info array as a whole
        // so we can use the unsafe data accessor in the search loop
        auto [begin_info, l] = _drawInfos.read(0);

        auto drawCount = numAllocatedDraws();

        // Pre allocate the result with the expected size
        DrawInfos drawInfos(drawCount, DrawInfo());

        // copy
        memcpy(drawInfos.data(), begin_info, drawCount * sizeof(DrawInfo));

        // done
        return drawInfos;
    }

    int32_t DrawStore::reference(DrawID id) {
        if (_indexTable.isValid(id)) {
            auto& the_node = *_drawInfos.unsafe_data(id);
            the_node._refCount++;
            _touchedElements.push_back(id);
            return the_node._refCount;
        } else {
            return 0;
        }
    }
    int32_t DrawStore::release(DrawID id) {
        if (_indexTable.isValid(id)) {
            auto& the_node = *_drawInfos.unsafe_data(id);
            the_node._refCount--;
            _touchedElements.push_back(id);
            return the_node._refCount;
        } else {
            return 0;
        }
    }

    void DrawStore::syncGPUBuffer(const BatchPointer& batch) {
        // Clean the touched elements
        std::sort(_touchedElements.begin(), _touchedElements.end());
        auto newEnd = std::unique(_touchedElements.begin(), _touchedElements.end());
        if (newEnd != _touchedElements.end()) _touchedElements.erase(newEnd);

        // Sync the gpu buffer version
        _drawInfos.sync_gpu_from_cpu(batch, _touchedElements);

        // Start fresh
        _touchedElements.clear();
    }
}

