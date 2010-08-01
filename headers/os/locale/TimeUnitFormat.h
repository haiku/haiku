/*
 * Copyright 2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_TIME_UNIT_FORMAT_H_
#define _B_TIME_UNIT_FORMAT_H_


#include <DateTimeFormat.h>
#include <SupportDefs.h>


class BString;


enum time_unit_style {
	B_TIME_UNIT_ABBREVIATED,	// e.g. '5 hrs.'
	B_TIME_UNIT_FULL,			// e.g. '5 hours'
};


enum time_unit_element {
	B_TIME_UNIT_YEAR,
	B_TIME_UNIT_MONTH,
	B_TIME_UNIT_WEEK,
	B_TIME_UNIT_DAY,
	B_TIME_UNIT_HOUR,
	B_TIME_UNIT_MINUTE,
	B_TIME_UNIT_SECOND,

	B_TIME_UNIT_LAST = B_TIME_UNIT_SECOND
};


class BTimeUnitFormat : public BFormat {
public:
			status_t			Format(int32 value, time_unit_element unit,
									BString* buffer,
									time_unit_style style = B_TIME_UNIT_FULL
									) const;
};


#endif
