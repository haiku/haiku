/*
 * Copyright 2010-2014, Haiku, Inc.
 * Distributed under the terms of the MIT Licence.
 */
#ifndef _B_DATE_FORMAT_H_
#define _B_DATE_FORMAT_H_


#include <DateTime.h>
#include <DateTimeFormat.h>
#include <Format.h>
#include <FormattingConventions.h>
#include <Language.h>
#include <Locker.h>


#ifndef U_ICU_NAMESPACE
  #define U_ICU_NAMESPACE icu
#endif
namespace U_ICU_NAMESPACE {
	class DateFormat;
}


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


class BDateFormat: public BFormat {
public:
								BDateFormat(const BLocale* locale = NULL);
								BDateFormat(const BLanguage& language,
									const BFormattingConventions& format);
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
			status_t			GetMonthName(int month, BString& outName);

								// parsing

			status_t			Parse(BString source, BDateFormatStyle style,
									BDate& output);

private:
			U_ICU_NAMESPACE::DateFormat*	_CreateDateFormatter(
									const BDateFormatStyle style) const;

};


#endif	// _B_DATE_FORMAT_H_
