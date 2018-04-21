#pragma once

template<class TInputIterator, class T> inline
bool contains(TInputIterator first, TInputIterator last, const T& value)
{
	return std::find(first, last, value) != last;
}

template<class TContainer, class T> inline
bool contains(const TContainer& container, const T& value)
{
	return contains(std::begin(container), std::end(container), value);
}
