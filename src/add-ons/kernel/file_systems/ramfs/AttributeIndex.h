/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ATTRIBUTE_INDEX_H
#define ATTRIBUTE_INDEX_H

#include "Index.h"

class Attribute;

class AttributeIndex : public Index {
public:
	AttributeIndex(Volume *volume, const char *name, uint32 type,
				   bool fixedKeyLength, size_t keyLength = 0);
	virtual ~AttributeIndex();

	virtual status_t Added(Attribute *attribute) = 0;
	virtual bool Removed(Attribute *attribute) = 0;
	virtual status_t Changed(Attribute *attribute,
							 const uint8 *oldKey, size_t length) = 0;
};

#endif	// ATTRIBUTE_INDEX_H
