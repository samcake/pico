// Transform.h 
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

#include <functional>
#include <core/math/LinearAlgebra.h>
#include <core/stl/IndexTable.h>

#include "dllmain.h"

#include <gpu/gpu.h>
#include "gpu/StructuredBuffer.h"

namespace graphics {

    using Transform = core::mat4x3;

    using NodeID = core::IndexTable::Index;
    using NodeIDs = core::IndexTable::Indices;
    static const NodeID INVALID_NODE_ID = core::IndexTable::INVALID_INDEX;
    static const NodeID ROOT_ID = 0;

    class VISUALIZATION_API NodeStore {
    public:
        // Right after allocation, MUST call the reserve function allocate the memory chuncks
        void reserve(const DevicePointer& device, uint32_t  numElements);

        static const NodeID ROOT_ID = 0;

        struct NodeInfo {
            NodeID parent{ INVALID_NODE_ID };
            NodeID sybling{ INVALID_NODE_ID };
            NodeID children_head{ INVALID_NODE_ID };
            uint16_t num_children{ 0 };
            uint16_t refCount{ 0 };

            NodeInfo(NodeID p): parent(p) {}
            NodeInfo() {}
        };

        struct NodeTransform {
            Transform local;
            Transform world;
        };

        using NodeInfoStructBuffer = StructuredBuffer<NodeInfo>;
        using NodeTransformStructBuffer = StructuredBuffer<NodeTransform>;

    private:
        core::IndexTable _indexTable;
        mutable NodeInfoStructBuffer _nodeInfos;
        mutable NodeTransformStructBuffer _nodeTransforms;

        mutable std::vector<NodeID> _touchedInfos;
        mutable std::vector<NodeID> _touchedTransforms;

        using ReadInfoLock = std::pair< const NodeInfo*, std::lock_guard<std::mutex>>;
        using WriteInfoLock = std::pair< NodeInfo*, std::lock_guard<std::mutex>>;

        inline ReadInfoLock readInfo(NodeID id) const {
            return  _nodeInfos.read(id);
        }
        inline WriteInfoLock writeInfo(NodeID id) const {
            _touchedInfos.emplace_back(id); // take not of the write for this element
            return  _nodeInfos.write(id);
        }

        using ReadTransformLock = std::pair< const NodeTransform*, std::lock_guard<std::mutex>>;
        using WriteTransformLock = std::pair< NodeTransform*, std::lock_guard<std::mutex>>;

        inline ReadTransformLock readTransform(NodeID id) const {
            return  _nodeTransforms.read(id);
        }
        inline WriteTransformLock writeTransform(NodeID id) const {
            _touchedTransforms.emplace_back(id); // take not of the write for this element
            return  _nodeTransforms.write(id);
        }

    public:

        NodeID createNode(const Transform& local, NodeID parent);
        NodeIDs createNodeBranch(NodeID rootParent, const std::vector<Transform>& localTransforms, const NodeIDs& parentsOffsets);

        void free(NodeID nodeId);

        void attachNode(NodeID child, NodeID parent);
        void detachNode(NodeID child);

        int32_t reference(NodeID nodeId);
        int32_t release(NodeID nodeId);

        void editTransform(NodeID nodeId, std::function<bool(Transform& rts)> editor);
        NodeIDs updateTransforms();
        void updateChildrenTransforms(NodeID parent, NodeIDs& touched);

        Transform getWorldTransform(NodeID nodeId) const { return _nodeTransforms.unsafe_data(nodeId)->world; }
    
        inline BufferPointer getNodeInfoGPUBuffer() const { return _nodeInfos.gpu_buffer(); }
        inline BufferPointer getNodeTransformGPUBuffer() const { return _nodeTransforms.gpu_buffer(); }

        void syncGPUBuffer(const BatchPointer& batch);  
    };


    class VISUALIZATION_API Node {
        friend class Scene;
        Node(const NodeStore* transformTree, NodeID index) : _transformTree(transformTree), _index(index) { }

        mutable const NodeStore* _transformTree { nullptr };
        mutable NodeID _index { INVALID_NODE_ID };
    public:
        static const Node null;
        Node() {}

        Node(const Node& node) : _transformTree(node._transformTree), _index(node._index) {}
        Node& operator= (const Node& node) {
            _transformTree = node._transformTree;
            _index = node._index;
            return (*this);
        }

        const NodeStore* tree() const { return _transformTree; }
        NodeID id() const { return _index; }
    };


}

