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
using namespace graphics;


TransformTree::NodeID TransformTree::allocate(const core::mat4x3& rts, NodeID parent) {

    NodeID new_node_id = _indexTable.allocate();
    bool allocated = new_node_id == (_indexTable.getNumAllocatedElements() - 1);

    if (allocated) {
        _treeNodes.push_back(Node());
        _nodeTransforms.push_back(rts);
        _worldTransforms.push_back(rts);
    } else {
        _treeNodes[new_node_id] = Node();
        _nodeTransforms[new_node_id] = rts;
        _worldTransforms[new_node_id] = rts;
    }

    attachNode(new_node_id, parent);

    return new_node_id;
}

TransformTree::NodeIDs TransformTree::allocateBranch(NodeID rootParent, const std::vector<core::mat4x3>& transforms, const NodeIDs& parentsOffsets) {
    NodeIDs new_node_ids = _indexTable.allocate(transforms.size());

    // first pass 
    uint32_t i = 0;
    for (auto new_node_id : new_node_ids) {
       // bool allocated = new_node_id == (_indexTable.getNumAllocatedElements() - 1);
        bool allocated = new_node_id == (_treeNodes.size());
        if (allocated) {
            _treeNodes.push_back(Node());
            _nodeTransforms.push_back(transforms[i]);
            _worldTransforms.push_back(transforms[i]);
        }
        else {
            _treeNodes[new_node_id] = Node();
            _nodeTransforms[new_node_id] = transforms[i];
            _worldTransforms[new_node_id] = transforms[i];
        }
        ++i;
    }

    // 2nd pass needs to happen after 1st pass allocation
    i = 0;
    for (auto new_node_id : new_node_ids) {
        auto o = parentsOffsets[i];
        attachNode(new_node_id, (o == INVALID_NODE_ID ? rootParent : new_node_ids[o]));
        i++;
    }

    return new_node_ids;
}

void TransformTree::free(NodeID nodeId) {
    _indexTable.free(nodeId);

    _treeNodes[nodeId] = Node();
    _nodeTransforms[nodeId] = core::mat4x3();
    _worldTransforms[nodeId] = core::mat4x3();
}

void TransformTree::attachNode(NodeID node_id, NodeID parent_id) {
    picoAssert(_indexTable.isValid(node_id));

    // Make sure the node is detached first
    detachNode(node_id);

    auto& the_node = _treeNodes[node_id];
    the_node.parent = parent_id;

    if (parent_id != INVALID_ID) {
        auto& parent_node = _treeNodes[parent_id];

        the_node.parent = parent_id;
        the_node.sybling = parent_node.children_head;

        parent_node.children_head = node_id;
        parent_node.num_children++;
    }

    _touchedTransforms.push_back(node_id);


    // invalid:
    // node world transform and world sphere
    // children world transform and world sphere
}

void TransformTree::detachNode(NodeID node_id) {
    auto& the_node = _treeNodes[node_id];
    if (the_node.parent == INVALID_ID) {
        return;
    }
    auto& parent_node = _treeNodes[the_node.parent];

    auto left_sybling_id = parent_node.children_head;
    if (left_sybling_id == node_id) {
        parent_node.children_head = the_node.sybling;
        parent_node.num_children--;
    } else {

        for (int i = 0; i < parent_node.num_children; i++) {
            auto& sybling_node = _treeNodes[left_sybling_id];
            if (sybling_node.sybling == node_id) {
                sybling_node.sybling = the_node.sybling;
                parent_node.num_children--;
                break;
            }

            left_sybling_id = sybling_node.sybling;
            picoAssert(left_sybling_id != INVALID_ID);
        }

        // Should never happen
        picoAssert(left_sybling_id != INVALID_ID);
    }


    the_node.parent = INVALID_ID;
    the_node.sybling = INVALID_ID;
}

void TransformTree::editTransform(NodeID nodeId, std::function<bool(core::mat4x3& rts)> editor) {
    if (editor(_nodeTransforms[nodeId])) {
        _touchedTransforms.push_back(nodeId);
    }
}

