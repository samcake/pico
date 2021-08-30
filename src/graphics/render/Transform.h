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

namespace graphics {
    class Transform {
    public:
        Transform() {}
        ~Transform() {}

        core::mat4x3 _matRTS;
    };

    class TransformTree;
    using TransformTreePtr = std::shared_ptr<TransformTree>;


    class VISUALIZATION_API TransformTree {
    public:
        using NodeID = core::IndexTable::Index;
        using NodeIDs = core::IndexTable::Indices;
        static const NodeID INVALID_ID = core::IndexTable::INVALID_INDEX;
        static const NodeID ROOT_ID = 0;

        struct Node {
            NodeID parent{ INVALID_ID };
            NodeID sybling{ INVALID_ID };
            NodeID children_head{ INVALID_ID };
            uint16_t num_children{ 0 };
            uint16_t refCount{ 0 };

            Node(NodeID p): parent(p) {}
            Node() {}
        };

        NodeID allocate(const core::mat4x3& rts, NodeID parent = 0);
        NodeIDs allocateBranch(NodeID rootParent, const std::vector<core::mat4x3>& transforms, const NodeIDs& parentsOffsets);
        void free(NodeID nodeId);
        
        void editTransform(NodeID nodeId, std::function<bool(core::mat4x3& rts)> editor);
        NodeIDs updateTransforms();
        void updateChildrenTransforms(NodeID parent, NodeIDs& touched);

        void attachNode(NodeID child, NodeID parent);
        void detachNode(NodeID child);


        int32_t reference(NodeID nodeId);
        int32_t release(NodeID nodeId);

        core::IndexTable _indexTable;
        std::vector<Node> _treeNodes;
        std::vector<core::mat4x3> _nodeTransforms;
        std::vector<core::mat4x3> _worldTransforms;

        std::vector<NodeID> _touchedTransforms;
    };

    using NodeID = TransformTree::NodeID;
    using NodeIDs = TransformTree::NodeIDs;
    const static NodeID INVALID_NODE_ID{ TransformTree::INVALID_ID };

    class VISUALIZATION_API NodeStore {
        public:

        TransformTree _tree;

        uint32_t  _num_buffers_elements{0};
        BufferPointer _nodes_buffer;
        BufferPointer _transforms_buffer;

        NodeStore() {}
        ~NodeStore() {}

        void resizeBuffers(const DevicePointer& device, uint32_t  numElements);

        NodeID createNode(const core::mat4x3& rts, NodeID parent);
        NodeIDs createNodeBranch(NodeID rootParent, const std::vector<core::mat4x3>& rts, const NodeIDs& parentsOffsets);
        void deleteNode(NodeID nodeId);
        void attachNode(NodeID child, NodeID parent);
        void detachNode(NodeID child);

        void editTransform(NodeID nodeId, std::function<bool(core::mat4x3& rts)> editor);
        void updateTransforms();

        int32_t reference(NodeID nodeId);
        int32_t release(NodeID nodeId);
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

