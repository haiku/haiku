/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_FILE_LOADING_STATE_H
#define DWARF_FILE_LOADING_STATE_H


#include <Referenceable.h>
#include <String.h>


class DwarfFile;


enum dwarf_file_loading_state {
	DWARF_FILE_LOADING_STATE_INITIAL = 0,
	DWARF_FILE_LOADING_STATE_USER_INPUT_NEEDED,
	DWARF_FILE_LOADING_STATE_USER_INPUT_PROVIDED,
	DWARF_FILE_LOADING_STATE_FAILED,
	DWARF_FILE_LOADING_STATE_SUCCEEDED
};


struct DwarfFileLoadingState {
			BReference<DwarfFile>
								dwarfFile;
			BString				externalInfoFileName;
			BString				locatedExternalInfoPath;
			dwarf_file_loading_state
								state;

								DwarfFileLoadingState();
								~DwarfFileLoadingState();
};


#endif	// DWARF_FILE_LOADING_STATE_H
