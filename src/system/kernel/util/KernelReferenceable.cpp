/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <util/KernelReferenceable.h>

#include <int.h>


void
KernelReferenceable::LastReferenceReleased()
{
	if (are_interrupts_enabled())
		delete this;
	else
		deferred_delete(this);
}
