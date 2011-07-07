/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef INDEX_IMPL_H
#define INDEX_IMPL_H


#include "Index.h"
#include "Node.h"


class AbstractIndexIterator {
public:
								AbstractIndexIterator();
	virtual						~AbstractIndexIterator();

	virtual	bool				HasNext() const = 0;
	virtual Node*				Next(void* buffer, size_t* _keyLength) = 0;

	virtual	status_t			Suspend();
	virtual	status_t			Resume();
};


#endif	// INDEX_IMPL_H
