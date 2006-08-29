/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Referenceable.h"

#define TRACE 0

#if TRACE
#define ICON 1
#include <debugger.h>
#include <stdio.h>

#if ICON
#include "IconObject.h"
#endif
#endif

// constructor
Referenceable::Referenceable()
	: fReferenceCount(1)
{
}

// destructor
Referenceable::~Referenceable()
{
}

// Acquire
void
Referenceable::Acquire()
{
	atomic_add(&fReferenceCount, 1);
}

// Release
bool
Referenceable::Release()
{
#if TRACE
	int32 old = atomic_add(&fReferenceCount, -1);
//#if ICON
//	if (old > 1) {
//IconObject* object = dynamic_cast<IconObject*>(this);
//printf("Referenceable::Release() - %s: %ld\n",
//	object ? object->Name() : "unkown", fReferenceCount);
//	} else
//#endif
	if (old == 1) {
#if ICON
IconObject* object = dynamic_cast<IconObject*>(this);
printf("Referenceable::Release() - deleting %s\n",
	object ? object->Name() : "unkown");
#else
printf("Referenceable::Release() - deleting\n");
#endif
		delete this;
		return true;
	} else if (old < 1)
		debugger("Referenceable::Release() - already deleted");
#else
	if (atomic_add(&fReferenceCount, -1) == 1) {
		delete this;
		return true;
	}
#endif

	return false;
}

