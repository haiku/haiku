/*
 * Copyright 2005-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <Referenceable.h>

#ifdef DEBUG
#include <stdio.h>
#endif

#include <debugger.h>

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
#ifdef DEBUG
	bool enterDebugger = false;
	char message[256];
	if (fReferenceCount == 1) {
		// Simple heuristic to test if this object was allocated
		// on the stack: check if this is within 1KB in either
		// direction of the current stack address, and the reference
		// count is 1. If so, we don't flag a warning since that would
		// imply the object was allocated/destroyed on the stack
		// without any references being acquired or released.
		char test;
		ssize_t testOffset = (addr_t)this - (addr_t)&test;
		if (testOffset > 1024 || -testOffset > 1024) {
			// might still be a stack object, check the thread's
			// stack range to be sure.
			thread_info info;
			status_t result = get_thread_info(find_thread(NULL), &info);
			if (result != B_OK || this < info.stack_base
				|| this > info.stack_end) {
				snprintf(message, sizeof(message), "Deleted referenceable "
					"object that's not on the stack (this: %p, stack_base: %p,"
					" stack_end: %p)\n", this, info.stack_base,
					info.stack_end);
				enterDebugger = true;
			}
		}
	} else if (fReferenceCount != 0) {
		snprintf(message, sizeof(message), "Deleted referenceable object %p with "
			"non-zero reference count (%" B_PRId32 ")\n", this, fReferenceCount);
		enterDebugger = true;
	}

	if (enterDebugger)
		debugger(message);
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
