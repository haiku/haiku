/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * Copyright 2016-2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "RatingSummary.h"


RatingSummary::RatingSummary()
	:
	averageRating(0.0f),
	ratingCount(0)
{
for (int i = 0; i < 5; i++)
	ratingCountByStar[i] = 0;
}


RatingSummary::RatingSummary(const RatingSummary& other)
{
	*this = other;
}


RatingSummary&
RatingSummary::operator=(const RatingSummary& other)
{
	averageRating = other.averageRating;
	ratingCount = other.ratingCount;

	for (int i = 0; i < 5; i++)
		ratingCountByStar[i] = other.ratingCountByStar[i];

	return *this;
}


bool
RatingSummary::operator==(const RatingSummary& other) const
{
	if (averageRating != other.averageRating
		|| ratingCount != other.ratingCount) {
		return false;
	}

	for (int i = 0; i < 5; i++) {
		if (ratingCountByStar[i] != other.ratingCountByStar[i])
			return false;
	}

	return true;
}


bool
RatingSummary::operator!=(const RatingSummary& other) const
{
	return !(*this == other);
}
