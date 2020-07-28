/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <View.h>


#include "HaikuDepotConstants.h"
#include "RatingUtils.h"


BReference<SharedBitmap> RatingUtils::sStarBlueBitmap
	= BReference<SharedBitmap>(new SharedBitmap(RSRC_STAR_BLUE));
BReference<SharedBitmap> RatingUtils::sStarGrayBitmap
	= BReference<SharedBitmap>(new SharedBitmap(RSRC_STAR_GREY));


/*static*/ void
RatingUtils::Draw(BView* target, BPoint at, float value)
{
	const BBitmap* star;

	if (value < RATING_MIN)
		star = sStarGrayBitmap->Bitmap(BITMAP_SIZE_16);
	else
		star = sStarBlueBitmap->Bitmap(BITMAP_SIZE_16);

	if (star == NULL) {
		debugger("no star icon found in application resources.");
		return;
	}

	Draw(target, at, value, star);
}


/*static*/ void
RatingUtils::Draw(BView* target, BPoint at, float value,
	const BBitmap* star)
{
	BRect rect = BOUNDS_RATING;
	rect.OffsetBy(at);

		// a rectangle covering the whole area of the stars
	target->FillRect(rect, B_SOLID_LOW);

	if (star == NULL) {
		debugger("no star icon found in application resources.");
		return;
	}

	target->SetDrawingMode(B_OP_OVER);

	float x = 0;
	for (int i = 0; i < 5; i++) {
		target->DrawBitmap(star, at + BPoint(x, 0));
		x += SIZE_RATING_STAR + WIDTH_RATING_STAR_SPACING;
	}

	if (value >= RATING_MIN && value < 5.0f) {
		target->SetDrawingMode(B_OP_OVER);

		rect = BOUNDS_RATING;
		rect.right = x - 2;
		rect.left = ceilf(rect.left + (value / 5.0f) * rect.Width());
		rect.OffsetBy(at);

		rgb_color color = target->LowColor();
		color.alpha = 190;
		target->SetHighColor(color);

		target->SetDrawingMode(B_OP_ALPHA);
		target->FillRect(rect, B_SOLID_HIGH);
	}
}