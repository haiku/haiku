/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "UserRatingInfo.h"


UserRatingInfo::UserRatingInfo()
	:
	fUserRatings(),
	fSummary(),
	fUserRatingsPopulated(false)
{
}


UserRatingInfo::UserRatingInfo(const UserRatingInfo& other)
	:
	fSummary(other.Summary()),
	fUserRatingsPopulated(other.UserRatingsPopulated())
{
	for (int32 i = CountUserRatings() - 1; i >= 0; i--)
		AddUserRating(other.UserRatingAtIndex(i));
}


void
UserRatingInfo::Clear()
{
	fSummary.Unset();
	ClearUserRatings();
}


void
UserRatingInfo::SetSummary(UserRatingSummaryRef value)
{
	fSummary = value;
}


void
UserRatingInfo::SetUserRatingsPopulated()
{
	fUserRatingsPopulated = true;
}


bool
UserRatingInfo::UserRatingsPopulated() const
{
	return fUserRatingsPopulated;
}


void
UserRatingInfo::ClearUserRatings()
{
	fUserRatings.clear();
	fUserRatingsPopulated = false;
}


int32
UserRatingInfo::CountUserRatings() const
{
	return fUserRatings.size();
}


UserRatingRef
UserRatingInfo::UserRatingAtIndex(int32 index) const
{
	return fUserRatings[index];
}


void
UserRatingInfo::AddUserRating(const UserRatingRef& rating)
{
	fUserRatings.push_back(rating);
}


UserRatingInfo&
UserRatingInfo::operator=(const UserRatingInfo& other)
{
	fSummary = other.Summary();

	fUserRatings.clear();

	fUserRatingsPopulated = other.UserRatingsPopulated();

	for (int32 i = CountUserRatings() - 1; i >= 0; i--)
		AddUserRating(other.UserRatingAtIndex(i));

	return *this;
}


bool
UserRatingInfo::operator==(const UserRatingInfo& other) const
{
	if (Summary() != other.Summary())
		return false;

	if (UserRatingsPopulated() != other.UserRatingsPopulated())
		return false;

	if (CountUserRatings() != other.CountUserRatings())
		return false;

	for (int32 i = CountUserRatings() - 1; i >= 0; i--) {
		if (other.UserRatingAtIndex(i) != UserRatingAtIndex(i))
			return false;
	}

	return true;
}


bool
UserRatingInfo::operator!=(const UserRatingInfo& other) const
{
	return !(*this == other);
}
