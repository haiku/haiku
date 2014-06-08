/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "DwarfImageDebugInfoLoadingState.h"


DwarfImageDebugInfoLoadingState::DwarfImageDebugInfoLoadingState()
	:
	SpecificImageDebugInfoLoadingState(),
	fState()
{
}


DwarfImageDebugInfoLoadingState::~DwarfImageDebugInfoLoadingState()
{
}


bool
DwarfImageDebugInfoLoadingState::UserInputRequired() const
{
	return fState.state == DWARF_FILE_LOADING_STATE_USER_INPUT_NEEDED;
}
