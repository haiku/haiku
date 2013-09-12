/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TextDocumentView.h"

#include <algorithm>

#include <ScrollBar.h>


TextDocumentView::TextDocumentView(const char* name)
	:
	BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS),
	fInsetLeft(0.0f),
	fInsetTop(0.0f),
	fInsetRight(0.0f),
	fInsetBottom(0.0f)
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
	InvalidateLayout();
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

