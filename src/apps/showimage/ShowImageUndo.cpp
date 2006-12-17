/*****************************************************************************/
// ShowImageUndo
// Written by Michael Wilber
//
// ShowImageUndo.cpp
//
//
// Copyright (c) 2003 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include "ShowImageUndo.h"

#include <stdio.h>
#include <stdlib.h>

ShowImageUndo::ShowImageUndo()
{
	fWindow = NULL;
	fUndoType = 0;
	fRestore = NULL;
	fSelection = NULL;
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
			fprintf(stderr, "ShowImageUndo::SendUndoStateMessage: window must be locked!");
			exit(-1);
		}
		BMessage msg(MSG_UNDO_STATE);
		msg.AddBool("can_undo", bCanUndo);
		fWindow->PostMessage(&msg);
	}
}

void
ShowImageUndo::SetTo(BRect rect, BBitmap *restore, BBitmap *selection)
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
ShowImageUndo::Undo(BRect rect, BBitmap *restore, BBitmap *selection)
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

