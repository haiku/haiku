/*
 * Copyright 2024-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "PackageUserRatingInfo.h"


PackageUserRatingInfo::PackageUserRatingInfo()
	:
	fUserRatings(),
	fSummary(),
	fUserRatingsPopulated(false)
{
}


PackageUserRatingInfo::PackageUserRatingInfo(const PackageUserRatingInfo& other)
	:
	fSummary(other.Summary()),
	fUserRatingsPopulated(other.UserRatingsPopulated())
{
	for (int32 i = CountUserRatings() - 1; i >= 0; i--)
		AddUserRating(other.UserRatingAtIndex(i));
}


const UserRatingSummaryRef
PackageUserRatingInfo::Summary() const
{
	return fSummary;
}


void
PackageUserRatingInfo::SetSummary(UserRatingSummaryRef value)
{
	fSummary = value;
}


void
PackageUserRatingInfo::SetUserRatingsPopulated()
{
	fUserRatingsPopulated = true;
}


bool
PackageUserRatingInfo::UserRatingsPopulated() const
{
	return fUserRatingsPopulated;
}


int32
PackageUserRatingInfo::CountUserRatings() const
{
	return fUserRatings.size();
}


const UserRatingRef
PackageUserRatingInfo::UserRatingAtIndex(int32 index) const
{
	return fUserRatings[index];
}


void
PackageUserRatingInfo::AddUserRating(const UserRatingRef& rating)
{
	fUserRatings.push_back(rating);
}


bool
PackageUserRatingInfo::operator==(const PackageUserRatingInfo& other) const
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
PackageUserRatingInfo::operator!=(const PackageUserRatingInfo& other) const
{
	return !(*this == other);
}


// #pragma mark - PackageUserRatingInfoBuilder


PackageUserRatingInfoBuilder::PackageUserRatingInfoBuilder()
	:
	fSource(),
	fUserRatings(),
	fSummary(),
	fUserRatingsPopulated(false)
{
}


PackageUserRatingInfoBuilder::PackageUserRatingInfoBuilder(const PackageUserRatingInfoRef& other)
	:
	fSource(other),
	fUserRatings(),
	fSummary(),
	fUserRatingsPopulated(false)
{
}


PackageUserRatingInfoBuilder::~PackageUserRatingInfoBuilder()
{
}


PackageUserRatingInfoRef
PackageUserRatingInfoBuilder::BuildRef()
{
	if (fSource.IsSet())
		return fSource;

	PackageUserRatingInfo* userRatingInfo = new PackageUserRatingInfo();

	userRatingInfo->SetSummary(fSummary);

	if (fUserRatingsPopulated)
		userRatingInfo->SetUserRatingsPopulated();

	std::vector<UserRatingRef>::const_iterator it;
	for (it = fUserRatings.begin(); it != fUserRatings.end(); it++)
		userRatingInfo->AddUserRating(*it);

	return PackageUserRatingInfoRef(userRatingInfo, true);
}


PackageUserRatingInfoBuilder&
PackageUserRatingInfoBuilder::AddUserRating(const UserRatingRef& rating)
{
	if (fSource.IsSet())
		_InitFromSource();
	fUserRatings.push_back(rating);
	return *this;
}


PackageUserRatingInfoBuilder&
PackageUserRatingInfoBuilder::WithSummary(UserRatingSummaryRef value)
{
	if (!fSource.IsSet() || fSource->Summary() != value) {
		_InitFromSource();
		fSummary = value;
	}
	return *this;
}


PackageUserRatingInfoBuilder&
PackageUserRatingInfoBuilder::WithUserRatingsPopulated()
{
	if (!fSource.IsSet() || !fSource->UserRatingsPopulated()) {
		_InitFromSource();
		fUserRatingsPopulated = true;
	}
	return *this;
}


void
PackageUserRatingInfoBuilder::_InitFromSource()
{
	if (fSource.IsSet()) {
		_Init(fSource.Get());
		fSource.Unset();
	}
}


void
PackageUserRatingInfoBuilder::_Init(const PackageUserRatingInfo* value)
{
	fSummary = value->Summary();
	fUserRatingsPopulated = value->UserRatingsPopulated();

	int32 count = value->CountUserRatings();
	for (int32 i = 0; i < count; i++)
		fUserRatings.push_back(value->UserRatingAtIndex(i));
}
