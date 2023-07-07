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

Graph::VarID Graph::allocateVar(Hash h, DataType dt)
{
    Var v = { (Index)_vars.size(), h, dt };
    _vars.emplace_back(v);
    return v.uuid;
}

void Graph::setupNode(Node* n, const NodeTypePointer& nodeType)
{
    n->_graph = this;

    for (auto& i : nodeType->in_pins.pins)
    {
        Pin p = { n, i.hash(), (Index)_pins.size(), {}, 0 };
        p.var = allocateVar(p.hash(), i.type.dataType);

        _pins.emplace_back(p);
        n->ins.push_back(p.uuid);
    }
    for (auto& i : nodeType->out_pins.pins)
    {
        Pin p = { n, i.hash(), (Index)_pins.size(), {} };
        p.var = allocateVar(p.hash(), i.type.dataType);
      
        _pins.emplace_back(p);
        n->outs.push_back(p.uuid);
    }


    n->_type = nodeType;

    n->uuid = _nodes.size();

}

Graph::NodePointer Graph::createNode(const NodeTypePointer& nodeType) {
    auto n = std::make_shared<Node>();

    setupNode(n.get(), nodeType);

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

        // Update the var id on p_to, same as p_from
        auto formerVar = p_to->swapVar(p_from->var);

        return e.uuid;
    }
    
    // else just add a new edge
    e.from = p_from->uuid;
    e.to = p_to->uuid;
    e.uuid = _edges.size();
    _edges.emplace_back(e);

    p_to->edges.emplace_back(e.uuid);
    p_from->edges.emplace_back(e.uuid);

    // Update the var id on p_to, same as p_from
    auto formerVar = p_to->swapVar(p_from->var);

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

void Graph::traverse(std::function<void(Node*, int32_t, int32_t)> visitor, int32_t graph_depth) const
{
    evalTraverseOrder();

    int32_t i = 0;
    for (auto ni : _traverseOrder)
    {
        if (_nodes[ni]->isNodeGraph())
        {
            visitor(_nodes[ni].get(), i, graph_depth);
         /*   ++graph_depth;
            static_cast<Graph::NodeGraph*> (_nodes[ni].get())->_subgraph->traverse(visitor, graph_depth);
            --graph_depth;*/
        }
        else
            visitor(_nodes[ni].get(), i, graph_depth);

        ++i;
    }
}

void Graph::Node::log(std::ostream& log, int32_t max_graph_depth, int32_t graph_depth) const
{
    std::string tabh( 4 * graph_depth, ' ' );
    log << tabh << "  ** Node <" << _type->name << "> #" << uuid << std::endl;

    std::string tabi = tabh + "   ";

    for (auto i : ins)
    {
        auto p = _graph->pin(i);
        auto v = _graph->var(p->var);
        log << tabi << "-> " << p->name() << "    " << " v" << std::dec << v->uuid << " h=" << std::hex << v->hash << std::endl;
        for (auto ei : p->edges)
        {
            auto e = _graph->_edges[ei];
            auto s = _graph->pin(e.from);
            log << tabi << "    +-- n#" << s->node->uuid << "." << s->name() << std::endl;
        }

    }

    std::string tabo = tabh + "      ************ ";

    for (auto o : outs)
    {
        auto p = _graph->pin(o);
        auto v = _graph->var(p->var);
        log << tabo << ">- " << p->name() << "    " << " v" << std::dec << v->uuid << " h=" << std::hex << v->hash << std::endl;
        for (auto ei : p->edges)
        {
            auto e = _graph->_edges[ei];
            auto d = _graph->pin(e.to);
            log << tabo << "     +--> n#" << d->node->uuid << "." << d->name() << std::endl;
        }

    }
    log << tabh << "  **" << std::endl;

}

void Graph::log(std::ostream& log, int32_t max_graph_depth, int32_t graph_depth) const
{
    std::string tab(4 * graph_depth, ' ');
    log << tab << "**** Graph " << _name << std::endl;

    traverse([tab, &log, max_graph_depth, graph_depth](Node* n, int32_t i, int32_t l) {
            n->log(log, max_graph_depth, l + graph_depth);
        });

    traverse([&, tab](Node* n, int32_t i, int32_t l) {
        
        log << tab << "  n" << n->uuid << "<" << n->_type->name << "> \t (";
        for (auto i : n->ins)
        {
            auto p = n->_graph->pin(i);
            auto v = n->_graph->var(p->var);
            log << " v" << v->uuid << ", ";
        }

        log << ") -> [";
        for (auto i : n->outs)
        {
            auto p = n->_graph->pin(i);
            auto v = n->_graph->var(p->var);
            log << " v" << v->uuid << ", ";
        }
        log << "]" << std::endl;
    });

    log << tab << "  Num node passes " << _nodeWavesCount << std::endl;
    log << tab << "  In node count " << _inNodesCount << std::endl;
    log << tab << "  Out node count " << _outNodesCount << std::endl;
    log << tab << "****" << std::endl;
}

void Graph::NodeGraph::log(std::ostream& log, int32_t max_graph_depth, int32_t graph_depth) const
{
    std::string tab(4 * graph_depth, ' ');
    log << tab << " *** NodeGraph " << std::endl; 
    Node::log(log, graph_depth);
    if ((graph_depth + 1) < max_graph_depth)
        _subgraph->log(log, graph_depth + 1, max_graph_depth);

    log << tab << " ***" << std::endl;
}

Graph::NodeTypePointer Graph::makeNodeGraphType() const
{
    evalInOutPins();

    NodeType::Init init{ _name };
    for (auto i : _inPins)
    {
        init.ins.emplace_back(_pins[i].namedPinType());
    }
    for (auto i : _outPins)
    {
        init.outs.emplace_back(_pins[i].namedPinType());
    }
    init.isNodeGraph = true;
    NodeTypePointer nodeType = std::make_shared<NodeType>(init);

    return nodeType;
}

Graph::NodeGraphPointer Graph::createNodeGraph(const GraphPointer& subgraph)
{
    // Make the node type out of the subgraph

    auto nodeType = subgraph->makeNodeGraphType();

    // duplicate the subgraph
    auto n = std::make_shared<NodeGraph>();
    n->_subgraph = subgraph;

    setupNode(n.get(), nodeType);

    _nodes.emplace_back(n);

    return n;
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
    
    auto gA = std::make_shared<Graph>();

    auto n0 = gA->createNode(nt);
    
    auto n1 = gA->createNode(nt);

    auto n2 = gA->createNode(nt);
    
    auto n3 = gA->createNode(nt);

    auto n4 = gA->createNode(nt2);
    
    gA->connect(n0, "bbb", n1, "aaa");
    gA->connect(n2, "bbb", n0, "aaa");
  
    gA->connect(n4, "o0", n2, "aaa");
    gA->connect(n4, "o1", n3, "aaa");
    
    gA->log(std::clog);

    auto gB = std::make_shared<Graph>(Graph::Init{ "graphB" });

    auto nA = gB->createNodeGraph(gA);

    auto n5 = gB->createNode(nt);

   // gB->connect(n5, "bbb", nA, "bbb");

    gB->log(std::clog, 1);
}


