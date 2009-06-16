/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Thread.h"


Thread::Thread(thread_id threadID)
	:
	fID(threadID)
{
}


Thread::~Thread()
{
}


status_t
Thread::Init()
{
	return B_OK;
}


void
Thread::SetName(const BString& name)
{
	fName = name;
}
