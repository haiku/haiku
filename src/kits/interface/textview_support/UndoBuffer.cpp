/*
 * Copyright 2003-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 */

//!	UndoBuffer and its subclasses handle different types of Undo operations.


#include "UndoBuffer.h"
#include "utf8_functions.h"

#include <Clipboard.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// TODO: properly document this file


//	#pragma mark - UndoBuffer


BTextView::UndoBuffer::UndoBuffer(BTextView* textView, undo_state state)
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
	
	fTextData = (char*)malloc(fTextLength);
	memcpy(fTextData, fTextView->Text() + fStart, fTextLength);

	if (fTextView->IsStylable())
		fRunArray = fTextView->RunArray(fStart, fEnd, &fRunArrayLength);
}


BTextView::UndoBuffer::~UndoBuffer()
{
	free(fTextData);
	BTextView::FreeRunArray(fRunArray);
}


void
BTextView::UndoBuffer::Undo(BClipboard* clipboard)
{
	fRedo ? RedoSelf(clipboard) : UndoSelf(clipboard);
		
	fRedo = !fRedo;
}


undo_state
BTextView::UndoBuffer::State(bool* _isRedo) const
{
	*_isRedo = fRedo;

	return fState;
}


void
BTextView::UndoBuffer::UndoSelf(BClipboard* clipboard)
{
	fTextView->Select(fStart, fStart);
	fTextView->Insert(fTextData, fTextLength, fRunArray);
	fTextView->Select(fStart, fStart);
}


void
BTextView::UndoBuffer::RedoSelf(BClipboard* clipboard)
{
}


//	#pragma mark - CutUndoBuffer


BTextView::CutUndoBuffer::CutUndoBuffer(BTextView* textView)
	: BTextView::UndoBuffer(textView, B_UNDO_CUT)
{
}


BTextView::CutUndoBuffer::~CutUndoBuffer()
{
}


void 
BTextView::CutUndoBuffer::RedoSelf(BClipboard* clipboard)
{
	BMessage* clip = NULL;
	
	fTextView->Select(fStart, fStart);
	fTextView->Delete(fStart, fEnd);
	if (clipboard->Lock()) {
		clipboard->Clear();
		if ((clip = clipboard->Data())) {
			clip->AddData("text/plain", B_MIME_TYPE, fTextData, fTextLength);
			if (fRunArray)
				clip->AddData("application/x-vnd.Be-text_run_array",
					B_MIME_TYPE, fRunArray, fRunArrayLength);
			clipboard->Commit();
		}
		clipboard->Unlock();
	}
}


//	#pragma mark - PasteUndoBuffer


BTextView::PasteUndoBuffer::PasteUndoBuffer(BTextView* textView,
		const char* text, int32 textLen, text_run_array* runArray,
		int32 runArrayLen)
	: BTextView::UndoBuffer(textView, B_UNDO_PASTE),
	fPasteText(NULL),
	fPasteTextLength(textLen),
	fPasteRunArray(NULL)
{
	fPasteText = (char*)malloc(fPasteTextLength);
	memcpy(fPasteText, text, fPasteTextLength);

	if (runArray)
		fPasteRunArray = BTextView::CopyRunArray(runArray);
}


BTextView::PasteUndoBuffer::~PasteUndoBuffer()
{
	free(fPasteText);
	BTextView::FreeRunArray(fPasteRunArray);
}


void
BTextView::PasteUndoBuffer::UndoSelf(BClipboard* clipboard)
{
	fTextView->Select(fStart, fStart);
	fTextView->Delete(fStart, fStart + fPasteTextLength);
	fTextView->Insert(fTextData, fTextLength, fRunArray);
	fTextView->Select(fStart, fEnd);
}


void
BTextView::PasteUndoBuffer::RedoSelf(BClipboard* clipboard)
{
	fTextView->Select(fStart, fStart);
	fTextView->Delete(fStart, fEnd);
	fTextView->Insert(fPasteText, fPasteTextLength, fPasteRunArray);
	fTextView->Select(fStart + fPasteTextLength, fStart + fPasteTextLength);
}


//	#pragma mark - ClearUndoBuffer


BTextView::ClearUndoBuffer::ClearUndoBuffer(BTextView* textView)
	: BTextView::UndoBuffer(textView, B_UNDO_CLEAR)
{
}


BTextView::ClearUndoBuffer::~ClearUndoBuffer()
{
}


void
BTextView::ClearUndoBuffer::RedoSelf(BClipboard* clipboard)
{
	fTextView->Select(fStart, fStart);
	fTextView->Delete(fStart, fEnd);
}


//	#pragma mark - DropUndoBuffer


