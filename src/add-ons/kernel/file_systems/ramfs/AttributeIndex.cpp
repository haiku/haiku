/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "AttributeIndex.h"

// constructor
AttributeIndex::AttributeIndex(Volume *volume, const char *name, uint32 type,
							   bool fixedKeyLength, size_t keyLength)
	: Index(volume, name, type, fixedKeyLength, keyLength)
{
}

// destructor
AttributeIndex::~AttributeIndex()
{
}

