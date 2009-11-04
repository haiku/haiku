/*
 * Copyright 2005-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <Referenceable.h>


BReferenceable::BReferenceable(bool deleteWhenUnreferenced)
	: fReferenceCount(1),
	  fDeleteWhenUnreferenced(deleteWhenUnreferenced)
{
}


BReferenceable::~BReferenceable()
{
}


void
BReferenceable::AcquireReference()
{
	if (atomic_add(&fReferenceCount, 1) == 0)
		FirstReferenceAcquired();
}


bool
BReferenceable::ReleaseReference()
{
	bool unreferenced = (atomic_add(&fReferenceCount, -1) == 1);
	if (unreferenced)
		LastReferenceReleased();
	return unreferenced;
}


void
BReferenceable::FirstReferenceAcquired()
{
}


void
BReferenceable::LastReferenceReleased()
{
	if (fDeleteWhenUnreferenced)
		delete this;
}
