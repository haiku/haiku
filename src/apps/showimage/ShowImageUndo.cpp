/*
 * Copyright 2003-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Wilber
 */

#include "ShowImageUndo.h"

#include <stdio.h>
#include <stdlib.h>


ShowImageUndo::ShowImageUndo()
	:
	fWindow(NULL),
	fUndoType(0),
	fRestore(NULL),
	fSelection(NULL)
{
}


ShowImageUndo::~ShowImageUndo()
{
	InternalClear();
}


void
ShowImageUndo::InternalClear()
{
	fUndoType = 0;
	delete fRestore;
	fRestore = NULL;
	delete fSelection;
	fSelection = NULL;
}


void
ShowImageUndo::Clear()
{
	InternalClear();	
	SendUndoStateMessage(false);
}


void
ShowImageUndo::SendUndoStateMessage(bool bCanUndo)
{
	if (fWindow) {
		if (!fWindow->IsLocked()) {
			fprintf(stderr,
				"ShowImageUndo::SendUndoStateMessage: window must be locked!");
			exit(-1);
		}
		BMessage msg(MSG_UNDO_STATE);
		msg.AddBool("can_undo", bCanUndo);
		fWindow->PostMessage(&msg);
	}
}


void
ShowImageUndo::SetTo(BRect rect, BBitmap* restore, BBitmap* selection)
{
	// NOTE: THIS FUNCTION DOES NOT MAKE COPIES OF THE BITMAPS PASSED TO IT
	InternalClear();
	
	fUndoType = UNDO_UNDO;
	fRect = rect;
	fRestore = restore;
	fSelection = selection;
	
	SendUndoStateMessage(true);
}


void
ShowImageUndo::Undo(BRect rect, BBitmap* restore, BBitmap* selection)
{
	// NOTE: THIS FUNCTION DOES NOT MAKE COPIES OF THE BITMAPS PASSED TO IT
	fUndoType = UNDO_REDO;
	fRect = rect;
	delete fRestore;
	fRestore = restore;
	fSelection = selection;
		// NOTE: fSelection isn't deleted here because ShowImageView
		// takes ownership of it during an Undo
}

