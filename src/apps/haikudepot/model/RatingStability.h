/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef RATING_STABILITY_H
#define RATING_STABILITY_H


#include <Referenceable.h>
#include <String.h>


class RatingStability : public BReferenceable {
public:
								RatingStability();
								RatingStability(
									const BString& code,
									const BString& name,
									int64 ordering);
								RatingStability(
									const RatingStability& other);

			RatingStability&
								operator=(const RatingStability& other);
			bool				operator==(const RatingStability& other)
									const;
			bool				operator!=(const RatingStability& other)
									const;

			const BString&		Code() const
									{ return fCode; }
			const BString&		Name() const
									{ return fName; }
				int64			Ordering() const
									{ return fOrdering; }

			int					Compare(const RatingStability& other)
									const;
private:
			BString				fCode;
			BString				fName;
			int64				fOrdering;
};


typedef BReference<RatingStability> RatingStabilityRef;


extern bool IsRatingStabilityBefore(const RatingStabilityRef& rs1,
	const RatingStabilityRef& rs2);


#endif // RATING_STABILITY_H
