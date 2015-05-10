/*
 * Copyright 2010-2014, Haiku, Inc.
 * Distributed under the terms of the MIT Licence.
 */
#ifndef _B_TIME_FORMAT_H_
#define _B_TIME_FORMAT_H_


#include <DateTimeFormat.h>


class BString;

namespace BPrivate {
	class BTime;
}


class BTimeFormat : public BFormat {
public:
								BTimeFormat();
								BTimeFormat(const BLanguage& language,
									const BFormattingConventions& conventions);
								BTimeFormat(const BTimeFormat &other);
	virtual						~BTimeFormat();

			void				SetTimeFormat(BTimeFormatStyle style,
									const BString& format);

								// formatting

			ssize_t				Format(char* string, size_t maxSize,
									time_t time, BTimeFormatStyle style) const;
			status_t			Format(BString& string, const time_t time,
									const BTimeFormatStyle style,
									const BTimeZone* timeZone = NULL) const;
			status_t			Format(BString& string,
									int*& fieldPositions, int& fieldCount,
									time_t time, BTimeFormatStyle style) const;

			status_t			GetTimeFields(BDateElement*& fields,
									int& fieldCount, BTimeFormatStyle style
									) const;

								// parsing

			status_t			Parse(BString source, BTimeFormatStyle style,
									BPrivate::BTime& output);

private:
			U_ICU_NAMESPACE::DateFormat*	_CreateTimeFormatter(
									const BTimeFormatStyle style) const;
};


#endif	// _B_TIME_FORMAT_H_
