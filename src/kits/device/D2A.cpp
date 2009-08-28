/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <D2A.h>

BD2A::BD2A()
{
}


BD2A::~BD2A()
{
}


status_t
BD2A::Open(const char* portName)
{
	return B_ERROR;
}


void
BD2A::Close()
{
}


bool
BD2A::IsOpen()
{
	return false;
}


ssize_t
BD2A::Read(uint8* buf)
{
	return 0;
}


ssize_t
BD2A::Write(uint8 value)
{
	return 0;
}


void
BD2A::_ReservedD2A1()
{
}


void
BD2A::_ReservedD2A2()
{
}


void
BD2A::_ReservedD2A3()
{
}


