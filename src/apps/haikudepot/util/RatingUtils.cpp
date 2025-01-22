/*
 * Copyright 2020-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <View.h>


#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "RatingUtils.h"
#include "SharedIcons.h"


RatingStarsMetrics::RatingStarsMetrics(BSize starSize)
	:
	fStarSize(starSize)
{
}


const BSize
RatingStarsMetrics::StarSize() const
{
	return fStarSize;
}


float
RatingStarsMetrics::SpacingBetweenStars() const
{
	return 2.0 * fStarSize.Width() / 16.0;
}


const BPoint
RatingStarsMetrics::LocationOfStarAtIndex(int index) const
{
	float indexf = static_cast<float>(index);
	return BPoint(indexf * (fStarSize.Width() + SpacingBetweenStars()), 0.0);
}


const BSize
RatingStarsMetrics::Size() const
{
	return BSize((fStarSize.Width() * 5) + (SpacingBetweenStars() * 4), fStarSize.Height());
}


/*static*/ void
RatingUtils::Draw(BView* target, BPoint at, float value)
{
	const BBitmap* star;

	if (value < RATING_MIN)
		star = SharedIcons::IconStarGrey16Scaled()->Bitmap();
	else
		star = SharedIcons::IconStarBlue16Scaled()->Bitmap();

	if (star == NULL) {
		debugger("no star icon found in application resources.");
		return;
	}

	Draw(target, at, value, star);
}


/*static*/ void
RatingUtils::Draw(BView* target, BPoint at, float value, const BBitmap* star)
{
	if (star == NULL) {
		HDFATAL("no star icon found in application resources.");
		return;
	}

	RatingStarsMetrics metrics(star->Bounds().Size());
	BRect rect(at, metrics.Size());

	target->FillRect(rect, B_SOLID_LOW);
		// a rectangle covering the whole area of the stars

	target->SetDrawingMode(B_OP_OVER);

	for (int i = 0; i < 5; i++)
		target->DrawBitmap(star, rect.LeftTop() + metrics.LocationOfStarAtIndex(i));

	if (value >= RATING_MIN && value < 5.0f) {
		target->SetDrawingMode(B_OP_OVER);
		BRect shadeOverRect = rect;
		shadeOverRect.left = ceilf(rect.right - (1.0 - (value / 5.0f)) * rect.Width());

		rgb_color color = target->LowColor();
		color.alpha = 190;
		target->SetHighColor(color);

		target->SetDrawingMode(B_OP_ALPHA);
		target->FillRect(shadeOverRect, B_SOLID_HIGH);
	}
}


/*!	With the `userRatingInfo` provided, does it make sense for the application
	to attempt to download the user ratings? If it looks like there are none
	then it's making no sense and if it has already downloaded some then it
	also does not make any sense.
*/
/*static*/ bool
RatingUtils::ShouldTryPopulateUserRatings(PackageUserRatingInfoRef userRatingInfo)
{
	if (!userRatingInfo.IsSet())
		return true;

	UserRatingSummaryRef summary = userRatingInfo->Summary();

	if (!summary.IsSet())
		return true;

	if (summary->RatingCount() == 0)
		return false;

	return !userRatingInfo->UserRatingsPopulated();
}
