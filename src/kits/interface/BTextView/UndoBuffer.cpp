//------------------------------------------------------------------------------
//	Copyright (c) 2003, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		UndoBuffer.cpp
//	Author:			Stefano Ceccherini (burton666@libero.it)
//	Description:	_BUndoBuffer_ and its subclasses 
//					handle different types of Undo operations.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <cstdio>
#include <cstdlib>
#include <cstring>

// System Includes -------------------------------------------------------------
#include <Clipboard.h>

// Local Includes --------------------------------------------------------------
#include "UndoBuffer.h"


//  ******** _BUndoBuffer_ *******
_BUndoBuffer_::_BUndoBuffer_(BTextView *textView, undo_state state)
	:
	fTextView(textView),
	fTextData(NULL),
	fRunArray(NULL),
	fRunArrayLength(0),
	fRedo(false),
	fState(state)
{
	fTextView->GetSelection(&fStart, &fEnd);
	fTextLength = fEnd - fStart;
	
	fTextData = (char *)malloc(fTextLength);
	memcpy(fTextData, fTextView->Text() + fStart, fTextLength);

	if (fTextView->IsStylable())
		fRunArray =  fTextView->RunArray(fStart, fEnd, &fRunArrayLength);
}


_BUndoBuffer_::~_BUndoBuffer_()
{
	free(fTextData);
	free(fRunArray);
}


void
_BUndoBuffer_::Undo(BClipboard *clipboard)
{
	fRedo ? RedoSelf(clipboard) : UndoSelf(clipboard);
		
	fRedo = !fRedo;
}


undo_state
_BUndoBuffer_::State(bool *redo)
{
	*redo = fRedo;

	return fState;
}


void
_BUndoBuffer_::UndoSelf(BClipboard *clipboard)
{
	fTextView->Select(fStart, fStart);
	fTextView->Insert(fTextData, fTextLength, fRunArray);
	fTextView->Select(fStart, fStart);
}


void
_BUndoBuffer_::RedoSelf(BClipboard *clipboard)
{
}


//  ******** _BCutUndoBuffer_ *******
_BCutUndoBuffer_::_BCutUndoBuffer_(BTextView *textView)
	:
	_BUndoBuffer_(textView, B_UNDO_CUT)
{
}


_BCutUndoBuffer_::~_BCutUndoBuffer_()
{
}


void 
_BCutUndoBuffer_::RedoSelf(BClipboard *clipboard)
{
	BMessage *clip = NULL;
	
	fTextView->Select(fStart, fStart);
	fTextView->Delete(fStart, fEnd);
	if (clipboard->Lock()) {
		clipboard->Clear();
		if ((clip = clipboard->Data())) {
			clip->AddData("text/plain", B_MIME_TYPE, fTextData, fTextLength);
			if (fRunArray)
				clip->AddData("application/x-vnd.Be-text_run_array", B_MIME_TYPE,
					fRunArray, fRunArrayLength);
			clipboard->Commit();
		}
		clipboard->Unlock();
	}
}


//  ******** _BPasteUndoBuffer_ *******
_BPasteUndoBuffer_::_BPasteUndoBuffer_(BTextView *textView, const char *text,
										int32 textLen, text_run_array *runArray, int32 runArrayLen)
	:
	_BUndoBuffer_(textView, B_UNDO_PASTE),
	fPasteText(NULL),
	fPasteTextLength(textLen),
	fPasteRunArray(NULL),
	fPasteRunArrayLength(0)
{
	fPasteText = (char *)malloc(fPasteTextLength);
	memcpy(fPasteText, text, fPasteTextLength);

	if (runArray) {
		fPasteRunArray = (text_run_array *)malloc(runArrayLen);
		fPasteRunArrayLength = runArrayLen;
		memcpy(fPasteRunArray, runArray, runArrayLen);
	}
}


_BPasteUndoBuffer_::~_BPasteUndoBuffer_()
{
	free(fPasteText);
	free(fPasteRunArray);
}


void
_BPasteUndoBuffer_::UndoSelf(BClipboard *clipboard)
{
	fTextView->Select(fStart, fStart);
	fTextView->Delete(fStart, fStart + fPasteTextLength);
	fTextView->Insert(fTextData, fTextLength, fRunArray);
	fTextView->Select(fStart, fEnd);
}


void
_BPasteUndoBuffer_::RedoSelf(BClipboard *clipboard)
{
	fTextView->Select(fStart, fStart);
	fTextView->Delete(fStart, fEnd);
	fTextView->Insert(fPasteText, fPasteTextLength, fPasteRunArray);
	fTextView->Select(fStart + fPasteTextLength, fStart + fPasteTextLength);
}


//  ******** _BClearUndoBuffer_ *******
_BClearUndoBuffer_::_BClearUndoBuffer_(BTextView *textView)
	:
	_BUndoBuffer_(textView, B_UNDO_CLEAR)
{
}


_BClearUndoBuffer_::~_BClearUndoBuffer_()
{
}


