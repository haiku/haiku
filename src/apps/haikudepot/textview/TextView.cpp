/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TextView.h"


TextView::TextView(const char* name)
	:
	BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS)
{
	BFont font;
	GetFont(&font);

	::ParagraphStyle style;
	style.SetLineSpacing(ceilf(font.Size() * 0.2));

	SetParagraphStyle(style);
}


TextView::~TextView()
{
}


void
TextView::Draw(BRect updateRect)
{
	FillRect(updateRect, B_SOLID_LOW);

	fTextLayout.SetWidth(Bounds().Width());
	fTextLayout.Draw(this, B_ORIGIN);
}


void
TextView::AttachedToWindow()
{
	AdoptParentColors();
}


void
TextView::FrameResized(float width, float height)
{
	fTextLayout.SetWidth(width);
}


BSize
TextView::MinSize()
{
	return BSize(50.0f, 0.0f);
}


BSize
TextView::MaxSize()
{
	return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
}


BSize
TextView::PreferredSize()
{
	return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
}


bool
TextView::HasHeightForWidth()
{
	return true;
}


void
TextView::GetHeightForWidth(float width, float* min, float* max,
	float* preferred)
{
	ParagraphLayout layout(fTextLayout);
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
TextView::SetText(const BString& text)
{
	fText.Clear();

	CharacterStyle regularStyle;
	fText.Append(TextSpan(text, regularStyle));

	fTextLayout.SetParagraph(fText);

	InvalidateLayout();
	Invalidate();
}


void
TextView::SetParagraphStyle(const ::ParagraphStyle& style)
{
	fText.SetStyle(style);
	fTextLayout.SetParagraph(fText);

	InvalidateLayout();
	Invalidate();
}

