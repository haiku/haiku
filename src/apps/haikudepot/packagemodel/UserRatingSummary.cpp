/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * Copyright 2016-2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "UserRatingSummary.h"

#include "HaikuDepotConstants.h"
#include "Logger.h"


UserRatingSummary::UserRatingSummary()
	:
	fAverageRating(RATING_MISSING),
	fRatingCount(0),
	fRatingCountNoStar(0)
{
	for (int i = 0; i < 6; i++)
		fRatingCountByStar[i] = 0;
}


UserRatingSummary::UserRatingSummary(const UserRatingSummary& other)
	:
	fAverageRating(other.AverageRating()),
	fRatingCount(other.RatingCount()),
	fRatingCountNoStar(other.RatingCountByStar(RATING_MISSING_STAR))
{
	for (int i = 0; i < 6; i++)
		fRatingCountByStar[i] = other.RatingCountByStar(i);
}


void
UserRatingSummary::Clear()
{
	fAverageRating = RATING_MISSING;
	fRatingCount = 0;
	fRatingCountNoStar = 0;

	for (int i = 0; i < 6; i++)
		fRatingCountByStar[i] = 0;
}


void
UserRatingSummary::SetAverageRating(float value)
{
	fAverageRating = value;
}


void
UserRatingSummary::SetRatingCount(int value)
{
	fRatingCount = value;
}


int
UserRatingSummary::RatingCountByStar(int star) const
{
	if (star == RATING_MISSING_STAR)
		return fRatingCountNoStar;

	if (star >= 0 && star <= 5)
		return fRatingCountByStar[star];

	return 0;
}


void
UserRatingSummary::SetRatingByStar(int star, int ratingCount)
{
	if (star == RATING_MISSING_STAR)
		fRatingCountNoStar = ratingCount;

	if (ratingCount < 0) {
		HDERROR("bad rating count %" B_PRId32, ratingCount);
		return;
	}

	if (star < RATING_MISSING_STAR || star > 5) {
		HDERROR("bad star %" B_PRId32, star);
		return;
	}

	fRatingCountByStar[star] = ratingCount;
}


UserRatingSummary&
UserRatingSummary::operator=(const UserRatingSummary& other)
{
	fAverageRating = other.AverageRating();
	fRatingCount = other.RatingCount();
	fRatingCountNoStar = other.RatingCountByStar(-1);

	for (int i = 0; i < 6; i++)
		fRatingCountByStar[i] = other.RatingCountByStar(i);

	return *this;
}


bool
UserRatingSummary::operator==(const UserRatingSummary& other) const
{
	if (fAverageRating != other.AverageRating() || fRatingCount != other.RatingCount()
		|| fRatingCountNoStar != other.RatingCountByStar(-1)) {
		return false;
	}

	for (int i = 0; i < 6; i++) {
		if (fRatingCountByStar[i] != other.RatingCountByStar(i))
			return false;
	}

	return true;
}


bool
UserRatingSummary::operator!=(const UserRatingSummary& other) const
{
	return !(*this == other);
}
