/*
 * Copyright 2010-2014, Haiku, Inc.
 * Distributed under the terms of the MIT Licence.
 */
#ifndef _B_DATE_FORMAT_H_
#define _B_DATE_FORMAT_H_


#include <DateTimeFormat.h>
#include <FormattingConventions.h>
#include <Language.h>
#include <Locker.h>


class BString;
class BTimeZone;


enum BWeekday {
	B_WEEKDAY_MONDAY = 1,
	B_WEEKDAY_TUESDAY,
	B_WEEKDAY_WEDNESDAY,
	B_WEEKDAY_THURSDAY,
	B_WEEKDAY_FRIDAY,
	B_WEEKDAY_SATURDAY,
	B_WEEKDAY_SUNDAY,
};


class BDateFormat {
public:
								BDateFormat(const BLanguage* const,
									const BFormattingConventions* const);
								BDateFormat(const BDateFormat &other);
	virtual						~BDateFormat();

	static	const BDateFormat*	Default();

								// formatting

			ssize_t				Format(char* string, size_t maxSize,
									time_t time, BDateFormatStyle style) const;
			status_t			Format(BString* string, time_t time,
									BDateFormatStyle style,
									const BTimeZone* timeZone = NULL) const;
			status_t			Format(BString* string,
									int*& fieldPositions, int& fieldCount,
									time_t time, BDateFormatStyle style) const;

			status_t			GetFields(BDateElement*& fields,
									int& fieldCount, BDateFormatStyle style
									) const;

			status_t			GetStartOfWeek(BWeekday* weekday) const;

								// TODO parsing

private:
			icu::DateFormat*	_CreateDateFormatter(
									const BString& format) const;

	mutable	BLocker				fLock;
			BFormattingConventions	fConventions;
			BLanguage			fLanguage;
};


#endif	// _B_DATE_FORMAT_H_
