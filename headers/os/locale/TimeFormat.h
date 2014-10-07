/*
 * Copyright 2010-2014, Haiku, Inc.
 * Distributed under the terms of the MIT Licence.
 */
#ifndef _B_TIME_FORMAT_H_
#define _B_TIME_FORMAT_H_

#include <DateTimeFormat.h>


class BString;


class BTimeFormat : public BFormat {
public:
								BTimeFormat(
									const BLanguage* const language = NULL,
									const BFormattingConventions* const
										format = NULL);
								BTimeFormat(const BTimeFormat &other);
	virtual						~BTimeFormat();

								// formatting

			ssize_t				Format(char* string, size_t maxSize,
									time_t time, BTimeFormatStyle style) const;
			ssize_t				Format(char* string, size_t maxSize,
									time_t time, BString format) const;
			status_t			Format(BString& string, const time_t time,
									const BTimeFormatStyle style,
									const BTimeZone* timeZone = NULL) const;
			status_t			Format(BString& string, const time_t time,
									const BString format,
									const BTimeZone* timeZone) const;
			status_t			Format(BString& string,
									int*& fieldPositions, int& fieldCount,
									time_t time, BTimeFormatStyle style) const;

			status_t			GetTimeFields(BDateElement*& fields,
									int& fieldCount, BTimeFormatStyle style
									) const;

								// TODO parsing
private:
			icu::DateFormat*	_CreateTimeFormatter(
									const BString& format) const;
};


#endif	// _B_TIME_FORMAT_H_
