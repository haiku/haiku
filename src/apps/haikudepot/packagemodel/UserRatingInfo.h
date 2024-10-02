/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef USER_RATING_INFO_H
#define USER_RATING_INFO_H

#include <vector>

#include <Referenceable.h>

#include "UserRating.h"
#include "UserRatingSummary.h"


/*!	This class carries the collection of user ratings related to a package but also provides
	an instance of `UserRatingSummaryRef` which is a summary of the user ratings.
*/

class UserRatingInfo : public BReferenceable
{
public:
								UserRatingInfo();
								UserRatingInfo(const UserRatingInfo& other);

			void				Clear();

	const	UserRatingSummaryRef
								Summary() const
									{ return fSummary; }
			void				SetSummary(UserRatingSummaryRef value);

			void				ClearUserRatings();
			void				AddUserRating(const UserRatingRef& rating);
			int32				CountUserRatings() const;
			UserRatingRef		UserRatingAtIndex(int32 index) const;

			void				SetUserRatingsPopulated();
			bool				UserRatingsPopulated() const;

			UserRatingInfo&		operator=(const UserRatingInfo& other);
			bool				operator==(const UserRatingInfo& other) const;
			bool				operator!=(const UserRatingInfo& other) const;

private:
			std::vector<UserRatingRef>
								fUserRatings;
			UserRatingSummaryRef
								fSummary;
			bool				fUserRatingsPopulated;
};

typedef BReference<UserRatingInfo> UserRatingInfoRef;


#endif // USER_RATING_SUMMARY_H
