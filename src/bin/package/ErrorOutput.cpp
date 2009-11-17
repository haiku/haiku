/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ErrorOutput.h"

#include <stdio.h>


ErrorOutput::~ErrorOutput()
{
}


void
ErrorOutput::PrintError(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	PrintErrorVarArgs(format, args);
	va_end(args);
}
