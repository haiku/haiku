/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Register.h"


Register::Register(int32 index, const char* name, register_format format,
	uint32 bitSize, register_type type)
	:
	fIndex(index),
	fName(name),
	fFormat(format),
	fBitSize(bitSize),
	fType(type)
{
}


Register::Register(const Register& other)
	:
	fIndex(other.fIndex),
	fName(other.fName),
	fFormat(other.fFormat),
	fBitSize(other.fBitSize),
	fType(other.fType)
{
}
