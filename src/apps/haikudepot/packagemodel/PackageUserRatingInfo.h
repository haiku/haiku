/*
 * Copyright 2024-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_USER_RATING_INFO_H
#define PACKAGE_USER_RATING_INFO_H

#include <vector>

#include <Referenceable.h>

#include "UserRating.h"
#include "UserRatingSummary.h"


class PackageUserRatingInfoBuilder;


/*!	This class carries the collection of user ratings related to a package but
	also provides an instance of `UserRatingSummaryRef` which is a summary of
	the user ratings.

	Instances of this class should not be created directly; instead use the
  	PackageUserRatingInfoBuilder class as a builder-constructor.
*/
class PackageUserRatingInfo : public BReferenceable
{
friend class PackageUserRatingInfoBuilder;

public:
								PackageUserRatingInfo();
								PackageUserRatingInfo(const PackageUserRatingInfo& other);

			bool				operator==(const PackageUserRatingInfo& other) const;
			bool				operator!=(const PackageUserRatingInfo& other) const;

	const	UserRatingSummaryRef
								Summary() const;
			int32				CountUserRatings() const;
	const	UserRatingRef		UserRatingAtIndex(int32 index) const;
			bool				UserRatingsPopulated() const;

private:
			void				SetSummary(UserRatingSummaryRef value);
			void				AddUserRating(const UserRatingRef& rating);
			void				SetUserRatingsPopulated();

private:
			std::vector<UserRatingRef>
								fUserRatings;
			UserRatingSummaryRef
								fSummary;
			bool				fUserRatingsPopulated;
};


typedef BReference<PackageUserRatingInfo> PackageUserRatingInfoRef;


class PackageUserRatingInfoBuilder
{
public:
								PackageUserRatingInfoBuilder();
								PackageUserRatingInfoBuilder(const PackageUserRatingInfoRef& other);
	virtual						~PackageUserRatingInfoBuilder();

			PackageUserRatingInfoRef
								BuildRef();

			PackageUserRatingInfoBuilder&
								AddUserRating(const UserRatingRef& rating);
			PackageUserRatingInfoBuilder&
								WithSummary(UserRatingSummaryRef value);
			PackageUserRatingInfoBuilder&
								WithUserRatingsPopulated();

private:
			void				_InitFromSource();
			void				_Init(const PackageUserRatingInfo* value);

private:
			PackageUserRatingInfoRef
								fSource;
			std::vector<UserRatingRef>
								fUserRatings;
			UserRatingSummaryRef
								fSummary;
			bool				fUserRatingsPopulated;
};


#endif // PACKAGE_USER_RATING_INFO_H
