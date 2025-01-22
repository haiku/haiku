/*
 * Copyright 2020-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "RatingStability.h"

#include <Collator.h>

#include "LocaleUtils.h"
#include "Logger.h"


bool
IsRatingStabilityRefLess(const RatingStabilityRef& rs1, const RatingStabilityRef& rs2)
{
	if (!rs1.IsSet() || !rs2.IsSet())
		debugger("illegal state in rating stability less");
	return *(rs1.Get()) < *(rs2.Get());
}


RatingStability::RatingStability()
	:
	fCode(),
	fName(),
	fOrdering(0)
{
}


RatingStability::RatingStability(const BString& code, const BString& name, int64 ordering)
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


bool
RatingStability::operator<(const RatingStability& other) const
{
	return Compare(other) < 0;
}


bool
RatingStability::operator==(const RatingStability& other) const
{
	return fCode == other.fCode && fName == other.fName && fOrdering == other.fOrdering;
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
