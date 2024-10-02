/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef USER_RATING_SUMMARY_H
#define USER_RATING_SUMMARY_H

#include <Referenceable.h>


/*!	This class contains data which provides an aggregate view of the user ratings
	for a package. Note that the data here is the result of an algorithm that only
	incorporates data from certain user ratings; for example we don't want to
	include users' ratings for every single version of a package but only the last
	one. Other rules also apply. For this reason, the calculation is performed on
	the server-side and presented here.
*/

class UserRatingSummary : public BReferenceable
{
public:
								UserRatingSummary();
								UserRatingSummary(const UserRatingSummary& other);

			void				Clear();

			float				AverageRating() const
									{ return fAverageRating; }
			int					RatingCount() const
									{ return fRatingCount; }
			int					RatingCountByStar(int star) const;

			void				SetAverageRating(float value);
			void				SetRatingCount(int value);
			void				SetRatingByStar(int star, int ratingCount);

			UserRatingSummary&	operator=(const UserRatingSummary& other);
			bool				operator==(const UserRatingSummary& other) const;
			bool				operator!=(const UserRatingSummary& other) const;

private:
			float				fAverageRating;
			int					fRatingCount;

			int					fRatingCountNoStar;
			int					fRatingCountByStar[6];
};

typedef BReference<UserRatingSummary> UserRatingSummaryRef;


#endif // USER_RATING_SUMMARY_H
