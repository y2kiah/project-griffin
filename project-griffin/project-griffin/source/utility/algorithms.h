#pragma once
#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <utility>
#include <algorithm>

/**	
* move a range selection in a container to another location in the container
* return the new range of the selection
*/
template<typename I> // I models RandomAccessIterator
auto slide(I f, I l, I p) -> std::pair<I, I>
{
	if (p < f) { return { p, std::rotate(p, f, l) }; }
	if (p > f) { return { std::rotate(f, l, p), p }; }
	return {f, l};
}

/**
* gather a set of disjointed selections together to a location in the container
* return the new range of the selection
*/
template<typename I, // I models BidirectionalIterator
		 typename S> // S models UnaryPredicate
auto gather(I f, I l, I p, S s) -> std::pair<I, I>
{
	return {
		std::stable_partition(f, p, std::not1(s)),
		std::stable_partition(p, l, s)
	};
}

#endif