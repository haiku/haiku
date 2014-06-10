/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "DwarfLoadingStateHandler.h"

#include <Entry.h>
#include <Path.h>

#include "DwarfFile.h"
#include "DwarfImageDebugInfoLoadingState.h"
#include "Tracing.h"
#include "UserInterface.h"


DwarfLoadingStateHandler::DwarfLoadingStateHandler()
	:
	ImageDebugLoadingStateHandler()
{
}


DwarfLoadingStateHandler::~DwarfLoadingStateHandler()
{
}


bool
DwarfLoadingStateHandler::SupportsState(
	SpecificImageDebugInfoLoadingState* state)
{
	return dynamic_cast<DwarfImageDebugInfoLoadingState*>(state) != NULL;
}


void
DwarfLoadingStateHandler::HandleState(
	SpecificImageDebugInfoLoadingState* state, UserInterface* interface)
{
	DwarfImageDebugInfoLoadingState* dwarfState
		= dynamic_cast<DwarfImageDebugInfoLoadingState*>(state);

	if (dwarfState == NULL) {
		ERROR("DwarfLoadingStateHandler::HandleState() passed "
			"non-dwarf state object %p.", state);
		return;
	}

	DwarfFileLoadingState& fileState = dwarfState->GetFileState();

	// TODO: if the image in question is packaged, query if it has a
	// corresponding debug info package. If so, offer to install it and
	// then locate the file automatically rather than forcing the user to
	// locate the file manually.
	BString message;
	message.SetToFormat("The debug information file '%s' for image '%s' is "
		"missing. Please locate it if possible.",
		fileState.externalInfoFileName.String(), fileState.dwarfFile->Name());
	int32 choice = interface->SynchronouslyAskUser("Debug info missing",
		message.String(), "Locate", "Skip", NULL);

	if (choice == 0) {
		entry_ref ref;
		interface->SynchronouslyAskUserForFile(&ref);
		BPath path(&ref);
		if (path.InitCheck() == B_OK)
			fileState.locatedExternalInfoPath = path.Path();
	}

	fileState.state = DWARF_FILE_LOADING_STATE_USER_INPUT_PROVIDED;
}
