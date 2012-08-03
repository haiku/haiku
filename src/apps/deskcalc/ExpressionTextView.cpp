/*
 * Copyright 2006-2011 Haiku, Inc. All Rights Reserved.
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
	:
	InputTextView(frame, "expression text view",
		(frame.OffsetToCopy(B_ORIGIN)).InsetByCopy(2, 2),
		B_FOLLOW_NONE, B_WILL_DRAW),
	fCalcView(calcView),
	fKeypadLabels(""),
	fPreviousExpressions(20),
	fHistoryPos(0),
	fCurrentExpression(""),
	fCurrentValue(""),
	fChangesApplied(false)
{
	SetStylable(false);
	SetDoesUndo(true);
	SetColorSpace(B_RGB32);
	SetFontAndColor(be_bold_font, B_FONT_ALL);
}


ExpressionTextView::~ExpressionTextView()
{
	int32 count = fPreviousExpressions.CountItems();
	for (int32 i = 0; i < count; i++)
		delete (BString*)fPreviousExpressions.ItemAtFast(i);
}


void
ExpressionTextView::MakeFocus(bool focused)
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
	// Handle expression history
	if (bytes[0] == B_UP_ARROW) {
		PreviousExpression();
		return;
	}
	if (bytes[0] == B_DOWN_ARROW) {
		NextExpression();
		return;
	}
	BString current = Text();

	// Handle in InputTextView, except B_TAB
	if (bytes[0] == '=')
		ApplyChanges();
	else if (bytes[0] != B_TAB)
		InputTextView::KeyDown(bytes, numBytes);

	// Pass on to CalcView if this was a label on a key
	if (fKeypadLabels.FindFirst(bytes[0]) >= 0)
		fCalcView->FlashKey(bytes, numBytes);
	else if (bytes[0] == B_BACKSPACE)
		fCalcView->FlashKey("BS", 2);

	// As soon as something is typed, we are at the end of the expression
	// history.
	if (current != Text())
		fHistoryPos = fPreviousExpressions.CountItems();

	// If changes where not applied the value has become a new expression
	// note that even if only the left or right arrow keys are pressed the
	// fCurrentValue string will be cleared.
	if (!fChangesApplied)
		fCurrentValue.SetTo("");
	else
		fChangesApplied = false;
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


void
ExpressionTextView::GetDragParameters(BMessage* dragMessage,
	BBitmap** bitmap, BPoint* point, BHandler** handler)
{
	InputTextView::GetDragParameters(dragMessage, bitmap, point, handler);
	dragMessage->AddString("be:clip_name", "DeskCalc clipping");
}


void
ExpressionTextView::SetTextRect(BRect rect)
{
	InputTextView::SetTextRect(rect);

	int32 count = fPreviousExpressions.CountItems();
	if (fHistoryPos == count && fCurrentValue.CountChars() > 0)
		SetValue(fCurrentValue.String());
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
	fCalcView->FlashKey("=", 1);
	fCalcView->Evaluate();
	fChangesApplied = true;
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
ExpressionTextView::SetValue(BString value)
{
	// save the value
	fCurrentValue = value;

	// calculate the width of the string
	BFont font;
	uint32 mode = B_FONT_ALL;
	GetFontAndColor(&font, &mode);
	float stringWidth = font.StringWidth(value);

	// make the string shorter if it does not fit in the view
	float viewWidth = Frame().Width();
	if (value.CountChars() > 3 && stringWidth > viewWidth) {
		// get the position of the first digit
		int32 firstDigit = 0;
		if (value[0] == '-')
			firstDigit++;

		// calculate the value of the exponent
		int32 exponent = 0;
		int32 offset = value.FindFirst('.');
		if (offset == B_ERROR) {
			exponent = value.CountChars() - 1 - firstDigit;
			value.Insert('.', 1, firstDigit + 1);
		} else {
			if (offset == firstDigit + 1) {
				// if the value is 0.01 or larger then scientific notation
				// won't shorten the string
				if (value[firstDigit] != '0' || value[firstDigit + 2] != '0'
					|| value[firstDigit + 3] != '0') {
					exponent = 0;
				} else {
					// remove the period
					value.Remove(offset, 1);

					// check for negative exponent value
					exponent = 0;
					while (value[firstDigit] == '0') {
						value.Remove(firstDigit, 1);
						exponent--;
					}

					// add the period
					value.Insert('.', 1, firstDigit + 1);
				}
			} else {
				// if the period + 1 digit fits in the view scientific notation
				// won't shorten the string
				BString temp = value;
				temp.Truncate(offset + 2);
				stringWidth = font.StringWidth(temp);
				if (stringWidth < viewWidth)
					exponent = 0;
				else {
					// move the period
					value.Remove(offset, 1);
					value.Insert('.', 1, firstDigit + 1);

					exponent = offset - (firstDigit + 1);
				}
			}
		}

		// add the exponent
		offset = value.CountChars() - 1;
		if (exponent != 0)
			value << "E" << exponent;

		// reduce the number of digits until the string fits or can not be
		// made any shorter
		stringWidth = font.StringWidth(value);
		char lastRemovedDigit = '0';
		while (offset > firstDigit && stringWidth > viewWidth) {
			if (value[offset] != '.')
				lastRemovedDigit = value[offset];
			value.Remove(offset--, 1);
			stringWidth = font.StringWidth(value);
		}

		// no need to keep the period if no digits follow
		if (value[offset] == '.') {
			value.Remove(offset, 1);
			offset--;
		}

		// take care of proper rounding of the result
		int digit = (int)lastRemovedDigit - '0'; // ascii to int
		if (digit >= 5) {
			for (; offset >= firstDigit; offset--) {
				if (value[offset] == '.')
					continue;

				digit = (int)(value[offset]) - '0' + 1; // ascii to int + 1
				if (digit != 10)
					break;

				value[offset] = '0';
			}
			if (digit == 10) {
				// carry over, shift the result
				if (value[firstDigit+1] == '.') {
					value[firstDigit+1] = '0';
					value[firstDigit] = '.';
				}
				value.Insert('1', 1, firstDigit);

				// remove the exponent value and the last digit
				offset = value.FindFirst('E');
				if (offset == B_ERROR)
					offset = value.CountChars();

				value.Truncate(--offset);
				offset--; // offset now points to the last digit

				// increase the exponent and add it back to the string
				exponent++;
				value << 'E' << exponent;
			} else {
				// increase the current digit value with one
				value[offset] = char(digit + 48);

				// set offset to last digit
				offset = value.FindFirst('E');
				if (offset == B_ERROR)
					offset = value.CountChars();

				offset--;
			}
		}

		// clean up decimal part if we have one
		if (value.FindFirst('.') != B_ERROR) {
			// remove trailing zeros
			while (value[offset] == '0')
				value.Remove(offset--, 1);

			// no need to keep the period if no digits follow
			if (value[offset] == '.')
				value.Remove(offset, 1);
		}
	}

	// set the new value
	SetExpression(value);
}


void
ExpressionTextView::BackSpace()
{
	const char bytes[1] = { B_BACKSPACE };
	KeyDown(bytes, 1);

	fCalcView->FlashKey("BS", 2);
}


void
ExpressionTextView::Clear()
{
	SetText("");

	fCalcView->FlashKey("C", 1);
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
		status_t ret = archive->AddString("previous expression",
			item->String());
		if (ret < B_OK)
			return ret;
	}
	return B_OK;
}
