/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TextDocumentView.h"

#include <algorithm>

#include <ScrollBar.h>
#include <Shape.h>
#include <Window.h>


TextDocumentView::TextDocumentView(const char* name)
	:
	BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS),
	fInsetLeft(0.0f),
	fInsetTop(0.0f),
	fInsetRight(0.0f),
	fInsetBottom(0.0f),

	fSelectionAnchorOffset(0),
	fCaretOffset(0),
	fCaretAnchorX(0.0f),
	fShowCaret(false),

	fMouseDown(false)
{
	fTextDocumentLayout.SetWidth(_TextLayoutWidth(Bounds().Width()));

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(255, 255, 255, 255);
}


TextDocumentView::~TextDocumentView()
{
}


void
TextDocumentView::Draw(BRect updateRect)
{
	FillRect(updateRect, B_SOLID_LOW);

	fTextDocumentLayout.SetWidth(_TextLayoutWidth(Bounds().Width()));
	fTextDocumentLayout.Draw(this, BPoint(fInsetLeft, fInsetTop), updateRect);

	if (fSelectionAnchorOffset == fCaretOffset)
		return;

	int32 start;
	int32 end;
	if (fSelectionAnchorOffset <= fCaretOffset) {
		start = fSelectionAnchorOffset;
		end = fCaretOffset;
	} else {
		start = fCaretOffset;
		end = fSelectionAnchorOffset;
	}

	BShape shape;
	_GetSelectionShape(shape, start, end);

	SetHighColor(60, 40, 0);
	SetDrawingMode(B_OP_SUBTRACT);

	MovePenTo(fInsetLeft, fInsetTop);
	FillShape(&shape);
}


void
TextDocumentView::AttachedToWindow()
{
	_UpdateScrollBars();
}


void
TextDocumentView::FrameResized(float width, float height)
{
	fTextDocumentLayout.SetWidth(width);
	_UpdateScrollBars();
}


void
TextDocumentView::MouseDown(BPoint where)
{
	int32 modifiers = 0;
	if (Window() != NULL && Window()->CurrentMessage() != NULL)
		Window()->CurrentMessage()->FindInt32("modifiers", modifiers);
	
	fMouseDown = true;
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	
	bool extendSelection = (modifiers & B_SHIFT_KEY) != 0;
	SetCaret(where, extendSelection);
}


void
TextDocumentView::MouseUp(BPoint where)
{
	fMouseDown = false;
}


void
TextDocumentView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	if (fMouseDown)
		SetCaret(where, true);
}


BSize
TextDocumentView::MinSize()
{
	return BSize(fInsetLeft + fInsetRight + 50.0f, fInsetTop + fInsetBottom);
}


BSize
TextDocumentView::MaxSize()
{
	return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
}


BSize
TextDocumentView::PreferredSize()
{
	return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
}


bool
TextDocumentView::HasHeightForWidth()
{
	return true;
}


void
TextDocumentView::GetHeightForWidth(float width, float* min, float* max,
	float* preferred)
{
	TextDocumentLayout layout(fTextDocumentLayout);
	layout.SetWidth(_TextLayoutWidth(width));

	float height = layout.Height() + 1 + fInsetTop + fInsetBottom;

	if (min != NULL)
		*min = height;
	if (max != NULL)
		*max = height;
	if (preferred != NULL)
		*preferred = height;
}


void
TextDocumentView::SetTextDocument(const TextDocumentRef& document)
{
	fTextDocument = document;
	fTextDocumentLayout.SetTextDocument(fTextDocument);

	fSelectionAnchorOffset = 0;
	fCaretOffset = 0;
	fCaretAnchorX = 0.0f;

	InvalidateLayout();
	Invalidate();
	_UpdateScrollBars();
}


void
TextDocumentView::SetInsets(float inset)
{
	SetInsets(inset, inset, inset, inset);
}


void
TextDocumentView::SetInsets(float horizontal, float vertical)
{
	SetInsets(horizontal, vertical, horizontal, vertical);
}


void
TextDocumentView::SetInsets(float left, float top, float right, float bottom)
{
	if (fInsetLeft == left && fInsetTop == top
		&& fInsetRight == right && fInsetBottom == bottom) {
		return;
	}

	fInsetLeft = left;
	fInsetTop = top;
	fInsetRight = right;
	fInsetBottom = bottom;

	InvalidateLayout();
	Invalidate();
}


void
TextDocumentView::SetCaret(const BPoint& location, bool extendSelection)
{
	if (fTextDocument.Get() == NULL)
		return;

	bool rightOfChar = false;
	int32 caretOffset = fTextDocumentLayout.TextOffsetAt(
		location.x - fInsetLeft, location.y - fInsetTop, rightOfChar);

	if (rightOfChar)
		caretOffset++;

	_SetCaretOffset(caretOffset, true, extendSelection);
}


// #pragma mark - private


float
TextDocumentView::_TextLayoutWidth(float viewWidth) const
{
	return viewWidth - (fInsetLeft + fInsetRight);
}


