/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef LINE_DATA_SOURCE_H
#define LINE_DATA_SOURCE_H


#include <Referenceable.h>


class LineDataSource : public BReferenceable {
public:
	virtual						~LineDataSource();

	virtual	int32				CountLines() const = 0;
	virtual	const char*			LineAt(int32 index) const = 0;
	virtual int32				LineLengthAt(int32 index) const = 0;
};


#endif	// LINE_DATA_SOURCE_H
