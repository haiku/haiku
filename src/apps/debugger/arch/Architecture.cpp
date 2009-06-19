/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Architecture.h"


Architecture::Architecture(DebuggerInterface* debuggerInterface)
	:
	fDebuggerInterface(debuggerInterface)
{
}


Architecture::~Architecture()
{
}


status_t
Architecture::Init()
{
	return B_OK;
}