static const float kHorizontalScrollBarStep = 10.0f;
static const float kVerticalScrollBarStep = 12.0f;


void
TextDocumentView::_UpdateScrollBars()
{
	BRect bounds(Bounds());

	BScrollBar* horizontalScrollBar = ScrollBar(B_HORIZONTAL);
	if (horizontalScrollBar != NULL) {
		long viewWidth = bounds.IntegerWidth();
		long dataWidth = (long)ceilf(
			fTextDocumentLayout.Width() + fInsetLeft + fInsetRight);

		long maxRange = dataWidth - viewWidth;
		maxRange = std::max(maxRange, 0L);

		horizontalScrollBar->SetRange(0, (float)maxRange);
		horizontalScrollBar->SetProportion((float)viewWidth / dataWidth);
		horizontalScrollBar->SetSteps(kHorizontalScrollBarStep, dataWidth / 10);
	}

 	BScrollBar* verticalScrollBar = ScrollBar(B_VERTICAL);
	if (verticalScrollBar != NULL) {
		long viewHeight = bounds.IntegerHeight();
		long dataHeight = (long)ceilf(
			fTextDocumentLayout.Height() + fInsetTop + fInsetBottom);

		long maxRange = dataHeight - viewHeight;
		maxRange = std::max(maxRange, 0L);

		verticalScrollBar->SetRange(0, maxRange);
		verticalScrollBar->SetProportion((float)viewHeight / dataHeight);
		verticalScrollBar->SetSteps(kVerticalScrollBarStep, viewHeight);
	}
}


void
TextDocumentView::_SetCaretOffset(int32 offset, bool updateAnchor,
	bool lockSelectionAnchor)
{
	if (offset < 0)
		offset = 0;
	int32 length = fTextDocument->Length();
	if (offset > length)
		offset = length;

	if (offset == fCaretOffset && (lockSelectionAnchor
			|| offset == fSelectionAnchorOffset)) {
		return;
	}

	if (!lockSelectionAnchor)
		fSelectionAnchorOffset = offset;

	fCaretOffset = offset;
	fShowCaret = true;

	if (updateAnchor) {
		float x1;
		float y1;
		float x2;
		float y2;

		fTextDocumentLayout.GetTextBounds(fCaretOffset, x1, y1, x2, y2);
		fCaretAnchorX = x1;
	}

	Invalidate();
}


// _GetSelectionShape
void
TextDocumentView::_GetSelectionShape(BShape& shape,
	int32 start, int32 end)
{
	float startX1;
	float startY1;
	float startX2;
	float startY2;
	fTextDocumentLayout.GetTextBounds(start, startX1, startY1, startX2,
		startY2);

	float endX1;
	float endY1;
	float endX2;
	float endY2;
	fTextDocumentLayout.GetTextBounds(end, endX1, endY1, endX2, endY2);

	int32 startLineIndex = fTextDocumentLayout.LineIndexForOffset(start);
	int32 endLineIndex = fTextDocumentLayout.LineIndexForOffset(end);

	if (startLineIndex == endLineIndex) {
		// Selection on one line
		BPoint lt(startX1, startY1);
		BPoint rt(endX1, endY1);
		BPoint rb(endX1, endY2);
		BPoint lb(startX1, startY2);

		shape.MoveTo(lt);
		shape.LineTo(rt);
		shape.LineTo(rb);
		shape.LineTo(lb);
		shape.Close();
	} else if (startLineIndex == endLineIndex - 1 && endX1 <= startX1) {
		// Selection on two lines, with gap:
		// ---------
		// ------###
		// ##-------
		// ---------
		BPoint lt(startX1, startY1);
		BPoint rt(fTextDocumentLayout.Width(), startY1);
		BPoint rb(fTextDocumentLayout.Width(), startY2);
		BPoint lb(startX1, startY2);

		shape.MoveTo(lt);
		shape.LineTo(rt);
		shape.LineTo(rb);
		shape.LineTo(lb);
		shape.Close();

		lt = BPoint(0, endY1);
		rt = BPoint(endX1, endY1);
		rb = BPoint(endX1, endY2);
		lb = BPoint(0, endY2);

		shape.MoveTo(lt);
		shape.LineTo(rt);
		shape.LineTo(rb);
		shape.LineTo(lb);
		shape.Close();
	} else {
		// Selection over multiple lines
		shape.MoveTo(BPoint(startX1, startY1));
		shape.LineTo(BPoint(fTextDocumentLayout.Width(), startY1));
		shape.LineTo(BPoint(fTextDocumentLayout.Width(), endY1));
		shape.LineTo(BPoint(endX1, endY1));
		shape.LineTo(BPoint(endX1, endY2));
		shape.LineTo(BPoint(0, endY2));
		shape.LineTo(BPoint(0, startY2));
		shape.LineTo(BPoint(startX1, startY2));
		shape.Close();
	}
}
