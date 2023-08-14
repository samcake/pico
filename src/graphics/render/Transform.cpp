// Transform.cpp
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
#include "Transform.h"
#include <core/log.h>
#include "gpu/Resource.h"
#include "gpu/Device.h"

using namespace core;
namespace graphics {

    Node Node::null;


    void NodeStore::reserve(const DevicePointer& device, uint32_t capacity) {
        _nodeInfos.reserve(device, capacity);
        _nodeTransforms.reserve(device, capacity);
        _nodeNames.reserve(capacity);
    }


    NodeID NodeStore::createNode(const NodeInit& init) {
        auto [new_id, recycle] = _indexTable.allocate();
        _nodeNames.resize(_indexTable.getNumAllocatedElements());

        NodeInfo info;
        info.refCount = 0; // Turn the node valid
        _nodeInfos.allocate_element(new_id, &info);
        _touchedInfos.push_back(new_id);

        NodeTransform transform = { init.localTransform, init.localTransform };
        _nodeTransforms.allocate_element(new_id, &transform);
        _touchedTransforms.push_back(new_id);

        _nodeNames[new_id] = init.name;

        attachNode(new_id, init.parent);

        return new_id;
    }

    NodeIDs NodeStore::createNodeBranch(const NodeBranchInit& init) {
        auto [new_node_ids, numRecycled] = _indexTable.allocate(init.localTransforms.size());
        _nodeNames.resize(_indexTable.getNumAllocatedElements());

        // first pass 
        uint32_t i = 0;
        for (auto new_id : new_node_ids) {

            NodeInfo info;
            _nodeInfos.allocate_element(new_id, &info);
            _touchedInfos.push_back(new_id);

            NodeTransform transform = { init.localTransforms[i], init.localTransforms[i] };
            _nodeTransforms.allocate_element(new_id, &transform);
            _touchedTransforms.push_back(new_id);

            _nodeNames[new_id] = init.names[i];

            ++i;
        }

        // 2nd pass needs to happen after 1st pass allocation
        i = 0;
        for (auto new_node_id : new_node_ids) {
            auto o = init.parentOffsets[i];
            attachNode(new_node_id, (o == INVALID_NODE_ID ? init.rootParent : new_node_ids[o]));
            i++;
        }

        return new_node_ids;
    }

    void NodeStore::free(NodeID nodeId) {
        if (_indexTable.isValid(nodeId)) {
            _indexTable.free(nodeId);

            _nodeInfos.set_element(nodeId, nullptr);
            _touchedInfos.push_back(nodeId);

            _nodeTransforms.set_element(nodeId, nullptr);
            _touchedTransforms.push_back(nodeId);

            _nodeNames[nodeId].clear();
        }
    }

    NodeIDs NodeStore::fetchValidNodes() const {
        // lock the node store info array as a whole
        // so we can use the unsafe data accessor in the search loop
        auto [begin_info, l] = _nodeInfos.read(0);

        // Pre allocate the result with the expected size
        NodeIDs validNodes(numValidNodes(), INVALID_NODE_ID);

        // Collect all the valid item ids
        auto nodeCount = numAllocatedNodes();
        for (NodeID i = 0; i < nodeCount; ++i) {
            if ((begin_info + i)->isValid())
                validNodes.emplace_back(i);
        }

        // done
        return validNodes;
    }
    NodeInfos NodeStore::fetchNodeInfos() const {
        // lock the node store info array as a whole
        // so we can use the unsafe data accessor in the search loop
        auto [begin_node, l] = _nodeInfos.read(0);

        auto nodeCount = numAllocatedNodes();

        // Pre allocate the result with the expected size
        NodeInfos nodeInfos(nodeCount, NodeInfo());

        // copy
        memcpy(nodeInfos.data(), begin_node, nodeCount * sizeof(NodeInfo));

        // done
        return nodeInfos;
    }

    NodeTransforms NodeStore::fetchNodeTransforms() const {
        // lock the node store transform array as a whole
        // so we can use the unsafe data accessor in the search loop
        auto [begin_node, l] = _nodeTransforms.read(0);

        auto nodeCount = numAllocatedNodes();

        // Pre allocate the result with the expected size
        NodeTransforms nodeTransforms(nodeCount, NodeTransform());

        // copy
        memcpy(nodeTransforms.data(), begin_node, nodeCount * sizeof(NodeTransform));

        // done
        return nodeTransforms;
    }


    void NodeStore::attachNode(NodeID node_id, NodeID parent_id) {
        picoAssert(_indexTable.isValid(node_id));

        // Make sure the node is detached first
        detachNode(node_id);

        // Write access and lock the nodeInfos
        auto [the_node, l] = _nodeInfos.write(node_id);

        the_node->parent = parent_id;

        if (parent_id != INVALID_NODE_ID) {
            auto& parent_node = *_nodeInfos.unsafe_data(parent_id);
            touchInfo(parent_id);

            the_node->sybling = parent_node.children_head;

            parent_node.children_head = node_id;
            parent_node.num_children++;
        }

        _touchedTransforms.push_back(node_id);

        // invalid:
        // node world transform and world sphere
        // children world transform and world sphere
    }

