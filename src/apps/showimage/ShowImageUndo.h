/*
 * Copyright 2003-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Wilber
 */
#ifndef SHOW_IMAGE_UNDO_H
#define SHOW_IMAGE_UNDO_H


#include <Bitmap.h>
#include <Message.h>
#include <Messenger.h>
#include <Rect.h>
#include <Window.h>

#include "ShowImageConstants.h"


// for Undo
#define UNDO_UNDO 1
#define UNDO_REDO 2


class ShowImageUndo {
public:
				ShowImageUndo();
				~ShowImageUndo();
	
	void		SetWindow(BWindow* win) { fWindow = win; }
	
	void		Clear();
	
	// NOTE: THESE TWO FUNCTIONS DO NOT MAKE COPIES OF THE
	// BITMAPS PASSED TO THEM
	void		SetTo(BRect rect, BBitmap* restore, BBitmap* selection);
	void		Undo(BRect rect, BBitmap* restore, BBitmap* selection);
	
	int32		GetType() { return fUndoType; }
	BRect		GetRect() { return fRect; }
	BBitmap*	GetRestoreBitmap() { return fRestore; }
	BBitmap*	GetSelectionBitmap() { return fSelection; }
	
private:
	void		InternalClear();
	void		SendUndoStateMessage(bool bCanUndo);
	
	BWindow*	fWindow;
		// Window to which notification messages are sent
	int32		fUndoType;
		// Nothing, Undo or Redo
	BRect		fRect;
		// Area of background bitmap where change took place
	BBitmap* 	fRestore;
		// Changed portion of background bitmap, before change took place
	BBitmap*	fSelection;
		// Selection present before change took place
};


#endif	// SHOW_IMAGE_UNDO_H

