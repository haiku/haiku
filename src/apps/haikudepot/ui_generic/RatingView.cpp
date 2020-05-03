/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2018-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "RatingView.h"

#include <stdio.h>

#include <LayoutUtils.h>

#include "HaikuDepotConstants.h"


RatingView::RatingView(const char* name)
	:
	BView(name, B_WILL_DRAW),
	fStarBlueBitmap(new SharedBitmap(RSRC_STAR_BLUE)),
	fStarGrayBitmap(new SharedBitmap(RSRC_STAR_GREY)),
	fRating(RATING_MISSING)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetLowUIColor(ViewUIColor());
}


RatingView::~RatingView()
{
}


void
RatingView::AttachedToWindow()
{
	AdoptParentColors();
}

/*! This method will return a star image that can be used repeatedly in the
    user interface in order to signify the rating given by a user.  It could
    be grey if no rating is assigned.
*/

const BBitmap*
RatingView::StarBitmap()
{
	if (fRating < RATING_MIN)
		return fStarGrayBitmap->Bitmap(SharedBitmap::SIZE_16);
	return fStarBlueBitmap->Bitmap(SharedBitmap::SIZE_16);
}


void
RatingView::Draw(BRect updateRect)
{
	FillRect(updateRect, B_SOLID_LOW);
	const BBitmap* star = StarBitmap();

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

	if (fRating >= RATING_MIN && fRating < 5.0f) {
		SetDrawingMode(B_OP_OVER);

		BRect rect(Bounds());
		rect.right = x - 2;
		rect.left = ceilf(rect.left + (fRating / 5.0f) * rect.Width());

		rgb_color color = LowColor();
		color.alpha = 190;
		SetHighColor(color);

		SetDrawingMode(B_OP_ALPHA);
		FillRect(rect, B_SOLID_HIGH);
	}
}


BSize
RatingView::MinSize()
{
	BSize size(16 * 5 + 2 * 4, 16 + 2);
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), size);
}


BSize
RatingView::PreferredSize()
{
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), MinSize());
}


BSize
RatingView::MaxSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), MinSize());
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

