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


TransformTree::NodeID TransformTree::allocate(const core::mat4x3& rts, const core::aabox3& box, NodeID parent) {

    NodeID new_node_id = _indexTable.allocate();
    bool allocated = new_node_id == (_indexTable.getNumAllocatedElements() - 1);

    if (allocated) {
        _treeNodes.push_back(Node());
        _nodeTransforms.push_back(rts);
        _nodeBoxes.push_back(box);
        _worldTransforms.push_back(rts);
        _worlSpheres.push_back(box.toSphere());
    } else {
        _treeNodes[new_node_id] = Node();
        _nodeTransforms[new_node_id] = rts;
        _nodeBoxes[new_node_id] = box;
        _worldTransforms[new_node_id] = rts;
        _worlSpheres[new_node_id] = box.toSphere();
    }

    attachNode(new_node_id, parent);

    return new_node_id;
}

void TransformTree::free(NodeID nodeId) {
    _indexTable.free(nodeId);

    _treeNodes[nodeId] = Node();
    _nodeTransforms[nodeId] = core::mat4x3();
    _nodeBoxes[nodeId] = core::aabox3();
    _worldTransforms[nodeId] = core::mat4x3();
    _worlSpheres[nodeId] = core::vec4(0.0f, 0.0f, 0.0f, 1.0f);
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

        _touchedBounds.push_back(parent_id);
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
    _touchedBounds.push_back(the_node.parent);

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
        }
        child_id = child.sybling;
    }

    for (auto& id : children) {
        updateChildrenTransforms(id, touched);
    }
}


TransformTreeGPU::TransformTreeGPU() {


}

TransformTreeGPU::~TransformTreeGPU() {

}


void TransformTreeGPU::resizeBuffers(const DevicePointer& device, uint32_t  numElements) {

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


        graphics::BufferInit bounds_buffer_init{};
        bounds_buffer_init.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
        bounds_buffer_init.hostVisible = true;
        bounds_buffer_init.bufferSize = capacity * sizeof(vec4);
        bounds_buffer_init.firstElement = 0;
        bounds_buffer_init.numElements = capacity;
        bounds_buffer_init.structStride = sizeof(vec4);

        _bounds_buffer = device->createBuffer(bounds_buffer_init);

    }
}


TransformTree::NodeID TransformTreeGPU::createNode(const core::mat4x3& rts, NodeID parent) {
    auto nodeId = _tree.allocate(rts, core::aabox3(), parent);
    
    reinterpret_cast<TransformTree::Node*>(_nodes_buffer->_cpuMappedAddress)[nodeId] = _tree._treeNodes[nodeId];
    reinterpret_cast<core::mat4x3*>(_transforms_buffer->_cpuMappedAddress)[2 * nodeId] = _tree._nodeTransforms[nodeId];
   // reinterpret_cast<core::vec4*>(_bounds_buffer->_cpuMappedAddress)[nodeId] = _tree._nodeBoxes[nodeId];

    return nodeId;
}

void TransformTreeGPU::deleteNode(NodeID nodeId) {
    _tree.free(nodeId);
}

void TransformTreeGPU::attachNode(NodeID child, NodeID parent) {
    _tree.attachNode(child, parent);
}

void TransformTreeGPU::detachNode(NodeID child) {
    _tree.detachNode(child);
}

void TransformTreeGPU::editTransform(NodeID nodeId, std::function<bool(core::mat4x3& rts)> editor) {
    auto gpu_transforms = reinterpret_cast<core::mat4x3*>(_transforms_buffer->_cpuMappedAddress);

    _tree.editTransform(nodeId, [gpu_transforms, editor, nodeId] (core::mat4x3& rts) {
        editor(rts);
        gpu_transforms[2 * nodeId] = rts;
        return true;
    });

}

void TransformTreeGPU::updateTransforms() {    
    auto touched_ids = _tree.updateTransforms();
    if (!touched_ids.empty()) {
        auto gpu_transforms = reinterpret_cast<core::mat4x3*>(_transforms_buffer->_cpuMappedAddress);
    
        for (const auto& id : touched_ids) {
            gpu_transforms[2 * id + 1] = _tree._worldTransforms[id];
        }
    }
}
