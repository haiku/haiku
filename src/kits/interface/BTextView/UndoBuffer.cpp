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
#include "moreUTF8.h"
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
	fTypedEnd(fEnd),
	fUndone(0)
{
}


_BTypingUndoBuffer_::~_BTypingUndoBuffer_()
{
	free(fTypedText);
}


void
_BTypingUndoBuffer_::UndoSelf(BClipboard *clipboard)
{
	int32 len = fTypedEnd - fTypedStart;
	
	free(fTypedText);
	fTypedText = NULL;
	fTypedText = (char *)malloc(len);
	memcpy(fTypedText, fTextView->Text() + fTypedStart, len);
	
	fTextView->Select(fTypedStart, fTypedStart);
	fTextView->Delete(fTypedStart, fTypedEnd);
	fTextView->Insert(fTextData, fTextLength);
	fTextView->Select(fStart, fEnd);
	fUndone++;
}


void
_BTypingUndoBuffer_::RedoSelf(BClipboard *clipboard)
{	
	fTextView->Select(fTypedStart, fTypedStart);
	fTextView->Delete(fTypedStart, fTypedStart + fTextLength);
	fTextView->Insert(fTypedText, fTypedEnd - fTypedStart);
	fUndone--;
}


void
_BTypingUndoBuffer_::InputCharacter(int32 len)
{
	int32 start, end;
	fTextView->GetSelection(&start, &end);
	
	if (start != fTypedEnd || end != fTypedEnd)
		Reset();
		
	fTypedEnd += len;
}


void
_BTypingUndoBuffer_::Reset()
{
	free(fTextData);
	fTextView->GetSelection(&fStart, &fEnd);
	fTextLength = fEnd - fStart;
	fTypedStart = fStart;
	fTypedEnd = fStart;
	
	fTextData = (char *)malloc(fTextLength);
	memcpy(fTextData, fTextView->Text() + fStart, fTextLength);
	
	free(fTypedText);
	fTypedText = NULL;
	fRedo = false;
	fUndone = 0;
}


void
_BTypingUndoBuffer_::BackwardErase()
{
	int32 start, end;
	fTextView->GetSelection(&start, &end);
	
	const char *text = fTextView->Text();
	int32 charLen = UTF8PreviousCharLen(text + start, text);
	
	if (start != fTypedEnd || end != fTypedEnd) {
		Reset();
		// if we've got a selection, we're already done
		if (start != end)
			return;
	} 
	
	char *buffer = (char *)malloc(fTextLength + charLen);
	memcpy(buffer + charLen, fTextData, fTextLength);
	
	fTypedStart = start - charLen;
	start = fTypedStart;
	for (int32 x = 0; x < charLen; x++)
		buffer[x] = fTextView->ByteAt(start + x);
	free(fTextData);
	fTextData = buffer;
	
	fTextLength += charLen;
	fTypedEnd -= charLen;
}


void
_BTypingUndoBuffer_::ForwardErase()
{
	int32 start, end;

	fTextView->GetSelection(&start, &end);
	
	int32 charLen = UTF8NextCharLen(fTextView->Text() + start);	
	printf("Char Len: %d\n", charLen);

	if (start != fTypedEnd || end != fTypedEnd || fUndone > 0) {
		Reset();
		// if we've got a selection, we're already done
		if (fStart == fEnd) {
			free(fTextData);
			fTextLength = charLen;
			fTextData = (char *)malloc(fTextLength);
			
			// store the erased character
			for (int32 x = 0; x < charLen; x++)
				fTextData[x] = fTextView->ByteAt(start + x);
		}
	} else {	
		// Here we need to store the erased text, so we get the text that it's 
		// already in the buffer, and we add the erased character.
		// a realloc + memmove would maybe be cleaner, but that way we spare a
		// copy (malloc + memcpy vs realloc + memmove).
		
		int32 newLength = fTextLength + charLen;
		char *buffer = (char *)malloc(newLength);
		
		// copy the already stored data
		memcpy(buffer, fTextData, fTextLength);
		
		if (fTextLength < newLength) {
			// store the erased character
			for (int32 x = 0; x < charLen; x++)
				buffer[fTextLength] = fTextView->ByteAt(start + x);
		}

		fTextLength = newLength;
		free(fTextData);
		fTextData = buffer;
	}
}