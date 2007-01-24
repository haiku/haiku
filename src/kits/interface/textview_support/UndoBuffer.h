/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 */

#ifndef __UNDOBUFFER_H
#define __UNDOBUFFER_H

#include <TextView.h>


class BClipboard;

class _BUndoBuffer_ {
public:
	_BUndoBuffer_(BTextView *, undo_state);
	virtual ~_BUndoBuffer_();

	void Undo(BClipboard *);
	undo_state State(bool *);

protected:
	virtual void UndoSelf(BClipboard *);
	virtual void RedoSelf(BClipboard *);
	
	BTextView *fTextView;
	int32 fStart;
	int32 fEnd;

	char *fTextData;
	int32 fTextLength;
	text_run_array *fRunArray;
	int32 fRunArrayLength;
	
	bool fRedo;

private:
	undo_state fState;
};


//  ******** _BCutUndoBuffer_ *******
class _BCutUndoBuffer_ : public _BUndoBuffer_ {
public:
	_BCutUndoBuffer_(BTextView *textView);
	~_BCutUndoBuffer_();

protected:
	virtual void RedoSelf(BClipboard *);
};


//  ******** _BPasteUndoBuffer_ *******
class _BPasteUndoBuffer_ : public _BUndoBuffer_ {
public:
	_BPasteUndoBuffer_(BTextView *, const char *, int32, text_run_array *, int32);
	~_BPasteUndoBuffer_();

protected:
	virtual void UndoSelf(BClipboard *);
	virtual void RedoSelf(BClipboard *);

private:
	char *fPasteText;
	int32 fPasteTextLength;
	text_run_array *fPasteRunArray;
};


//  ******** _BClearUndoBuffer_ *******
class _BClearUndoBuffer_ : public _BUndoBuffer_ {
public:
	_BClearUndoBuffer_(BTextView *textView);
	~_BClearUndoBuffer_();

protected:
	virtual void RedoSelf(BClipboard *);
};


//  ******** _BDropUndoBuffer_ ********
class _BDropUndoBuffer_ : public _BUndoBuffer_ {
public:
	_BDropUndoBuffer_(BTextView *, char const *, int32, text_run_array *, int32, int32, bool);
	~_BDropUndoBuffer_();

protected:
	virtual void UndoSelf(BClipboard *);
	virtual void RedoSelf(BClipboard *);

private:
	char *fDropText;
	int32 fDropTextLength;
	text_run_array *fDropRunArray;
	
	int32 fDropLocation;
	bool fInternalDrop;
};


//  ******** _BTypingUndoBuffer_ ********
class _BTypingUndoBuffer_ : public _BUndoBuffer_ {
public:
	_BTypingUndoBuffer_(BTextView *);
	~_BTypingUndoBuffer_();

	void InputCharacter(int32);
	void BackwardErase();
	void ForwardErase();

protected:
	virtual void RedoSelf(BClipboard *);
	virtual void UndoSelf(BClipboard *);

private:
	void Reset();
	
	char *fTypedText;
	int32 fTypedStart;
	int32 fTypedEnd;
	int32 fUndone;
};

#endif //__UNDOBUFFER_H
