#include <algorithm>
#include <iostream>
#include "predicate.h"

Predicate::Predicate(const std::vector<Interval>& truth_intervals)
	: truth_intervals_()
{
	for (int k = 0; k < truth_intervals.size(); k++)
		addInterval(truth_intervals[k]);
}

bool Predicate::check(double x) const {
	for (int k = 0; k < truth_intervals_.size(); k++)
		if (truth_intervals_[k].contains(x))
			return true;
	return false;
}

void Predicate::addInterval(const Interval & truth_interval)
{
	// FIXME: удалить возможные пересечения входных интервалов
	truth_intervals_.push_back(truth_interval);
}

static Interval::BorderType borderType(int closeness_bitset) {
	switch (closeness_bitset) {
	case 0: return Interval::OPEN;
	case 1: return Interval::RIGHT_CLOSED;
	case 2: return Interval::LEFT_CLOSED;
	case 3: return Interval::DOUBLE_CLOSED;
	}
	std::cerr << "Unable to convert borderType into enum value; borderness_bitset = " << closeness_bitset << std::endl;
	return Interval::OPEN;
}

Predicate intersect(const Predicate & p1, const Predicate &p2)
{
	std::vector<Interval>::const_iterator i, p2_i;
	Predicate result;
	result.clear();
	i = p1.truth_intervals_.begin();
	p2_i = p2.truth_intervals_.begin();
	while (i != p1.truth_intervals_.end() && p2_i != p2.truth_intervals_.end()) {
		if (i->right >= p2_i->left && i->left <= p2_i->right ) {
			double left = std::max(i->left, p2_i->left);
			double right = std::min(i->right, p2_i->right);
			Interval::BorderType bt = borderType(
				((i->left < p2_i->left ? p2_i->border : i->border) & Interval::LEFT_CLOSED) |
				((i->right > p2_i->right ? p2_i->border : i->border) & Interval::RIGHT_CLOSED));
			if (left < right || bt != Interval::OPEN)
				result.addInterval({ left, right, bt });
		}
		if (i->right < p2_i->left)
			i++;
		else
			p2_i++;
	}
	return result;
}

double Predicate::sample() const
{
	if(truth_intervals_.empty())
		return 0.0;
	const Interval& first_interval = *(truth_intervals_.begin());
	if (std::isinf(first_interval.left))
		return (std::isinf(first_interval.right) ? 0 : -2.0 * std::fabs(first_interval.right));
	if (std::isinf(first_interval.right))
		return 2.0 * std::fabs(first_interval.left);
	return first_interval.left + 0.5 * (first_interval.right - first_interval.left);
}

std::ostream& operator<< (std::ostream &os, const Predicate &p) {
	os << "Predicate: ";
	for (std::vector<Interval>::const_iterator i = p.truth_intervals_.begin(); i != p.truth_intervals_.end(); i++) {
		os << (i->border & Interval::LEFT_CLOSED ? '[' : '(') << i->left
		   << " " << i->right << (i->border & Interval::RIGHT_CLOSED ? ']' : ')') << " ";
	}
	return os;
}
