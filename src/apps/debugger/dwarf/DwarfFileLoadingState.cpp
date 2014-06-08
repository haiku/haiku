/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "DwarfFileLoadingState.h"

#include "DwarfFile.h"


DwarfFileLoadingState::DwarfFileLoadingState()
	:
	dwarfFile(),
	externalInfoFileName(),
	locatedExternalInfoPath(),
	state(DWARF_FILE_LOADING_STATE_INITIAL)
{
}


DwarfFileLoadingState::~DwarfFileLoadingState()
{
}


