/*
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef COLLECTOR_H
#define COLLECTOR_H


#include <vector>


template <typename T>
class Collector {
public:
	virtual void Add(T value) = 0;
};


template <typename T>
class VectorCollector : public Collector<T> {
public:
	VectorCollector(std::vector<T>& target)
		:
		fTarget(target)
	{
	}

	virtual void Add(T value) {
		fTarget.push_back(value);
	}

private:
	std::vector<T>&	fTarget;
};


#endif // COLLECTOR_H
