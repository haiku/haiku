/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TextDocumentLayout.h"

#include <new>
#include <stdio.h>

#include <View.h>


TextDocumentLayout::TextDocumentLayout()
	:
	fWidth(0.0f),
	fLayoutValid(false),

	fDocument(),
	fParagraphLayouts()
{
}


TextDocumentLayout::TextDocumentLayout(const TextDocumentRef& document)
	:
	fWidth(0.0f),
	fLayoutValid(false),

	fDocument(document),
	fParagraphLayouts()
{
	_Init();
}


TextDocumentLayout::TextDocumentLayout(const TextDocumentLayout& other)
	:
	fWidth(other.fWidth),
	fLayoutValid(other.fLayoutValid),

	fDocument(other.fDocument),
	fParagraphLayouts(other.fParagraphLayouts)
{
}


TextDocumentLayout::~TextDocumentLayout()
{
}


void
TextDocumentLayout::SetTextDocument(const TextDocumentRef& document)
{
	if (fDocument != document) {
		fDocument = document;
		_Init();
		fLayoutValid = false;
	}
}


void
TextDocumentLayout::SetWidth(float width)
{
	if (fWidth != width) {
		fWidth = width;
		fLayoutValid = false;
	}
}


float
TextDocumentLayout::Height()
{
	_ValidateLayout();

	float height = 0.0f;

	if (fParagraphLayouts.CountItems() > 0) {
		const ParagraphLayoutInfo& lastLayout = fParagraphLayouts.LastItem();
		height = lastLayout.y + lastLayout.layout->Height();
	}

	return height;
}


void
TextDocumentLayout::Draw(BView* view, const BPoint& offset,
	const BRect& updateRect)
{
	_ValidateLayout();

	int layoutCount = fParagraphLayouts.CountItems();
	for (int i = 0; i < layoutCount; i++) {
		const ParagraphLayoutInfo& layout = fParagraphLayouts.ItemAtFast(i);
		BPoint location(offset.x, offset.y + layout.y);
		if (location.y > updateRect.bottom)
			break;
		if (location.y + layout.layout->Height() > updateRect.top)
			layout.layout->Draw(view, location);
	}
}


// #pragma mark - private


void
TextDocumentLayout::_ValidateLayout()
{
	if (!fLayoutValid) {
		_Layout();
		fLayoutValid = true;
	}
}


void
TextDocumentLayout::_Init()
{
	fParagraphLayouts.Clear();

	const ParagraphList& paragraphs = fDocument->Paragraphs();

	int paragraphCount = paragraphs.CountItems();
	for (int i = 0; i < paragraphCount; i++) {
		const Paragraph& paragraph = paragraphs.ItemAtFast(i);
		ParagraphLayoutRef layout(new(std::nothrow) ParagraphLayout(paragraph),
			true);
		if (layout.Get() == NULL
			|| !fParagraphLayouts.Add(ParagraphLayoutInfo(0.0f, layout))) {
			fprintf(stderr, "TextDocumentLayout::_Layout() - out of memory\n");
			return;
		}
	}
}


void
TextDocumentLayout::_Layout()
{
	float y = 0.0f;

	int layoutCount = fParagraphLayouts.CountItems();
	for (int i = 0; i < layoutCount; i++) {
		ParagraphLayoutInfo info = fParagraphLayouts.ItemAtFast(i);
		const ParagraphStyle& style = info.layout->Style();

		if (i > 0)
			y += style.SpacingTop();

		fParagraphLayouts.Replace(i, ParagraphLayoutInfo(y, info.layout));

		info.layout->SetWidth(fWidth);
		y += info.layout->Height() + style.SpacingBottom();
	}
}
