#ifndef __UNDOBUFFER_H
#define __UNDOBUFFER_H

#include <TextView.h>

class BClipboard;
class _BUndoBuffer_
{
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

private:
	undo_state fState;
	bool fRedo;
};


//  ******** _BCutUndoBuffer_ *******
class _BCutUndoBuffer_ : public _BUndoBuffer_
{
public:
	_BCutUndoBuffer_(BTextView *textView);
	~_BCutUndoBuffer_();
protected:
	virtual void RedoSelf(BClipboard *);
};


//  ******** _BPasteUndoBuffer_ *******
class _BPasteUndoBuffer_ : public _BUndoBuffer_
{
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
	int32 fPasteRunArrayLength;
};


//  ******** _BClearUndoBuffer_ *******
class _BClearUndoBuffer_ : public _BUndoBuffer_
{
public:
	_BClearUndoBuffer_(BTextView *textView);
	~_BClearUndoBuffer_();
protected:
	virtual void RedoSelf(BClipboard *);
};


#endif //__UNDOBUFFER_H
