/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PAGER_H
#define PAGER_H


#include <SupportDefs.h>


class PagerTextSource {
public:
	virtual						~PagerTextSource();

	virtual	size_t				BytesAvailable() const = 0;
	virtual	size_t				Read(size_t offset, void* buffer,
									size_t size) const = 0;
};


void	pager(const PagerTextSource& textSource);


#endif	// PAGER_H
