/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "VMPageQueue.h"


void
VMPageQueue::Init(const char* name, int lockingOrder)
{
	fName = name;
	fLockingOrder = lockingOrder;
	fCount = 0;
	mutex_init(&fLock, fName);
}
