/*
 * Copyright 2010-2014, Haiku, Inc.
 * Distributed under the terms of the MIT Licence.
 */
#ifndef _B_DATE_FORMAT_H_
#define _B_DATE_FORMAT_H_


#include <DateTime.h>
#include <Format.h>
#include <FormattingConventions.h>
#include <Language.h>
#include <Locker.h>


namespace icu {
	class DateFormat;
}


class BString;
class BTimeZone;


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

enum BWeekday {
	B_WEEKDAY_MONDAY = 1,
	B_WEEKDAY_TUESDAY,
	B_WEEKDAY_WEDNESDAY,
	B_WEEKDAY_THURSDAY,
	B_WEEKDAY_FRIDAY,
	B_WEEKDAY_SATURDAY,
	B_WEEKDAY_SUNDAY,
};


class BDateFormat: public BFormat {
public:
								BDateFormat(
									const BLanguage* const language = NULL,
									const BFormattingConventions* const
										format = NULL);
								BDateFormat(const BDateFormat &other);
	virtual						~BDateFormat();

			status_t			GetDateFormat(BDateFormatStyle style,
									BString& outFormat) const;
			void				SetDateFormat(BDateFormatStyle style,
									const BString& format);

								// formatting

			ssize_t				Format(char* string, const size_t maxSize,
									const time_t time,
									const BDateFormatStyle style) const;
			status_t			Format(BString& string, const time_t time,
									const BDateFormatStyle style,
									const BTimeZone* timeZone = NULL) const;
			status_t			Format(BString& string, const BDate& time,
									const BDateFormatStyle style,
									const BTimeZone* timeZone = NULL) const;
			status_t			Format(BString& string,
									int*& fieldPositions, int& fieldCount,
									const time_t time,
									const BDateFormatStyle style) const;

			status_t			GetFields(BDateElement*& fields,
									int& fieldCount, BDateFormatStyle style
									) const;

			status_t			GetStartOfWeek(BWeekday* weekday) const;

								// TODO parsing

private:
			icu::DateFormat*	_CreateDateFormatter(
									const BString& format) const;

};


#endif	// _B_DATE_FORMAT_H_
