/*
 * Copyright 2005-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <Referenceable.h>

#ifdef _KERNEL_MODE
#	include "kernel_debug_config.h"
#	if PARANOID_KERNEL_FREE && !defined(DEBUG)
#		define DEBUG
#	endif
#endif

#ifdef DEBUG
#include <stdio.h>
#endif

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
#if !defined(_BOOT_MODE) && defined(DEBUG)
	if (fReferenceCount != 0 && fReferenceCount != 1) {
		char message[256];
		snprintf(message, sizeof(message), "deleting referenceable object %p with "
			"reference count (%" B_PRId32 ")\n", this, fReferenceCount);
		debugger(message);
	}
#endif
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
