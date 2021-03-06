// AccessValidator.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <limits>
#include <cassert>

#include "AccessValidator.h"

int main() {
	// Проверим, что допускается использование бесконечности.
	// Если это не компилируется, то можно удалить три строчки, но не будет бесконечных пределов
	static_assert(std::numeric_limits<float>::is_iec559, "IEEE 754 required");
	const double inf = std::numeric_limits<double>::infinity();
	const double neg_inf = -1 * std::numeric_limits<double>::infinity();

	// Определим предикаты (индексы 0, 1, 2)
	std::vector<Predicate> predicates = {
		Predicate({
			{ 0, 1, Interval::DOUBLE_CLOSED }}), // [0, 1]
		Predicate({
			{2, 3, Interval::LEFT_CLOSED}}), // [2, 3)
		Predicate({
			{ neg_inf, -4, Interval::RIGHT_CLOSED }, // <= -4
			{ 6, inf, Interval::OPEN }}) // > 6 
	};

	// Задаем ребра графа; вершины нумеруются начиная с нуля
	std::vector<edge_info> edges = {
		{ 0, 1, 0 },
		{ 0, 2, 1 },
		{ 1, 2, 2 }
	};

	VerticesValues values = {
		{ 0, 3.12 },
		{ 1, 2.17 },
		{ 2, 5.0 }
	};

	std::cerr << "Building graph..." << std::endl;
	std::cerr << predicates[0];
	Graph g(edges, values, predicates);
	std::cerr << "Please, consider FIXING Vertex::computeEquivalenceClasses()!" << std::endl;
	std::cout << g.vertices().at(2) << std::endl;

	Solver s;
	s.solver(g, 0);

	return 0;
}

