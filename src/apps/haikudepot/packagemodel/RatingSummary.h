/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef RATING_SUMMARY_H
#define RATING_SUMMARY_H


class RatingSummary {
public:
								RatingSummary();
								RatingSummary(const RatingSummary& other);

			RatingSummary&		operator=(const RatingSummary& other);
			bool				operator==(const RatingSummary& other) const;
			bool				operator!=(const RatingSummary& other) const;

public:
			float				averageRating;
			int					ratingCount;

			int					ratingCountByStar[5];
};


#endif // RATING_SUMMARY_H
