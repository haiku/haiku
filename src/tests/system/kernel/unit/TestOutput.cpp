/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TestOutput.h"

#include <stdio.h>

#include <util/AutoLock.h>


// #pragma mark - TestOutput


TestOutput::TestOutput()
{
}


TestOutput::~TestOutput()
{
}


// #pragma mark - DebugTestOutput


DebugTestOutput::DebugTestOutput()
{
	B_INITIALIZE_SPINLOCK(&fLock);
}


DebugTestOutput::~DebugTestOutput()
{
}


int
DebugTestOutput::PrintArgs(const char* format, va_list args)
{
	InterruptsSpinLocker locker(fLock);

	int bytes = vsnprintf(fBuffer, sizeof(fBuffer), format, args);
	dprintf("%s", fBuffer);

	return bytes;
}
