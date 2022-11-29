// Graph.cpp
//
// Sam Gateau - October 2022
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
#include "Graph.h"

#include <set>

#include <iostream>

using namespace core;


void Graph::Node::log()
{
    std::clog << "Node #" << uuid << std::endl;
    std::clog << "    <" << _type->name << ">" << std::endl;
    for (auto i : ins)
    {
        auto p = _graph->pin(i);
        std::clog << "    >--" << p->name() << std::endl;
        for (auto ei : p->edges)
        {
            auto e = _graph->_edges[ei];
            auto s = _graph->pin(e.from);
            std::clog << "      +--< n#" << s->node->uuid << "." << s->name() << std::endl;
        }

    }
    for (auto o : outs)
    {
        auto p = _graph->pin(o);
        std::clog << "    -->" << p->name() << std::endl;
        for (auto ei : p->edges)
        {
            auto e = _graph->_edges[ei];
            auto d = _graph->pin(e.to);
            std::clog << "       +--> n#" << d->node->uuid << "." << d->name() << std::endl;
        }

    }
}

Graph::NodePointer Graph::createNode(const NodeTypePointer& nodeType) {
    auto n = std::make_shared<Node>();
    n->_graph = this;

    for (auto& i : nodeType->in_pins.pins)
    {
        Pin p = { n.get(), i.hash(), (Index) _pins.size(), {}};
        _pins.emplace_back(p);
        n->ins.push_back(p.uuid);
    }
    for (auto& i : nodeType->out_pins.pins)
    {
        Pin p = { n.get(), i.hash(), (Index) _pins.size(), {} };
        _pins.emplace_back(p);
        n->outs.push_back(p.uuid);
    }

    n->_type = nodeType;

    n->uuid = _nodes.size();

    _nodes.emplace_back(n);

    return n;
}

Graph::ID Graph::connect(Node& nFrom, Name& pFrom, Node& nTo, Name& pTo)
{
    auto p_from = nFrom.out_pin(pFrom);
    if (p_from == nullptr)
        return NULL_ID;

    auto p_to = nTo.in_pin(pTo);
    if (p_to == nullptr)
        return NULL_ID;

    Edge e;

    // detect if there is already an edge arriving to destination we could update and reuse
    if (!p_to->edges.empty())
    {
        e.uuid = p_to->edges.front();
        e = _edges[e.uuid];

        // Remove edge arriving to dest pin from the previous pin.
        auto& op = _pins[e.from];
        // op.removeEdgeId(e.uuid);
        for (auto it = op.edges.begin(); it != op.edges.end(); ++it)
        {
            if (*it == e.uuid)
            {
                op.edges.erase(it);
                break;
            }
        }

        // now simply update the from pin and the reuse the allocated edge
        p_from->edges.emplace_back(e.uuid);
        _edges[e.uuid].from = p_from->uuid;
        return e.uuid;
    }
    
    // else just add a new edge
    e.from = p_from->uuid;
    e.to = p_to->uuid;
    e.uuid = _edges.size();
    _edges.emplace_back(e);

    p_to->edges.emplace_back(e.uuid);
    p_from->edges.emplace_back(e.uuid);
    return e.uuid;
}

Graph::IDs Graph::gatherSourceNodes(const Node* n) const
{
    bool allInConnected = true;
    bool allInDisconnected = true;
    IDs sourceNodes;
    for (auto opi : n->ins)
    {
        const auto& p = _pins[opi];
        if (p.edges.empty())
        {
            allInConnected = false;
        } else
        {
            allInDisconnected = false;
            const auto& e = _edges[p.edges[0]];
            const auto& ps = _pins[e.from];
            sourceNodes.emplace_back(ps.node->uuid);
        }
    }
    return sourceNodes;
}

void Graph::gatherOuterNodes(const std::vector<IDs>& sourceNodesPerNode, std::set<ID>& nodeReservoirA, std::set<ID>& nodeReservoirB) const
{
    // Fill reservoir B with the nodes feeding inputs to some other nodes
    for (auto a : nodeReservoirA)
    {
        for (auto b : sourceNodesPerNode[a])
        {
            nodeReservoirB.insert(b);
        }      
    }

    // Now remove the inputing nodes from reservoir a
    for (auto b : nodeReservoirB)
    {
        nodeReservoirA.erase(b);
    }

    // Reservoir A is left with only outer nodes
}