void
_BClearUndoBuffer_::RedoSelf(BClipboard *clipboard)
{
	fTextView->Select(fStart, fStart);
	fTextView->Delete(fStart, fEnd);
}


//  ******** _BDropUndoBuffer_ ********
_BDropUndoBuffer_::_BDropUndoBuffer_(BTextView *textView, char const *text, int32 textLen,
									text_run_array *runArray, int32 runArrayLen, int32 location,
									bool internalDrop)
	:
	_BUndoBuffer_(textView, B_UNDO_DROP),
	fDropText(NULL),
	fDropTextLength(textLen),
	fDropRunArray(NULL)
{
	fInternalDrop = internalDrop;
	fDropLocation = location;

	fDropText = (char *)malloc(fDropTextLength);
	memcpy(fDropText, text, fDropTextLength);

	if (runArray) {
		fDropRunArray = (text_run_array *)malloc(runArrayLen);
		fDropRunArrayLength = runArrayLen;
		memcpy(fDropRunArray, runArray, runArrayLen);
	}
}


_BDropUndoBuffer_::~_BDropUndoBuffer_()
{
	free(fDropText);
	free(fDropRunArray);
}


void
_BDropUndoBuffer_::UndoSelf(BClipboard *)
{
	fTextView->Select(fDropLocation, fDropLocation);
	fTextView->Delete(fDropLocation, fDropLocation + fDropTextLength);
	if (fInternalDrop) {
		fTextView->Select(fStart, fStart);
		fTextView->Insert(fTextData, fTextLength, fRunArray);
	}
	fTextView->Select(fStart, fEnd);
}


void
_BDropUndoBuffer_::RedoSelf(BClipboard *)
{
	if (fInternalDrop) {
		fTextView->Select(fStart, fStart);
		fTextView->Delete(fStart, fEnd);
	}
	fTextView->Select(fDropLocation, fDropLocation);
	fTextView->Insert(fDropText, fDropTextLength, fDropRunArray);
	fTextView->Select(fDropLocation, fDropLocation + fDropTextLength);
}


//  ******** _BTypingUndoBuffer_ ********
_BTypingUndoBuffer_::_BTypingUndoBuffer_(BTextView *textView)
	:
	_BUndoBuffer_(textView, B_UNDO_TYPING),
	fTypedText(NULL),
	fTypedStart(fStart),
	fNumBytes(0)
{
}


_BTypingUndoBuffer_::~_BTypingUndoBuffer_()
{
	free(fTypedText);
}


void
_BTypingUndoBuffer_::UndoSelf(BClipboard *clipboard)
{
	free(fTypedText);
	fTypedText = (char *)malloc(fNumBytes);
	memcpy(fTypedText, fTextView->Text() + fTypedStart, fNumBytes);
	
	fTextView->Select(fTypedStart, fTypedStart);
	fTextView->Delete(fTypedStart, fTypedStart + fNumBytes);
	fTextView->Insert(fTextData, fTextLength);
	fTextView->Select(fStart, fEnd);
}


void
_BTypingUndoBuffer_::RedoSelf(BClipboard *clipboard)
{	
	fTextView->Select(fTypedStart, fTypedStart);
	fTextView->Delete(fTypedStart, fTypedStart + fTextLength);
	fTextView->Insert(fTypedText, fNumBytes);
}


void
_BTypingUndoBuffer_::InputCharacter(int32 len)
{
	int32 start, end;
	fTextView->GetSelection(&start, &end);
	
	int32 currentPos = fTypedStart + fNumBytes;
	if (start != currentPos)
		Reset();
		
	fNumBytes += len;
}


void
_BTypingUndoBuffer_::Reset()
{
	free(fTextData);
	fTextView->GetSelection(&fStart, &fEnd);
	fTextLength = fEnd - fStart;
	fTypedStart = fStart;
	fNumBytes = 0;
	
	fTextData = (char *)malloc(fTextLength);
	memcpy(fTextData, fTextView->Text() + fStart, fTextLength);
	
	free(fTypedText);
	fTypedText = NULL;
	fRedo = false;
}


void
_BTypingUndoBuffer_::ForwardErase()
{
	//TODO: Fix this. Currently, it just resets the
	//undobuffer every time DELETE is pressed, while
	//it should reset it just the first time it's pressed.
	//Moreover, it doesn't work with multi-byte characters.
	int32 start, end;
	fTextView->GetSelection(&start, &end);
	
	int32 currentPos = fTypedStart + fNumBytes;
	if (start != currentPos)
		Reset();
}


void
_BTypingUndoBuffer_::BackwardErase()
{
	//TODO: Fix this. Currently, it just resets the
	//undobuffer every time BACKSPACE is pressed, while
	//it should reset it just the first time it's pressed.
	//Moreover, it doesn't work with multi-byte characters.
	int32 start, end;
	fTextView->GetSelection(&start, &end);
	
	int32 currentPos = fTypedStart + fNumBytes;
	if (start != currentPos)
		Reset();
}
