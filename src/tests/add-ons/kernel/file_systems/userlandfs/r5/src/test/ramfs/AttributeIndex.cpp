// AttributeIndex.cpp

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

