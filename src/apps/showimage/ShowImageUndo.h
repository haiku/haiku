/*****************************************************************************/
// ShowImageUndo
// Written by Michael Wilber
//
// ShowImageUndo.h
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

#ifndef _ShowImageUndo_h
#define _ShowImageUndo_h

#include <Rect.h>
#include <Bitmap.h>
#include <Window.h>
#include <Message.h>
#include <Messenger.h>
#include "ShowImageConstants.h"

// for Undo
#define UNDO_UNDO 1
#define UNDO_REDO 2

class ShowImageUndo {
public:
	ShowImageUndo();
	~ShowImageUndo();
	
	void SetWindow(BWindow *win) { fWindow = win; }
	
	void Clear();
	
	// NOTE: THESE TWO FUNCTIONS DO NOT MAKE COPIES OF THE
	// BITMAPS PASSED TO THEM
	void SetTo(BRect rect, BBitmap *restore, BBitmap *selection);
	void Undo(BRect rect, BBitmap *restore, BBitmap *selection);
	
	int32 GetType() { return fUndoType; }
	BRect GetRect() { return fRect; }
	BBitmap *GetRestoreBitmap() { return fRestore; }
	BBitmap *GetSelectionBitmap() { return fSelection; }
	
private:
	void InternalClear();
	void SendUndoStateMessage(bool bCanUndo);
	
	BWindow *fWindow;
		// Window to which notification messages are sent
	int32 fUndoType;
		// Nothing, Undo or Redo
	BRect fRect;
		// Area of background bitmap where change took place
	BBitmap *fRestore;
		// Changed portion of background bitmap, before change took place
	BBitmap *fSelection;
		// Selection present before change took place
};

#endif /* _ShowImageUndo_h */

