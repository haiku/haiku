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


BReferenceable::BReferenceable()
	:
	fReferenceCount(1)
{
}


BReferenceable::~BReferenceable()
{
}


int32
BReferenceable::AcquireReference()
{
	int32 previousReferenceCount = atomic_add(&fReferenceCount, 1);
	if (previousReferenceCount == 0)
		FirstReferenceAcquired();

	TRACE("%p: acquire %ld\n", this, fReferenceCount);

	return previousReferenceCount;
}


int32
BReferenceable::ReleaseReference()
{
	int32 previousReferenceCount = atomic_add(&fReferenceCount, -1);
	TRACE("%p: release %ld\n", this, fReferenceCount);
	if (previousReferenceCount == 1)
		LastReferenceReleased();
	return previousReferenceCount;
}


void
BReferenceable::FirstReferenceAcquired()
{
}


void
BReferenceable::LastReferenceReleased()
{
	delete this;
}