TransformTree::NodeIDs TransformTree::updateTransforms() {
    NodeIDs touched;
    for (auto node_id : _touchedTransforms) {
        const auto& node = _treeNodes[node_id];
        if (node.parent == INVALID_ID) {
            _worldTransforms[node_id] = _nodeTransforms[node_id];
        } else {
            _worldTransforms[node_id] = core::mul(_worldTransforms[node.parent], _nodeTransforms[node_id]);
        }
        touched.push_back(node_id);
        updateChildrenTransforms(node_id, touched);
    }
    _touchedTransforms.clear();

    return touched;
}


void TransformTree::updateChildrenTransforms(NodeID parentId, NodeIDs& touched) {

    const auto& node = _treeNodes[parentId];
    if (node.num_children == 0) {
        return;
    }
    const auto& parent_world_transform = _worldTransforms[parentId];

    NodeIDs children;
    children.reserve(node.num_children);

    NodeID child_id = node.children_head;
    for (int i = 0; i < node.num_children; ++i) {
        _worldTransforms[child_id] = core::mul(parent_world_transform, _nodeTransforms[child_id]);
        touched.push_back(child_id);

        const auto& child = _treeNodes[child_id];
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

void NodeStore::resizeBuffers(const DevicePointer& device, uint32_t  numElements) {

    if (_num_buffers_elements < numElements) {
        auto capacity = (numElements);

        graphics::BufferInit nodes_buffer_init{};
        nodes_buffer_init.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
        nodes_buffer_init.hostVisible = true;
        nodes_buffer_init.bufferSize = capacity * sizeof(vec4);
        nodes_buffer_init.firstElement = 0;
        nodes_buffer_init.numElements = capacity;
        nodes_buffer_init.structStride = sizeof(vec4);

        _nodes_buffer = device->createBuffer(nodes_buffer_init);


        graphics::BufferInit transforms_buffer_init{};
        transforms_buffer_init.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
        transforms_buffer_init.hostVisible = true;
        transforms_buffer_init.bufferSize = 2 * capacity * sizeof(mat4x3);
        transforms_buffer_init.firstElement = 0;
        transforms_buffer_init.numElements = 2 * capacity;
        transforms_buffer_init.structStride = sizeof(mat4x3);

        _transforms_buffer = device->createBuffer(transforms_buffer_init);

        _num_buffers_elements = capacity;
    }
}


NodeID NodeStore::createNode(const core::mat4x3& rts, NodeID parent) {
    auto nodeId = _tree.allocate(rts, parent);
    
    if (nodeId < _num_buffers_elements) {
        reinterpret_cast<TransformTree::Node*>(_nodes_buffer->_cpuMappedAddress)[nodeId] = _tree._treeNodes[nodeId];
        reinterpret_cast<core::mat4x3*>(_transforms_buffer->_cpuMappedAddress)[2 * nodeId] = _tree._nodeTransforms[nodeId];
    }

    return nodeId;
}

NodeIDs NodeStore::createNodeBranch(NodeID rootParent, const std::vector<core::mat4x3>& transforms, const NodeIDs& parentsOffsets) {
    auto nodeIds = _tree.allocateBranch(rootParent, transforms, parentsOffsets);

    if (nodeIds.back() < _num_buffers_elements) {
        for (auto n: nodeIds) {
            reinterpret_cast<TransformTree::Node*>(_nodes_buffer->_cpuMappedAddress)[n] = _tree._treeNodes[n];
            reinterpret_cast<core::mat4x3*>(_transforms_buffer->_cpuMappedAddress)[2 * n] = _tree._nodeTransforms[n];
        }
    }

    return nodeIds;
}

void NodeStore::deleteNode(NodeID nodeId) {
    _tree.free(nodeId);
}

void NodeStore::attachNode(NodeID child, NodeID parent) {
    _tree.attachNode(child, parent);
}

void NodeStore::detachNode(NodeID child) {
    _tree.detachNode(child);
}

void NodeStore::editTransform(NodeID nodeId, std::function<bool(core::mat4x3& rts)> editor) {
    _tree.editTransform(nodeId, editor);
}

void NodeStore::updateTransforms() {
    auto touched_ids = _tree.updateTransforms();
    if (!touched_ids.empty()) {
        auto gpu_transforms = reinterpret_cast<core::mat4x3*>(_transforms_buffer->_cpuMappedAddress);
    
        for (const auto& id : touched_ids) {
            gpu_transforms[2 * id] = _tree._nodeTransforms[id];
            gpu_transforms[2 * id + 1] = _tree._worldTransforms[id];
        }
    }
}

const Node Node::null {};

