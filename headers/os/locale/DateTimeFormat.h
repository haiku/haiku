/*
 * Copyright 2010-2014, Haiku, Inc.
 * Distributed under the terms of the MIT Licence.
 */
#ifndef _B_DATE_TIME_FORMAT_H_
#define _B_DATE_TIME_FORMAT_H_


#include <DateFormat.h>
#include <Format.h>
#include <FormatParameters.h>
#include <TimeFormat.h>


class BString;


class BDateTimeFormat : public BFormat {
public:
								BDateTimeFormat();
								BDateTimeFormat(const BDateTimeFormat &other);
	virtual						~BDateTimeFormat();

								// formatting

			ssize_t				Format(char* target, const size_t maxSize,
									const time_t time,
									BDateFormatStyle dateStyle,
									BTimeFormatStyle timeStyle) const;
			status_t			Format(BString& buffer, const time_t time,
									BDateFormatStyle dateStyle,
									BTimeFormatStyle timeStyle,
									const BTimeZone* timeZone = NULL) const;

private:
			icu::DateFormat*	_CreateDateFormatter(
									const BString& format) const;
			icu::DateFormat*	_CreateTimeFormatter(
									const BString& format) const;
};


#endif	// _B_DATE_TIME_FORMAT_H_
