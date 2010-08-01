/*
 * Copyright 2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_DURATION_FORMAT_H_
#define _B_DURATION_FORMAT_H_


#include <DateTimeFormat.h>
#include <String.h>
#include <TimeUnitFormat.h>


class BDurationFormat : public BFormat {
public:
			status_t			Format(bigtime_t startValue,
									bigtime_t stopValue, BString* buffer,
									time_unit_style style = B_TIME_UNIT_FULL,
									const BString& separator = ", "
									) const;
};


#endif
