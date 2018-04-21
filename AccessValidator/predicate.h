#pragma once

#include <limits>
#include <vector>
#include <bitset>
#include <unordered_set>


struct Interval {
	double left; // Границы отрезков могут быть +- бесконечностью
	double right;
	enum BorderType {OPEN=0, RIGHT_CLOSED=1, LEFT_CLOSED=2, DOUBLE_CLOSED=3} border;

	bool contains(double x) const {
		return ((left < x) || ((left <= x) && (border & LEFT_CLOSED))) &&
			((x < right) || (x <= right && (border & RIGHT_CLOSED)));
	}
};

class Predicate {
	std::vector<Interval> truth_intervals_; // Интервалы без пересечений, в порядке возрастания левой границы.
public:
	// Создание по множеству интервалов, на которых предикат принимает истинное значени.
	// Входные интервалы могут пересекаться и перечисляться в произвольном порядке.
	Predicate(const std::vector<Interval>& truth_intervals);
	// По умолчанию создаем тождественно верный предикат
	Predicate() {
		addInterval({ -1 * std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), Interval::OPEN }); 
	}

	bool check(double x) const; // выполнимость в точке x
	const std::vector<Interval>& intervals() const { return truth_intervals_; }
	void clear() { truth_intervals_.clear(); }
	void addInterval(const Interval& truth_interval);

	// Тождественно ложен?
	bool isEmpty() const { return truth_intervals_.empty(); }

	friend Predicate intersect(const Predicate& p1, const Predicate& p2); // Пересечение
	double sample() const; // Поиск значения при котором предикат выполняется
	friend std::ostream& operator<< (std::ostream &os, const Predicate &p);	
};
