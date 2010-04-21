/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "IOCallback.h"


IOCallback::~IOCallback()
{
}


status_t
IOCallback::DoIO(IOOperation* operation)
{
	return B_ERROR;
}


/*static*/ status_t
IOCallback::WrapperFunction(void* data, io_operation* operation)
{
	return ((IOCallback*)data)->DoIO(operation);
}
