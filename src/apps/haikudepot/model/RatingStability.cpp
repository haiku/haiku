/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "RatingStability.h"

#include <Collator.h>

#include "LocaleUtils.h"
#include "Logger.h"


bool IsRatingStabilityBefore(const RatingStabilityRef& rs1,
	const RatingStabilityRef& rs2)
{
	if (rs1.Get() == NULL || rs2.Get() == NULL)
		HDFATAL("unexpected NULL reference in a referencable");
	return rs1.Get()->Compare(*(rs2.Get())) < 0;
}


RatingStability::RatingStability()
	:
	fCode(),
	fName(),
	fOrdering(0)
{
}


RatingStability::RatingStability(const BString& code,
		const BString& name, int64 ordering)
	:
	fCode(code),
	fName(name),
	fOrdering(ordering)
{
}


RatingStability::RatingStability(const RatingStability& other)
	:
	fCode(other.fCode),
	fName(other.fName),
	fOrdering(other.fOrdering)
{
}


RatingStability&
RatingStability::operator=(const RatingStability& other)
{
	fCode = other.fCode;
	fName = other.fName;
	fOrdering = other.fOrdering;
	return *this;
}


bool
RatingStability::operator==(const RatingStability& other) const
{
	return fCode == other.fCode && fName == other.fName
		&& fOrdering == other.fOrdering;
}


bool
RatingStability::operator!=(const RatingStability& other) const
{
	return !(*this == other);
}


int
RatingStability::Compare(const RatingStability& other) const
{
	int32 result = other.Ordering() - Ordering();
	if (0 == result)
		result = Code().Compare(other.Code());
	return result;
}
