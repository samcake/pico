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
        static const uint16_t INVALID_REFCOUNT_INFO = 0xFFFF;

        struct NodeInfo {
            NodeID parent{ INVALID_NODE_ID };
            NodeID sybling{ INVALID_NODE_ID };
            NodeID children_head{ INVALID_NODE_ID };
            uint16_t num_children{ 0 };
            uint16_t refCount{ INVALID_REFCOUNT_INFO };

            NodeInfo(NodeID p) : parent(p) {}
            NodeInfo() {}

            inline bool isValid() const { return refCount != INVALID_REFCOUNT_INFO; }
        };
        using NodeInfoStructBuffer = StructuredBuffer<NodeInfo>;
        using NodeInfos = NodeInfoStructBuffer::Array;

        struct NodeTransform {
            Transform local;
            Transform world;
        };
        using NodeTransformStructBuffer = StructuredBuffer<NodeTransform>;
        using NodeTransforms = NodeTransformStructBuffer::Array;

    private:
        core::IndexTable _indexTable;
        mutable NodeInfoStructBuffer _nodeInfos;
        mutable NodeTransformStructBuffer _nodeTransforms;

        mutable NodeIDs _touchedInfos;
        mutable NodeIDs _touchedTransforms;

        using ReadInfoLock = std::pair< const NodeInfo*, std::lock_guard<std::mutex>>;
        using WriteInfoLock = std::pair< NodeInfo*, std::lock_guard<std::mutex>>;

        inline void touchInfo(NodeID id) const { _touchedInfos.emplace_back(id); }
        inline ReadInfoLock readInfo(NodeID id) const {
            return  _nodeInfos.read(id);
        }
        inline WriteInfoLock writeInfo(NodeID id) const {
            touchInfo(id); // take note of the write for this element
            return  _nodeInfos.write(id);
        }

        using ReadTransformLock = std::pair< const NodeTransform*, std::lock_guard<std::mutex>>;
        using WriteTransformLock = std::pair< NodeTransform*, std::lock_guard<std::mutex>>;

        inline ReadTransformLock readTransform(NodeID id) const {
            return  _nodeTransforms.read(id);
        }
        inline WriteTransformLock writeTransform(NodeID id) const {
            _touchedTransforms.emplace_back(id); // take note of the write for this element
            return  _nodeTransforms.write(id);
        }

        struct Handle {
            NodeStore* _store = nullptr;
            NodeID     _id = INVALID_NODE_ID;

            inline bool isValidHandle() const {
                return (_store != nullptr) && (_id != INVALID_NODE_ID);
            }
        };

    public:


        // Node struct is  just an interface on a particular NodeInfo stored in the Node Store at NodeID.
        // Node can be copied by value
        struct Node {
        public:
            static Node null;

            Node() {} // null item
            Node(const Node& src) = default;
            Node& Node::operator= (const Node&) = default;

            inline bool isValid() const {
                if (_self.isValidHandle())
                    return true; //  store()->isValid(id());
                return false;
            }
            inline NodeID   id() const { return  _self._id; }
            inline const NodeStore* store() const { return _self._store; }
            inline NodeStore* store() { return _self._store; }

            inline NodeInfo info() const { return store()->getNodeInfo(id()); }
            inline NodeTransform transform() const { return store()->getNodeTransform(id()); }


        private:
            Handle _self;

            friend NodeStore;
            Node(Handle& h) : _self(h) {}
        };
        inline Node makeNode(NodeID id) { return { Handle{ this, id } }; }

        NodeID createNode(const Transform& local, NodeID parent);
        NodeIDs createNodeBranch(NodeID rootParent, const std::vector<Transform>& localTransforms, const NodeIDs& parentsOffsets);
        void free(NodeID nodeId);

        int32_t reference(NodeID nodeId);
        int32_t release(NodeID nodeId);

        inline auto numValidNodes() const { return _indexTable.getNumValidElements(); }
        inline auto numAllocatedNodes() const { return _indexTable.getNumAllocatedElements(); }

        inline Node getNode(NodeID id) const { return (isValid(id) ? Node(Handle{ const_cast<NodeStore*>(this), id }) : Node::null); }
        inline Node getUnsafeNode(NodeID id) const { return Node(Handle{ const_cast<NodeStore*>(this), id }); } // We do not check that the nodeID is valid here, so could get a fake valid node

        NodeIDs fetchValidNodes() const;
        NodeInfos fetchNodeInfos() const; // Collect all the NodeInfos in an array, there could be INVALID itemInfos
        NodeTransforms fetchNodeTransforms() const; // Collect all the NodeTransforms in an array, there could be INVALID itemInfos

        // Connect the nodes
        void attachNode(NodeID child, NodeID parent);
        void detachNode(NodeID child);

        // Node Interface
        inline bool isValid(NodeID id) const { return getNodeInfo(id).isValid(); }
     
        inline NodeInfo getNodeInfo(NodeID id) const {
            auto [i, l] = readInfo(id);
            return *i;
        }

        inline NodeTransform getNodeTransform(NodeID id) const {
            auto [t, l] = readTransform(id);
            return *t;
        }

        inline void editNodeTransform(NodeID id, std::function<bool(Transform& rts)> editor) {
            auto [t, l] = writeTransform(id);
            if (!editor(t->local)) { _touchedTransforms.pop_back(); }
        }

        // Update and Manage the transform tree once per loop
        NodeIDs updateTransforms();
        void updateChildrenTransforms(NodeID parent, NodeIDs& touched);

    
    public:
        // gpu api
        inline BufferPointer getNodeInfoGPUBuffer() const { return _nodeInfos.gpu_buffer(); }
        inline BufferPointer getNodeTransformGPUBuffer() const { return _nodeTransforms.gpu_buffer(); }
        void syncGPUBuffer(const BatchPointer& batch);  
    };

    using Node = NodeStore::Node;
    using NodeInfo = NodeStore::NodeInfo;
    using NodeInfos = NodeStore::NodeInfos;
    using NodeTransform = NodeStore::NodeTransform;
    using NodeTransforms = NodeStore::NodeTransforms;
}

