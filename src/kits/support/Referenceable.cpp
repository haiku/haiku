/*
 * Copyright 2005-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <Referenceable.h>


Referenceable::Referenceable(bool deleteWhenUnreferenced)
	: fReferenceCount(1),
	  fDeleteWhenUnreferenced(deleteWhenUnreferenced)
{
}


Referenceable::~Referenceable()
{
}


void
Referenceable::AcquireReference()
{
	if (atomic_add(&fReferenceCount, 1) == 0)
		FirstReferenceAcquired();
}


bool
Referenceable::ReleaseReference()
{
	bool unreferenced = (atomic_add(&fReferenceCount, -1) == 1);
	if (unreferenced)
		LastReferenceReleased();
	return unreferenced;
}


void
Referenceable::FirstReferenceAcquired()
{
}


void
Referenceable::LastReferenceReleased()
{
	if (fDeleteWhenUnreferenced)
		delete this;
}