BTextView::DropUndoBuffer::DropUndoBuffer(BTextView* textView,
		char const* text, int32 textLen, text_run_array* runArray,
		int32 runArrayLen, int32 location, bool internalDrop)
	: BTextView::UndoBuffer(textView, B_UNDO_DROP),
	fDropText(NULL),
	fDropTextLength(textLen),
	fDropRunArray(NULL)
{
	fInternalDrop = internalDrop;
	fDropLocation = location;

	fDropText = (char*)malloc(fDropTextLength);
	memcpy(fDropText, text, fDropTextLength);

	if (runArray)
		fDropRunArray = BTextView::CopyRunArray(runArray);

	if (fInternalDrop && fDropLocation >= fEnd)
		fDropLocation -= fDropTextLength;
}


BTextView::DropUndoBuffer::~DropUndoBuffer()
{
	free(fDropText);
	BTextView::FreeRunArray(fDropRunArray);
}


void
BTextView::DropUndoBuffer::UndoSelf(BClipboard* )
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
BTextView::DropUndoBuffer::RedoSelf(BClipboard* )
{
	if (fInternalDrop) {
		fTextView->Select(fStart, fStart);
		fTextView->Delete(fStart, fEnd);
	}
	fTextView->Select(fDropLocation, fDropLocation);
	fTextView->Insert(fDropText, fDropTextLength, fDropRunArray);
	fTextView->Select(fDropLocation, fDropLocation + fDropTextLength);
}


//	#pragma mark - TypingUndoBuffer


BTextView::TypingUndoBuffer::TypingUndoBuffer(BTextView* textView)
	: BTextView::UndoBuffer(textView, B_UNDO_TYPING),
	fTypedText(NULL),
	fTypedStart(fStart),
	fTypedEnd(fEnd),
	fUndone(0)
{
}


BTextView::TypingUndoBuffer::~TypingUndoBuffer()
{
	free(fTypedText);
}


void
BTextView::TypingUndoBuffer::UndoSelf(BClipboard* clipboard)
{
	int32 len = fTypedEnd - fTypedStart;
	
	free(fTypedText);
	fTypedText = (char*)malloc(len);
	memcpy(fTypedText, fTextView->Text() + fTypedStart, len);
	
	fTextView->Select(fTypedStart, fTypedStart);
	fTextView->Delete(fTypedStart, fTypedEnd);
	fTextView->Insert(fTextData, fTextLength);
	fTextView->Select(fStart, fEnd);
	fUndone++;
}


void
BTextView::TypingUndoBuffer::RedoSelf(BClipboard* clipboard)
{	
	fTextView->Select(fTypedStart, fTypedStart);
	fTextView->Delete(fTypedStart, fTypedStart + fTextLength);
	fTextView->Insert(fTypedText, fTypedEnd - fTypedStart);
	fUndone--;
}


void
BTextView::TypingUndoBuffer::InputCharacter(int32 len)
{
	int32 start, end;
	fTextView->GetSelection(&start, &end);
	
	if (start != fTypedEnd || end != fTypedEnd)
		_Reset();
		
	fTypedEnd += len;
}


void
BTextView::TypingUndoBuffer::_Reset()
{
	free(fTextData);
	fTextView->GetSelection(&fStart, &fEnd);
	fTextLength = fEnd - fStart;
	fTypedStart = fStart;
	fTypedEnd = fStart;
	
	fTextData = (char*)malloc(fTextLength);
	memcpy(fTextData, fTextView->Text() + fStart, fTextLength);
	
	free(fTypedText);
	fTypedText = NULL;
	fRedo = false;
	fUndone = 0;
}


void
BTextView::TypingUndoBuffer::BackwardErase()
{
	int32 start, end;
	fTextView->GetSelection(&start, &end);
	
	const char* text = fTextView->Text();
	int32 charLen = UTF8PreviousCharLen(text + start, text);
	
	if (start != fTypedEnd || end != fTypedEnd) {
		_Reset();
		// if we've got a selection, we're already done
		if (start != end)
			return;
	} 
	
	char* buffer = (char*)malloc(fTextLength + charLen);
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
BTextView::TypingUndoBuffer::ForwardErase()
{
	// TODO: Cleanup
	int32 start, end;

	fTextView->GetSelection(&start, &end);
	
	int32 charLen = UTF8NextCharLen(fTextView->Text() + start);	
	
	if (start != fTypedEnd || end != fTypedEnd || fUndone > 0) {
		_Reset();
		// if we've got a selection, we're already done
		if (fStart == fEnd) {
			free(fTextData);
			fTextLength = charLen;
			fTextData = (char*)malloc(fTextLength);
			
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
		char* buffer = (char*)malloc(newLength);
		
		// copy the already stored data
		memcpy(buffer, fTextData, fTextLength);
		
		if (fTextLength < newLength) {
			// store the erased character
			for (int32 x = 0; x < charLen; x++)
				buffer[fTextLength + x] = fTextView->ByteAt(start + x);
		}

		fTextLength = newLength;
		free(fTextData);
		fTextData = buffer;
	}
}
