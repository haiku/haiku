/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2010-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "DebuggerInterface.h"


// #pragma mark - DebuggerInterface


DebuggerInterface::~DebuggerInterface()
{
}


bool
DebuggerInterface::IsPostMortem() const
{
	// only true for core file interfaces
	return false;
}
