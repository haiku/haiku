/*
 * Copyright 2005-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <Referenceable.h>

#include <stdio.h>
#include <OS.h>

//#define TRACE_REFERENCEABLE
#if defined(TRACE_REFERENCEABLE) && defined(_KERNEL_MODE)
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
#if !defined(_BOOT_MODE)
	if (fReferenceCount != 0 && fReferenceCount != 1) {
		char message[256];
		snprintf(message, sizeof(message), "deleting referenceable object %p with "
			"reference count (%" B_PRId32 ")", this, fReferenceCount);
		debugger(message);
	}
#endif
}


int32
BReferenceable::AcquireReference()
{
	const int32 previousReferenceCount = atomic_add(&fReferenceCount, 1);
	if (previousReferenceCount == 0)
		FirstReferenceAcquired();
#if !defined(_BOOT_MODE)
	if (previousReferenceCount < 0)
		debugger("reference count is/was negative");
#endif

	TRACE("%p: acquire %ld\n", this, fReferenceCount);

	return previousReferenceCount;
}


int32
BReferenceable::ReleaseReference()
{
	const int32 previousReferenceCount = atomic_add(&fReferenceCount, -1);
	TRACE("%p: release %ld\n", this, fReferenceCount);
	if (previousReferenceCount == 1)
		LastReferenceReleased();
#if !defined(_BOOT_MODE)
	if (previousReferenceCount <= 0)
		debugger("reference count is negative");
#endif
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
