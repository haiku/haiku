/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_IMAGE_DEBUG_INFO_LOADING_STATE_H
#define DWARF_IMAGE_DEBUG_INFO_LOADING_STATE_H


#include "DwarfFileLoadingState.h"
#include "SpecificImageDebugInfoLoadingState.h"


class DwarfImageDebugInfoLoadingState
	: public SpecificImageDebugInfoLoadingState {
public:
								DwarfImageDebugInfoLoadingState();
	virtual						~DwarfImageDebugInfoLoadingState();

	virtual	bool				UserInputRequired() const;

			DwarfFileLoadingState& GetFileState()
									{ return fState; }

private:
			DwarfFileLoadingState fState;
};


#endif // DWARF_IMAGE_DEBUG_INFO_LOADING_STATE_H

