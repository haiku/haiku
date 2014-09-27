/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "RatingView.h"

#include <stdio.h>


RatingView::RatingView(const char* name)
	:
	BView(name, B_WILL_DRAW),
	fStarBitmap(501),
	fRating(-1.0f)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


RatingView::~RatingView()
{
}


void
RatingView::AttachedToWindow()
{
	BView* parent = Parent();
	if (parent != NULL)
		SetLowColor(parent->ViewColor());
}


void
RatingView::Draw(BRect updateRect)
{
	FillRect(updateRect, B_SOLID_LOW);

	if (fRating < 0.0f)
		return;

	const BBitmap* star = fStarBitmap.Bitmap(SharedBitmap::SIZE_16);
	if (star == NULL) {
		fprintf(stderr, "No star icon found in application resources.\n");
		return;
	}

	SetDrawingMode(B_OP_OVER);

	float x = 0;
	for (int i = 0; i < 5; i++) {
		DrawBitmap(star, BPoint(x, 0));
		x += 16 + 2;
	}

	if (fRating >= 5.0f)
		return;

	SetDrawingMode(B_OP_OVER);

	BRect rect(Bounds());
	rect.left = ceilf(rect.left + (fRating / 5.0f) * rect.Width());

	rgb_color color = LowColor();
	color.alpha = 190;
	SetHighColor(color);

	SetDrawingMode(B_OP_ALPHA);
	FillRect(rect, B_SOLID_HIGH);
}


BSize
RatingView::MinSize()
{
	return BSize(16 * 5 + 2 * 4, 16 + 2);
}


BSize
RatingView::PreferredSize()
{
	return MinSize();
}


BSize
RatingView::MaxSize()
{
	return MinSize();
}


void
RatingView::SetRating(float rating)
{
	fRating = rating;
	Invalidate();
}


float
RatingView::Rating() const
{
	return fRating;
}

