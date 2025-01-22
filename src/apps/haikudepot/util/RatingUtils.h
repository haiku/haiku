/*
 * Copyright 2020-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef RATING_UTILS_H
#define RATING_UTILS_H


#include <Referenceable.h>

#include "PackageUserRatingInfo.h"


class BView;
class BBitmap;


class RatingStarsMetrics
{
public:
							RatingStarsMetrics(BSize starSize);

	const	BSize			StarSize() const;
			float			SpacingBetweenStars() const;
	const	BPoint			LocationOfStarAtIndex(int index) const;
	const	BSize			Size() const;

private:
			BSize			fStarSize;
};


class RatingUtils
{
public:
	static	void			Draw(BView* target, BPoint at, float value,
								const BBitmap* star);
	static	void			Draw(BView* target, BPoint at, float value);

	static	bool			ShouldTryPopulateUserRatings(PackageUserRatingInfoRef userRatingInfo);
};


#endif // RATING_UTILS_H