    void NodeStore::detachNode(NodeID node_id) {
        auto [the_node, l] = writeInfo(node_id); // write or not, but lock
        if (the_node->parent == INVALID_NODE_ID) {
            return;
        }
        auto& parent_node = *_nodeInfos.unsafe_data(the_node->parent);
        touchInfo(the_node->parent);

        auto left_sybling_id = parent_node.children_head;
        if (left_sybling_id == node_id) {
            parent_node.children_head = the_node->sybling;
            parent_node.num_children--;
        } else {

            for (int i = 0; i < parent_node.num_children; i++) {
                auto& sybling_node = *_nodeInfos.unsafe_data(left_sybling_id);
                if (sybling_node.sybling == node_id) {
                    sybling_node.sybling = the_node->sybling;
                    touchInfo(left_sybling_id);
                    parent_node.num_children--;
                    break;
                }

                left_sybling_id = sybling_node.sybling;
                picoAssert(left_sybling_id != INVALID_NODE_ID);
            }

            // Should never happen
            picoAssert(left_sybling_id != INVALID_NODE_ID);
        }


        the_node->parent = INVALID_NODE_ID;
        the_node->sybling = INVALID_NODE_ID;
    }

    int32_t NodeStore::reference(NodeID nodeId) {
        if (_indexTable.isValid(nodeId)) {
            auto& the_node = *_nodeInfos.unsafe_data(nodeId);
            the_node.refCount++;
            _touchedInfos.push_back(nodeId);
            return the_node.refCount;
        } else {
            return 0;
        }
    }
    int32_t NodeStore::release(NodeID nodeId) {
        if (_indexTable.isValid(nodeId)) {
            auto& the_node = *_nodeInfos.unsafe_data(nodeId);
            the_node.refCount--;
            _touchedInfos.push_back(nodeId);
            return the_node.refCount;
        } else {
            return 0;
        }
    }

    NodeIDs NodeStore::updateTransforms() {
        NodeIDs touched;
        for (auto nodeId : _touchedTransforms) {
            const auto& info = *_nodeInfos.unsafe_data(nodeId);
            if (info.parent == INVALID_NODE_ID) {
                _nodeTransforms.unsafe_data(nodeId)->world = _nodeTransforms.unsafe_data(nodeId)->local;
            } else {
                _nodeTransforms.unsafe_data(nodeId)->world = core::mul(_nodeTransforms.unsafe_data(info.parent)->world, _nodeTransforms.unsafe_data(nodeId)->local);
            }
            _nodeTransforms.write(nodeId);
            updateChildrenTransforms(nodeId, touched);
        }

        _touchedTransforms.clear();

        return touched;
    }


    void NodeStore::updateChildrenTransforms(NodeID parentId, NodeIDs& touched) {

        const auto& info = *_nodeInfos.unsafe_data(parentId);
        _nodeTransforms.write(parentId);

        if (info.num_children == 0) {
            return;
        }
        const auto& parent_world_transform = _nodeTransforms.unsafe_data(parentId)->world;

        NodeIDs children;
        children.reserve(info.num_children);

        NodeID child_id = info.children_head;
        for (int i = 0; i < info.num_children; ++i) {
            _nodeTransforms.unsafe_data(child_id)->world = core::mul(parent_world_transform, _nodeTransforms.unsafe_data(child_id)->local);
            touched.push_back(child_id);

            const auto& child = *_nodeInfos.unsafe_data(child_id);
            if (child.num_children > 0) {
                children.push_back(child_id);
            } else {
            }

            child_id = child.sybling;
        }

        for (auto& id : children) {
            updateChildrenTransforms(id, touched);
        }
    }


    void NodeStore::syncGPUBuffer(const BatchPointer& batch) {
        updateTransforms();

        _nodeInfos.sync_gpu_from_cpu(batch);
        _nodeTransforms.sync_gpu_from_cpu(batch);
    }

    void NodeStore::traverse(TraverseAccessor accessor) const {
        // lock the node store transform array as a whole
        // so we can use the unsafe data accessor in the search loop
        auto [begin_node, l] = _nodeTransforms.read(0);
        auto nodeCount = numAllocatedNodes();
        uint32_t nodeId = 0;

        while (nodeId < nodeCount) {
            auto currentInfo = _nodeInfos.unsafe_data(nodeId);
            traverseNode(*currentInfo, nodeId, 0, accessor);
            nodeId = currentInfo->sybling;
        }
    }

    void NodeStore::traverseNode(const NodeInfo& nodeInfo, NodeID nodeId, int32_t depth, TraverseAccessor accessor) const {
        accessor({ .id= nodeId, .info= nodeInfo, .transform= *_nodeTransforms.unsafe_data(nodeId), .name = _nodeNames[nodeId], .depth = depth});

        if (nodeInfo.num_children) {
            depth++;
            NodeIDs childrenIDs(nodeInfo.num_children, INVALID_NODE_ID);
            std::vector<const NodeInfo*> childrenInfos(nodeInfo.num_children, nullptr);
            uint32_t childNodeId = nodeInfo.children_head;
            int32_t i = -1;
            while (childNodeId != INVALID_NODE_ID) {
                ++i;
                auto childInfo = _nodeInfos.unsafe_data(childNodeId);
                childrenInfos[i] = childInfo;
                childrenIDs[i] = childNodeId;
                childNodeId = childInfo->sybling;
            }
            while (i >= 0) {
                traverseNode(*childrenInfos[i], childrenIDs[i], depth, accessor);
                --i;
            }
        }
    }
}
