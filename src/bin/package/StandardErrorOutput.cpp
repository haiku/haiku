/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "StandardErrorOutput.h"

#include <stdio.h>


void
StandardErrorOutput::PrintErrorVarArgs(const char* format, va_list args)
{
	vfprintf(stderr, format, args);
}
