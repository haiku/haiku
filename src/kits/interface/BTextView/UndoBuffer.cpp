#include "UndoBuffer.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <Clipboard.h>

//  ******** _BUndoBuffer_ *******

_BUndoBuffer_::_BUndoBuffer_(BTextView *textView, undo_state state)
	:
	fTextView(textView),
	fState(state),
	fRedo(false),
	fRunArray(NULL),
	fRunArrayLength(0)

{
	fTextView->GetSelection(&fStart, &fEnd);
	fTextLength = fEnd - fStart;
	
	fTextData = (char*)malloc(fTextLength);
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
	:_BUndoBuffer_(textView, B_UNDO_CUT)
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
	:_BUndoBuffer_(textView, B_UNDO_PASTE),
	fPasteText(NULL),
	fPasteTextLength(textLen),
	fPasteRunArray(NULL)
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
