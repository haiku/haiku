/*
 * Copyright 2005-2007, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Referenceable.h>


// constructor
Referenceable::Referenceable(bool deleteWhenUnreferenced)
	: fReferenceCount(1),
	  fDeleteWhenUnreferenced(deleteWhenUnreferenced)
{
}

// destructor
Referenceable::~Referenceable()
{
}

// RemoveReference
bool
Referenceable::RemoveReference()
{
	bool unreferenced = (atomic_add(&fReferenceCount, -1) == 1);
	if (fDeleteWhenUnreferenced && unreferenced)
		delete this;
	return unreferenced;
}
