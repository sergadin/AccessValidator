
#include <iostream>
#include <limits>
#include <cassert>

#include "AccessValidator.h"

//
// Vertices
//

static void checkEdgeUniqueness(const Edges& edges, Vertex *v, bool reverse_edge) {
	Edges::const_iterator it = std::find_if(edges.begin(), edges.end(), [&](const Edge* e) { return (reverse_edge ? e->to : e->from) == v; });
	if (it != edges.end())
		throw DuplicatedEdge();
}

Edge* Vertex::addInEdge(Edge *edge) {
	checkEdgeUniqueness(in_, edge->from, true);
	in_.push_back(edge);
	return edge;
}

void Vertex::addOutEdge(Edge *edge) {
	checkEdgeUniqueness(out_, edge->to, false);
	out_.push_back(edge);
	edge->to->addInEdge(edge);
}

void Vertex::changeAccessibility(bool new_value) {
	if (new_value == accessible_)
		return;
	accessible_ = new_value;
	g_->toggleVertexAccessibility(id, accessible_);
}

void Vertex::setValue(const ValueType &val)
{
	if(!accessible_)
		throw Inaccessible();
	val_ = val;
	for (Edges::iterator it = in_.begin(); it != in_.end(); it++) {
		Edge *pe = *it;
		pe->satisfied = pe->p->check(val_);
		g_->currentClass_[pe->seq] = pe->satisfied;
		if (!pe->satisfied) {
			pe->from->changeAccessibility(false);
		}
		else {
			pe->from->changeAccessibility(pe->from->checkAccessibility());
		}
	}
}

static bool covered(const SatisfiedEdges &se, const ValueClasses &classes)
{
	for (ValueClasses::const_iterator vc = classes.begin(); vc != classes.end(); vc++) {
		bool covered = true;
		for (int k = 0; k < se.size() && covered; k++) {
			if (se[k] && !vc->edges_bitset[k])
				covered = false;
		}
		if (covered)
			return true;
	}
	return false;
}

void Vertex::computeEquivalenceClasses()
{
	if (in_.size() == 0) {
		classes_.push_back(ValueClass(1.23456, SatisfiedEdges()));
		return; // No incoming edges
	}
	// Заполним вектор порядковых номеров входящих ребер
	std::vector<size_t> ids(in_.size());
	for (size_t k = 0; k < in_.size(); k++) {
		ids[k] = in_.at(k)->seq;
	}

	// Переберем все подмножества входящих ребер и проверим возможность их выполнимости.
	// Это можно значительно ускорить, но надо ли?
	for (size_t bitset = (1 << in_.size()) - 1; bitset > 0; --bitset) {
		Predicate pred;
		SatisfiedEdges se;
		for (int k = 0; k < in_.size() && !pred.isEmpty(); k++) {
			if (bitset & (1 << k)) {
				se[ids[k]] = true;
				pred = intersect(pred, *(in_.at(k)->p));
			}
		}
		if (!pred.isEmpty() && !covered(se, classes_)) {
			std::cerr << pred << std::endl;
			classes_.push_back(ValueClass(pred.sample(), se));
		}
	}
}

Vertex::~Vertex() {
	// std::cerr << "Deleting vertex " << id << " at " << this << std::endl;
}


std::ostream& operator<<(std::ostream& os, const Vertex& v) {
	os << "Vertex " << v.id << ", value = " << v.val_  << "\n";
	os << "\tIncoming edges are:\n";
	if (v.in_.begin() == v.in_.end())
		os << "\tNone\n";
	for (Edges::const_iterator e = v.in_.begin(); e != v.in_.end(); e++) {
		os << "\t<- " << (*e)->from->id << " [" << (*e)->from->val_ << "] " << *((*e)->p) << "\n";
	}
	os << "\tOutcoming edges are:\n";
	if (v.out_.begin() == v.out_.end())
		os << "\tNone\n";
	for (Edges::const_iterator e = v.out_.begin(); e != v.out_.end(); e++) {
		os << "\t-> " << (*e)->to->id << " [" << (*e)->to->val_ << "] " << *((*e)->p) << "\n";
	}
	os << "\tEquivalence classes:\n";
	for (ValueClasses::const_iterator vc = v.classes_.begin(); vc != v.classes_.end(); vc++) {
		os << "\t\t" << vc->val << " " << vc->edges_bitset << "\n";
	}
	os << std::endl;
	return os;
}

//
// Graphs
//

Vertex *Graph::addVertex(VertexID vid, const ValueType &val) {
	VerticesMap::iterator v = vertices_.find(vid);
	if (v == vertices_.end()) {
		std::pair<VerticesMap::iterator, bool> ret;
		ret = vertices_.insert(VerticesMap::value_type(vid, Vertex(this, vid, val)));
		v = ret.first;
	}
	return &(v->second);
}

Graph::Graph(const std::vector<edge_info>& edges, VerticesValues values, const std::vector<Predicate>& predicates)
{
	edges_.reserve(edges.size());

	for (std::vector<edge_info>::const_iterator e = edges.begin(); e != edges.end(); e++) {
		Vertex *v_from = addVertex(e->from, values[e->from]);
		Vertex *v_to = addVertex(e->to, values[e->to]);
		const Predicate *p = &(predicates.at(e->predicate));
		Edge *edge = new Edge({ v_from, v_to, p, p->check(v_to->value()), edges_.size() });
		edges_.push_back(edge);
		v_from->addOutEdge(edge);
	}
	for (VerticesMap::iterator v = vertices_.begin(); v != vertices_.end(); v++) {
		v->second.computeEquivalenceClasses();
		if (v->second.checkAccessibility()) {
			accessibleVertices_.insert(v->second.id);
			v->second.accessible_ = true;
		}
	}
}

Graph::~Graph() {
	for (Edges::iterator e = edges_.begin(); e != edges_.end(); e++) {
		delete *e;
	}
}

ValueType Graph::setValue(VertexID vid, const ValueType &val) {
	Vertex& v = vertices_.at(vid);
	ValueType old_value = v.value();

	if (!v.accessible_)
		throw Inaccessible();
	v.val_ = val;
	for (Edges::iterator it = v.in_.begin(); it != v.in_.end(); it++) {
		Edge *e = *it;
		e->satisfied = e->p->check(v.val_);
		currentClass_[e->seq] = e->satisfied;
		if (!e->satisfied) {
			e->from->changeAccessibility(false);
		}
		else {
			e->from->changeAccessibility(e->from->checkAccessibility());
		}
	}

	return old_value;
}

void Graph::toggleVertexAccessibility(VertexID vid, bool accessibility) {
	if (accessibility)
		accessibleVertices_.insert(vid);
	else
		accessibleVertices_.erase(vid);
}


std::ostream& operator<<(std::ostream& os, const Graph& g) {
	os << "Graph " << g.vertices_.size() << " vertices.";
	os << "\tAccessible: ";
	for (std::set<VertexID>::const_iterator v = g.accessibleVertices_.begin(); v != g.accessibleVertices_.end(); v++) {
		if (v != g.accessibleVertices_.begin())
			os << ", ";
		os << *v;
	}
	return os;
}
