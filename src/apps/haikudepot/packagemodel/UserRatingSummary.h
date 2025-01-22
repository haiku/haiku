/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2025, Andrew Lindesay <apl@lindesay.co.nz>.
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

	Instances of this class should not be created directly; instead use the
  	UserRatingSummaryBuilder class as a builder-constructor.
*/
class UserRatingSummary : public BReferenceable
{
friend class UserRatingSummaryBuilder;

public:
								UserRatingSummary();
								UserRatingSummary(const UserRatingSummary& other);

			bool				operator==(const UserRatingSummary& other) const;
			bool				operator!=(const UserRatingSummary& other) const;

			float				AverageRating() const;
			int					RatingCount() const;
			int					RatingCountByStar(int star) const;

private:
			void				SetAverageRating(float value);
			void				SetRatingCount(int value);
			void				SetRatingByStar(int star, int ratingCount);

private:
			float				fAverageRating;
			int					fRatingCount;
			int					fRatingCountNoStar;
			int					fRatingCountByStar[6];
};


typedef BReference<UserRatingSummary> UserRatingSummaryRef;


class UserRatingSummaryBuilder
{
public:
								UserRatingSummaryBuilder();
								UserRatingSummaryBuilder(const UserRatingSummaryRef& other);
	virtual						~UserRatingSummaryBuilder();

			UserRatingSummaryRef
								BuildRef();

			UserRatingSummaryBuilder&
								WithAverageRating(float value);
			UserRatingSummaryBuilder&
								WithRatingCount(int value);
			UserRatingSummaryBuilder&
								AddRatingByStar(int star, int ratingCount);

private:
			void				_InitFromSource();
			void				_Init(const UserRatingSummary* value);

private:
			UserRatingSummaryRef
								fSource;
			float				fAverageRating;
			int					fRatingCount;
			int					fRatingCountNoStar;
			int					fRatingCountByStar[6];
};


#endif // USER_RATING_SUMMARY_H
