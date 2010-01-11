/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "VMPageQueue.h"


// #pragma mark - VMPageQueue


void
VMPageQueue::Init(const char* name)
{
	new(&fPages) PageList;

	B_INITIALIZE_SPINLOCK(&fLock);

	fName = name;
	fCount = 0;
}
