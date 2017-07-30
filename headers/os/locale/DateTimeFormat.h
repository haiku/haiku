/*
 * Copyright 2010-2014, Haiku, Inc.
 * Distributed under the terms of the MIT Licence.
 */
#ifndef _B_DATE_TIME_FORMAT_H_
#define _B_DATE_TIME_FORMAT_H_


#include <Format.h>


class BString;
class BTimeZone;


enum BDateElement {
	B_DATE_ELEMENT_INVALID = 0,
	B_DATE_ELEMENT_YEAR = 1 << 0,
	B_DATE_ELEMENT_MONTH = 1 << 1,
	B_DATE_ELEMENT_WEEKDAY = 1 << 2,
	B_DATE_ELEMENT_DAY = 1 << 3,
	B_DATE_ELEMENT_AM_PM = 1 << 4,
	B_DATE_ELEMENT_HOUR = 1 << 5,
	B_DATE_ELEMENT_MINUTE = 1 << 6,
	B_DATE_ELEMENT_SECOND = 1 << 7,
	B_DATE_ELEMENT_TIMEZONE = 1 << 8
};



class BDateTimeFormat : public BFormat {
public:
								BDateTimeFormat(const BLocale* locale = NULL);
								BDateTimeFormat(const BLanguage& language,
									const BFormattingConventions& conventions);
								BDateTimeFormat(const BDateTimeFormat &other);
	virtual						~BDateTimeFormat();

			void				SetDateTimeFormat(BDateFormatStyle dateStyle,
									BTimeFormatStyle timeStyle,
									int32 elements);

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
			U_ICU_NAMESPACE::DateFormat*	_CreateDateTimeFormatter(
									const BString& format) const;
};


#endif	// _B_DATE_TIME_FORMAT_H_
