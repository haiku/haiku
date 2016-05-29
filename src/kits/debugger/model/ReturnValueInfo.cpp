/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "ReturnValueInfo.h"

#include "CpuState.h"


ReturnValueInfo::ReturnValueInfo()
	:
	BReferenceable(),
	fAddress(0),
	fState(NULL)
{
}


ReturnValueInfo::ReturnValueInfo(target_addr_t address, CpuState* state)
	:
	BReferenceable(),
	fAddress(address),
	fState(state)
{
	state->AcquireReference();
}


ReturnValueInfo::~ReturnValueInfo()
{
	if (fState != NULL)
		fState->ReleaseReference();
}


void
ReturnValueInfo::SetTo(target_addr_t address, CpuState* state)
{
	fAddress = address;

	if (fState != NULL)
		fState->ReleaseReference();

	fState = state;
	fState->AcquireReference();
}