void Graph::evalInOutPins() const
{
    _inPins.clear();
    _outPins.clear();

    // just grab the ins and outs pins for the full graph
    for (auto& p : _pins)
    {
        if (p.edges.empty())
        {
            auto a = p.pinType().access;
            if (a == PinAccess::in)
            {
                _inPins.push_back(p.uuid);
            } else if (a == PinAccess::out)
            {
                _outPins.push_back(p.uuid);
            }
        }
    }
}

void Graph::evalTraverseOrder() const
{
    _traverseOrder.clear();
    _inNodesCount = 0;
    _outNodesCount = 0;
    _nodeWavesCount = 0;

    // Store all the nodes in a set, the starting reservoir
    std::set<ID> nodeReservoirs[2];
    for (auto& n : _nodes)
        nodeReservoirs[0].insert(n->uuid);

    // For every node, the source nodes needed
    std::vector<IDs> sourceNodesArray;
    for (auto& n : _nodes)
        sourceNodesArray.emplace_back(gatherSourceNodes(n.get()));

    // Find the outer nodes from the reservoir
    // start from the full reservoir, every pass, separate the outers and 
    // record them in the traverse order.
    auto pass = 0;
    auto reservoirIndex = pass % 2;
    while (nodeReservoirs[reservoirIndex].size())
    {
        gatherOuterNodes(sourceNodesArray, nodeReservoirs[reservoirIndex], nodeReservoirs[(pass + 1) % 2]);
        if (nodeReservoirs[pass].empty())
            break; /// THis should not happen, means a 

        // Add all the outer nodes in the traverse order
        for (auto n : nodeReservoirs[reservoirIndex])
            _traverseOrder.push_back(n);

        // Store the number of outers in this pass
        _inNodesCount = nodeReservoirs[reservoirIndex].size();

        // How many last outer nodes
        if (pass == 0) 
            _outNodesCount = _inNodesCount;

        // next pass start fresh with the B reservoir
        nodeReservoirs[reservoirIndex].clear();
        ++pass;
        reservoirIndex = pass % 2;
    }
    _nodeWavesCount = pass;
    std::reverse(_traverseOrder.begin(), _traverseOrder.end());
}

void Graph::traverse(std::function<void(Node*, int32_t)> visitor) const
{
    evalTraverseOrder();

    int32_t i = 0;
    for (auto ni : _traverseOrder)
    {
        visitor(_nodes[ni].get(), i);
        ++i;
    }
}

void Graph::log() const
{
    traverse([](Node* n, int32_t i) {
            std::clog << "#" << i << std::endl;

            n->log();
        });
    std::clog << "Num node passes " << _nodeWavesCount << std::endl;
    std::clog << "In node count " << _inNodesCount << std::endl;
    std::clog << "Out node count " << _outNodesCount << std::endl;
}

Graph::NodeGraphPointer Graph::createNodeGraph(const Graph& subgraph)
{
    // Make the node type out of the subgraph

    // duplicate the subgraph


}

void Graph::testGraph()
{
    NodeTypePointer nt = std::make_shared<NodeType>(NodeType::Init{
        "MyFirstType",
        {ADD_IN(aaa, Scalar)},
        {ADD_OUT(bbb, Scalar)}
        });
    
    NodeTypePointer nt2 = std::make_shared<NodeType>(NodeType::Init{
        "MySecondType",
        {ADD_IN(i0, Scalar), ADD_IN(i1, Image)},
        {ADD_OUT(o0, Scalar), ADD_OUT(o1, Scalar) }
        });
    
    Graph g;
    auto n0 = g.createNode(nt);
    
    auto n1 = g.createNode(nt);

    auto n2 = g.createNode(nt);
    
    auto n3 = g.createNode(nt);

    auto n4 = g.createNode(nt2);
    
    g.connect(n0, "bbb", n1, "aaa");
    g.connect(n2, "bbb", n0, "aaa");
  
    g.connect(n4, "o0", n2, "aaa");
    g.connect(n4, "o1", n3, "aaa");
    
    g.log();

    Graph g2;

    auto nodeA = g2.createNodeGraph(g);


}


