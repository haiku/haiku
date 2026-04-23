/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "VMPageQueue.h"


// #pragma mark - VMPageQueue


void
VMPageQueue::Init()
{
	new(&fPages) PageList;

	B_INITIALIZE_SPINLOCK(&fLock);

	fCount = 0;
}
