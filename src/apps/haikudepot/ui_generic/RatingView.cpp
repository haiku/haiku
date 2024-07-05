/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2018-2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "RatingView.h"

#include <stdio.h>

#include <LayoutUtils.h>

#include "HaikuDepotConstants.h"
#include "RatingUtils.h"
#include "SharedIcons.h"


RatingView::RatingView(const char* name)
	:
	BView(name, B_WILL_DRAW),
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
		return SharedIcons::IconStarGrey16Scaled()->Bitmap();
	return SharedIcons::IconStarBlue16Scaled()->Bitmap();
}


void
RatingView::Draw(BRect updateRect)
{
	RatingUtils::Draw(this, BPoint(0, 0), fRating, StarBitmap());
}


BSize
RatingView::MinSize()
{
	RatingStarsMetrics metrics(StarBitmap()->Bounds().Size());
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), metrics.Size());
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

