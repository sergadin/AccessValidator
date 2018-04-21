#include <stack>
#include <unordered_set>
#include <deque>
#include <algorithm>
#include <iterator>
#include "AccessValidator.h"
#include "utils.h"

typedef std::unordered_set<EquivalenceClass> History;

typedef typename std::deque<VertexID> VertexIDSeq;

// Возвращает последовательность вершин для изменения (в порядке)
static VertexIDSeq chooseVerticesToModify(Graph &g) {
	const VertixIDSet &accessible = g.accessibleVertices();
	VertexIDSeq result;
	// ... копируем доступные вершины в случайном порядке
	for (VertixIDSet::const_iterator v = accessible.begin(); v != accessible.end(); v++) {
		result.push_back(*v);
	}
	return result;
}

// merge вычисляет (c1+c2) - removed.
// c2 объединенное с removed дает битовое множество ребер, входящих в вершину.
// Изменение значения в вершине приведет к тому, что ребра из c2 станут выполнены, а из removed - нет.
static EquivalenceClass merge(const EquivalenceClass &c1, const EquivalenceClass &c2, const EdgesBitset &removed) {
	EquivalenceClass res;
	for (int k = 0; k < c1.size(); k++) {
		res[k] = (c1[k] || c2[k]) && !removed[k];
	}
	return res;
}

// Состояние перебора определяется двумя параметрами:
// - списком вершин, которые могут быть сейчас изменены и
// - возможными новыми значениями (для каждого класса раскраски входящих ребер вершины выбираем одно значение)
//
// verticesToTry задает порядок изменяемых вершин (порядок определяется эвристикой)
// valueClassIterator указывает на класс значений вершины, который нужно попробовать следующим
struct SearchState {
	VertexIDSeq verticesToTry;
	ValueClasses::const_iterator valueClassIterator;
};


// Создает струкутру, которая определяет последовательность перебора 
static SearchState makeSearchState(Graph &g, const std::set<VertexID> &visited) {
	VertexIDSeq new_vertices_to_try = chooseVerticesToModify(g);
	std::remove_if(new_vertices_to_try.begin(), new_vertices_to_try.end(),
		[&visited](VertexID vid) { return contains(visited, vid); });
	if(!new_vertices_to_try.empty()) {
		const Vertex &v = g.vertex(new_vertices_to_try.at(0));
		const ValueClasses &equiv_classes = v.equivalenceClasses();
		return SearchState({ new_vertices_to_try, equiv_classes.begin() });
	}
	return SearchState({ new_vertices_to_try, ValueClasses::const_iterator() });
}

bool Solver::solver(Graph &g, VertexID target) {
	History classes_history; // Какие классы эквивалентности мы уже видели
	std::stack<SearchState> search_state; // Что еще осталось перебрать. Замена рекурсии
	std::set<VertexID> visited_vertices; // Чтобы по многу раз не пытаться изменять значения одной вершины

	this->trace = std::stack<UpdateInfo>();

	classes_history.insert(g.equivalenceClass());
	// Создадим корневую запись для организации перебора
	search_state.push(makeSearchState(g, visited_vertices));
	
	while (!search_state.empty()) {
		std::cout << "\n\n" << g << std::endl;

		if (contains(g.accessibleVertices(), target)) {
			std::cout << "SUCCESS! Chages made are (in reverse order):\n\n";
			while (!trace.empty()) {
				std::cout << trace.top().vid << " " << trace.top().old << " ==> " << trace.top().newValue << std::endl;
				trace.pop();
			}
			return true;
		}

		// Восстановим то место, где перебор был прерван "спуском вниз".
		// Это ссылки, то есть код ниже изменяет значения, которые записаны в стеке search_state.
		VertexIDSeq &to_modify = search_state.top().verticesToTry;
		ValueClasses::const_iterator &ec = search_state.top().valueClassIterator;

		bool vertex_updated = false;
		// Найдем следующее изменение значения вершины, которое приводит к новому классу
		while (!to_modify.empty() && !vertex_updated) {
			VertexID vid = to_modify.front();
			const ValueClasses &equiv_classes = g.vertex(vid).equivalenceClasses();
			visited_vertices.insert(vid);
			for (/* empty */; ec != equiv_classes.end(); ec++) {
				EquivalenceClass expected_outcome = merge(g.equivalenceClass(), ec->edges_bitset, ec->unsatisfied_edges);
				std::cout << "Inspecting possible change of " << vid << " from " << g.vertex(vid).value() << " to " << ec->val << std::endl;
				std::cout << "\tcurrent  : " << g.equivalenceClass()
					<< "\n\tsatisfied: " << ec->edges_bitset
					<< "\n\tmerged   : " << expected_outcome
					<< std::endl;
				if (!contains(classes_history, expected_outcome)) { // classes_history.find(expected_outcome) == classes_history.end()) {
					// Если мы изменим значение vid на ec->val, то попадем в новый класс. Делаем!
					ValueType old_value = g.setValue(vid, ec->val);
					trace.push({ vid, old_value, ec->val }); // Запомним сделанную замену
					classes_history.insert(g.equivalenceClass());
					std::cout << "Changed " << vid << " from " << old_value << " to " << ec->val << std::endl;
					// Добавим информацию о том, что нужно перебрать в новом состоянии
					search_state.push(makeSearchState(g, visited_vertices));
					vertex_updated = true;
					break;
				}
			}
			if (!vertex_updated) { // Не удалось найти значение для вершины vid
				to_modify.pop_front(); // переходим к следующей вершине
				if (!to_modify.empty()) // если вершина есть, то устанавливаем итератор на ее первый класс значений 
					ec = g.vertices().at(to_modify.front()).equivalenceClasses().begin();
			}
		}
		// Перебрали все вершины, поднимаемся на один уровень "рекурсии" вверх
		if (to_modify.empty()) {
			search_state.pop();
			if(!trace.empty())
				trace.pop();
		}
	}
	std::cout << "No access to " << target << "\n" << std::endl;
	return false;
}
