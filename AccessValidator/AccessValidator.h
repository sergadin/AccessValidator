#pragma once

#include <map>
#include <vector>
#include <bitset>
#include <set>
#include <algorithm>
#include <iostream>
#include "predicate.h"

// Ошибки
class Error {};

class Inaccessible : public Error {};

// Попытка повторного добавления ребра 
class DuplicatedEdge : public Error {};

//
// Вершины
//

typedef unsigned int VertexID;
typedef double ValueType;

// Битовые множества требуют указать размер в момент компиляции.
// Эта константа задает максимально допустимое число ребер в графе.
const int max_edges = 32;
typedef std::bitset<max_edges> SatisfiedEdges;
typedef typename SatisfiedEdges EquivalenceClass;

// Описание класса эквивалентности значений вершины
struct ValueClass {
	ValueType val; // Представитель класса
	SatisfiedEdges edges_bitset; // Какие входящие ребра становятся выполнимыми, если вершина имеет это значение
	ValueClass(const ValueType& v, SatisfiedEdges &se) : val(v), edges_bitset(se) {}
};

typedef std::vector<ValueClass> ValueClasses;

class Graph;
class Vertex;

struct Edge {
	Vertex *from;
	Vertex *to;
	const Predicate *p;
	bool satisfied;
	size_t seq; // Порядковый номер ребра в графе
};

typedef std::vector<Edge*> Edges;

class Vertex {
	Graph *g_; // К какому графу относится
	Edges in_; // Входящие ребра
	Edges out_; // Исходящие ребра
	ValueType val_; // Значение вершины
	bool accessible_; // результат последнего вызова checkAccessibility

	ValueClasses classes_;

	// Добавление входящего в вершину ребра
	Edge *addInEdge(Edge *e);
	
	inline bool checkAccessibility() const; // Проверяет, что все исходящие ребра выполнены
	void changeAccessibility(bool new_value);
public:
	const VertexID id; // Номер, который использовался при инициализации вершины

	Vertex(Graph *g, VertexID vertex_id, const ValueType &val)
		: g_(g), id(vertex_id), val_(val), accessible_(false) {
	}
	~Vertex();

	void addOutEdge(Edge *edge);
	ValueType value() const { return val_; }
	void setValue(const ValueType &val); // Изменяет значение вершины. Создает исключение Inaccessible, если значение нельзя изменять
	const Edges& predecessors() const { return in_; }
	const Edges& successors() const { return out_; }
	void computeEquivalenceClasses(); // вызывать после того, как все in_, out_ ребра заполнены
	const ValueClasses &equivalenceClasses() const { return classes_; }

	friend std::ostream& operator<<(std::ostream& os, const Vertex& v);
	friend class Graph;
};


// Структра для "простого" описания ребер. Пример инициализации ниже.
typedef struct {
	VertexID from;
	VertexID to;
	int predicate;
} edge_info;

typedef typename std::set<VertexID> VertixIDSet;
typedef typename std::map<VertexID, ValueType> VerticesValues;

class Graph {
private:
	friend class Vertex;
	typedef typename std::map<VertexID, Vertex> VerticesMap;
	VerticesMap vertices_;
	Edges edges_;
	std::set<VertexID> accessibleVertices_; // Вершины, которые доступны в настощий момент; изменяется после вызова setValue()
	EquivalenceClass currentClass_;

	Graph() {} // Приватный конструктор запрещает создавать неинициализированные объекты

	Vertex *addVertex(VertexID vid, const ValueType &val); // Создает вершину (или находит по номеру) в vertices_

	// Отмечает доступность или недоступность вершины в acessibleVertices_
	void toggleVertexAccessibility(VertexID vid, bool accessibility);
public:
	Graph(const std::vector<edge_info>& edges, VerticesValues values, const std::vector<Predicate>& predicates);
	~Graph();

	ValueType setValue(VertexID v, const ValueType &val); // Установить значение вершины
	const Edges& predecessors(VertexID vid) const { return vertices_.at(vid).predecessors(); }
	const Edges& successors(VertexID vid) const { return vertices_.at(vid).successors(); }

	const VerticesMap& vertices() const {
		return vertices_;
	}
	const Vertex& vertex(VertexID vid) const { return vertices_.at(vid); }
	Vertex& vertex(VertexID vid) { return vertices_.at(vid); }

	const std::set<VertexID>& accessibleVertices() const { return accessibleVertices_; }
	const EquivalenceClass &equivalenceClass() const { return currentClass_; }
	friend std::ostream &operator<<(std::ostream &os, const Graph &g);
};


void solver(Graph &g, VertexID target);

//
// Реализация inline-функций
//
bool Vertex::checkAccessibility() const {
	for (Edges::const_iterator e = out_.begin(); e != out_.end(); e++) {
		if (!(*e)->satisfied)
			return false;
	}
	return true;
}
