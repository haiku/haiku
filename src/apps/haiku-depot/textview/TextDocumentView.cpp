/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TextDocumentView.h"


TextDocumentView::TextDocumentView(const char* name)
	:
	BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS)
{
	fParagraphLayout.SetWidth(Bounds().Width());

	SetViewColor(B_TRANSPARENT_COLOR);
//	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(255, 255, 255, 255);
}


TextDocumentView::~TextDocumentView()
{
}


void
TextDocumentView::Draw(BRect updateRect)
{
	FillRect(updateRect, B_SOLID_LOW);

	fParagraphLayout.SetWidth(Bounds().Width());
	fParagraphLayout.Draw(this, B_ORIGIN);
}


void
TextDocumentView::AttachedToWindow()
{
	BView* parent = Parent();
	if (parent != NULL)
		SetLowColor(parent->ViewColor());
}


void
TextDocumentView::FrameResized(float width, float height)
{
	fParagraphLayout.SetWidth(width);
}


BSize
TextDocumentView::MinSize()
{
	return BSize(50.0f, 0.0f);
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
	ParagraphLayout layout(fParagraphLayout);
	layout.SetWidth(width);

	float height = layout.Height() + 1;

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
	fParagraphLayout.SetParagraph(fTextDocument->ParagraphAt(0));
	InvalidateLayout();
}

