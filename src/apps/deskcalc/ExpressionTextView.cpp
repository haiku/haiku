/*
 * Copyright 2006 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "ExpressionTextView.h"

#include <new>
#include <stdio.h>

#include <Beep.h>
#include <Window.h>

#include "CalcView.h"

using std::nothrow;

static const int32 kMaxPreviousExpressions = 20;


ExpressionTextView::ExpressionTextView(BRect frame, CalcView* calcView)
	: InputTextView(frame, "expression text view",
					(frame.OffsetToCopy(B_ORIGIN)).InsetByCopy(2, 2),
					B_FOLLOW_NONE, B_WILL_DRAW),
	  fCalcView(calcView),
	  fKeypadLabels(""),
	  fPreviousExpressions(20),
	  fHistoryPos(0),
	  fCurrentExpression("")
{
	SetFont(be_bold_font);
//	SetAlignment(B_ALIGN_RIGHT);
	SetStylable(false);
	SetDoesUndo(true);
	SetColorSpace(B_RGB32);
}


ExpressionTextView::~ExpressionTextView()
{
	int32 count = fPreviousExpressions.CountItems();
	for (int32 i = 0; i < count; i++)
		delete (BString*)fPreviousExpressions.ItemAtFast(i);
}


void
ExpressionTextView::MakeFocus(bool focused = true)
{
	if (focused == IsFocus()) {
		// stop endless loop when CalcView calls us again
		return;
	}

	// NOTE: order of lines important!
	InputTextView::MakeFocus(focused);
	fCalcView->MakeFocus(focused);
}


void
ExpressionTextView::KeyDown(const char* bytes, int32 numBytes)
{
	// handle expression history
	if (bytes[0] == B_UP_ARROW) {
		PreviousExpression();
		return;
	}
	if (bytes[0] == B_DOWN_ARROW) {
		NextExpression();
		return;
	}
	BString current = Text();

	// handle in InputTextView, except B_TAB
	if (bytes[0] != B_TAB)
		InputTextView::KeyDown(bytes, numBytes);

	// pass on to CalcView if this was a label on a key
	if (fKeypadLabels.FindFirst(bytes[0]) >= 0)
		fCalcView->FlashKey(bytes, numBytes);

	// as soon as something is typed, we are at the
	// end of the expression history
	if (current != Text())
		fHistoryPos = fPreviousExpressions.CountItems();
}


void
ExpressionTextView::MouseDown(BPoint where)
{
	uint32 buttons;
	Window()->CurrentMessage()->FindInt32("buttons", (int32*)&buttons);
	if (buttons & B_PRIMARY_MOUSE_BUTTON) {
		InputTextView::MouseDown(where);
		return;
	}
	where = ConvertToParent(where);
	fCalcView->MouseDown(where);
}


// #pragma mark -


void
ExpressionTextView::RevertChanges()
{
	Clear();
}


void
ExpressionTextView::ApplyChanges()
{
	AddExpressionToHistory(Text());
	fCalcView->Evaluate();
}


// #pragma mark -


void
ExpressionTextView::AddKeypadLabel(const char* label)
{
	fKeypadLabels << label;
}


void
ExpressionTextView::SetExpression(const char* expression)
{
	SetText(expression);
	int32 lastPos = strlen(expression);
	Select(lastPos, lastPos);
}


void
ExpressionTextView::BackSpace()
{
	if (Window())
		Window()->PostMessage(B_UNDO, this);
}


void
ExpressionTextView::Clear()
{
	SetText("");
}


// #pragma mark -


void
ExpressionTextView::AddExpressionToHistory(const char* expression)
{
	// clean out old expressions that are the same as
	// the one to be added
	int32 count = fPreviousExpressions.CountItems();
	for (int32 i = 0; i < count; i++) {
		BString* item = (BString*)fPreviousExpressions.ItemAt(i);
		if (*item == expression && fPreviousExpressions.RemoveItem(i)) {
			delete item;
			i--;
			count--;
		}
	}

	BString* item = new (nothrow) BString(expression);
	if (!item)
		return;
	if (!fPreviousExpressions.AddItem(item)) {
		delete item;
		return;
	}
	while (fPreviousExpressions.CountItems() > kMaxPreviousExpressions)
		delete (BString*)fPreviousExpressions.RemoveItem(0L);

	fHistoryPos = fPreviousExpressions.CountItems();
}


void
ExpressionTextView::PreviousExpression()
{
	int32 count = fPreviousExpressions.CountItems();
	if (fHistoryPos == count) {
		// save current expression
		fCurrentExpression = Text();
	}

	fHistoryPos--;
	if (fHistoryPos < 0) {
		fHistoryPos = 0;
		return;
	}

	BString* item = (BString*)fPreviousExpressions.ItemAt(fHistoryPos);
	if (item)
		SetExpression(item->String());
}


void
ExpressionTextView::NextExpression()
{
	int32 count = fPreviousExpressions.CountItems();

	fHistoryPos++;
	if (fHistoryPos == count) {
		SetExpression(fCurrentExpression.String());
		return;
	}

	if (fHistoryPos > count) {
		fHistoryPos = count;
		return;
	}

	BString* item = (BString*)fPreviousExpressions.ItemAt(fHistoryPos);
	if (item)
		SetExpression(item->String());
}


// #pragma mark -


void
ExpressionTextView::LoadSettings(const BMessage* archive)
{
	const char* oldExpression;
	for (int32 i = 0;
		 archive->FindString("previous expression", i, &oldExpression) == B_OK;
		 i++) {
		 AddExpressionToHistory(oldExpression);
	}
}


status_t
ExpressionTextView::SaveSettings(BMessage* archive) const
{
	int32 count = fPreviousExpressions.CountItems();
	for (int32 i = 0; i < count; i++) {
		BString* item = (BString*)fPreviousExpressions.ItemAtFast(i);
		status_t ret = archive->AddString("previous expression", item->String());
		if (ret < B_OK)
			return ret;
	}
	return B_OK;
}


