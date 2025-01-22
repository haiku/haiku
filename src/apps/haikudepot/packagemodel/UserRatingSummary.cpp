/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * Copyright 2016-2025, Andrew Lindesay <apl@lindesay.co.nz>.
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
	for (int i = 0; i <= 5; i++)
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


float
UserRatingSummary::AverageRating() const
{
	return fAverageRating;
}


int
UserRatingSummary::RatingCount() const
{
	return fRatingCount;
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


// #pragma mark - UserRatingSummaryBuilder


UserRatingSummaryBuilder::UserRatingSummaryBuilder()
	:
	fAverageRating(RATING_MISSING),
	fRatingCount(0),
	fRatingCountNoStar(0)
{
	for (int i = 0; i < 6; i++)
		fRatingCountByStar[i] = 0;
}


UserRatingSummaryBuilder::UserRatingSummaryBuilder(const UserRatingSummaryRef& other)
	:
	fSource(),
	fAverageRating(RATING_MISSING),
	fRatingCount(0),
	fRatingCountNoStar(0)
{
	for (int i = 0; i < 6; i++)
		fRatingCountByStar[i] = other->RatingCountByStar(i);
}


UserRatingSummaryBuilder::~UserRatingSummaryBuilder()
{
}


UserRatingSummaryRef
UserRatingSummaryBuilder::BuildRef()
{
	if (fSource.IsSet())
		return fSource;

	UserRatingSummary* userRatingSummary = new UserRatingSummary();
	userRatingSummary->SetAverageRating(fAverageRating);
	userRatingSummary->SetRatingCount(fRatingCount);
	userRatingSummary->SetRatingByStar(RATING_MISSING_STAR, fRatingCountNoStar);

	for (int star = 0; star <= 5; star++)
		userRatingSummary->SetRatingByStar(star, fRatingCountByStar[star]);

	return UserRatingSummaryRef(userRatingSummary, true);
}


UserRatingSummaryBuilder&
UserRatingSummaryBuilder::WithAverageRating(float value)
{
	if (!fSource.IsSet() || fSource->AverageRating() != value) {
		_InitFromSource();
		fAverageRating = value;
	}
	return *this;
}


UserRatingSummaryBuilder&
UserRatingSummaryBuilder::WithRatingCount(int value)
{
	if (!fSource.IsSet() || fSource->RatingCount() != value) {
		_InitFromSource();
		fRatingCount = value;
	}
	return *this;
}


UserRatingSummaryBuilder&
UserRatingSummaryBuilder::AddRatingByStar(int star, int ratingCount)
{
	if (star < RATING_MISSING_STAR || star > 5) {
		HDERROR("bad star %" B_PRId32 " in builder", star);
		return *this;
	}

	if (!fSource.IsSet() || fSource->RatingCountByStar(star) != ratingCount) {
		_InitFromSource();

		if (star == RATING_MISSING_STAR)
			fRatingCountNoStar = ratingCount;
		else
			fRatingCountByStar[star] = ratingCount;
	}

	return *this;
}


void
UserRatingSummaryBuilder::_InitFromSource()
{
	if (fSource.IsSet()) {
		_Init(fSource.Get());
		fSource.Unset();
	}
}


void
UserRatingSummaryBuilder::_Init(const UserRatingSummary* value)
{
	fAverageRating = value->AverageRating();
	fRatingCount = value->RatingCount();
	fRatingCountNoStar = value->RatingCountByStar(RATING_MISSING_STAR);

	for (int i = 0; i <= 5; i++)
		fRatingCountByStar[i] = value->RatingCountByStar(i);
}
