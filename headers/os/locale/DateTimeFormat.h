/*
 * Copyright 2010-2014, Haiku, Inc.
 * Distributed under the terms of the MIT Licence.
 */
#ifndef _B_DATE_TIME_FORMAT_H_
#define _B_DATE_TIME_FORMAT_H_


#include <Format.h>
#include <FormatParameters.h>


class BString;


enum BDateElement {
	B_DATE_ELEMENT_INVALID = B_BAD_DATA,
	B_DATE_ELEMENT_YEAR = 0,
	B_DATE_ELEMENT_MONTH,
	B_DATE_ELEMENT_DAY,
	B_DATE_ELEMENT_AM_PM,
	B_DATE_ELEMENT_HOUR,
	B_DATE_ELEMENT_MINUTE,
	B_DATE_ELEMENT_SECOND
};


class BDateTimeFormat : public BFormat {
public:
								BDateTimeFormat();
								BDateTimeFormat(const BDateTimeFormat &other);
	virtual						~BDateTimeFormat();

								// formatting

								// no-frills version: Simply appends the
								// formatted date to the string buffer.
								// Can fail only with B_NO_MEMORY or
								// B_BAD_VALUE.
	virtual	status_t 			Format(bigtime_t value, BString* buffer) const;

								// TODO: ... basically, all of it!
};


#endif	// _B_DATE_TIME_FORMAT_H_
