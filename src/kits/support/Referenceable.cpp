/*
 * Copyright 2005-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <Referenceable.h>


//#define TRACE_REFERENCEABLE
#ifdef TRACE_REFERENCEABLE
#	include <tracing.h>
#	define TRACE(x, ...) ktrace_printf(x, __VA_ARGS__);
#else
#	define TRACE(x, ...)
#endif


BReferenceable::BReferenceable(bool deleteWhenUnreferenced)
	:
	fReferenceCount(1),
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

	TRACE("%p: acquire %ld\n", this, fReferenceCount);
}


bool
BReferenceable::ReleaseReference()
{
	bool unreferenced = (atomic_add(&fReferenceCount, -1) == 1);
	TRACE("%p: release %ld\n", this, fReferenceCount);
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
