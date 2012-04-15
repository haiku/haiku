/*
 * Copyright 2003-2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_LOCALE_H_
#define _B_LOCALE_H_


#include <Collator.h>
#include <FormattingConventions.h>
#include <Language.h>
#include <Locker.h>


namespace icu {
	class DateFormat;
}


class BCatalog;
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

enum BNumberElement {
	B_NUMBER_ELEMENT_INVALID = B_BAD_DATA,
	B_NUMBER_ELEMENT_INTEGER = 0,
	B_NUMBER_ELEMENT_FRACTIONAL,
	B_NUMBER_ELEMENT_CURRENCY
};


// TODO: move this to BCalendar (should we ever have that) or BDate
enum BWeekday {
	B_WEEKDAY_MONDAY = 1,
	B_WEEKDAY_TUESDAY,
	B_WEEKDAY_WEDNESDAY,
	B_WEEKDAY_THURSDAY,
	B_WEEKDAY_FRIDAY,
	B_WEEKDAY_SATURDAY,
	B_WEEKDAY_SUNDAY,
};


class BLocale {
public:
								BLocale(const BLanguage* language = NULL,
									const BFormattingConventions* conventions
										= NULL);
								BLocale(const BLocale& other);
								~BLocale();

	static	const BLocale*		Default();

			BLocale&			operator=(const BLocale& other);

			status_t			GetCollator(BCollator* collator) const;
			status_t			GetLanguage(BLanguage* language) const;
			status_t			GetFormattingConventions(
									BFormattingConventions* conventions) const;

			void				SetFormattingConventions(
									const BFormattingConventions& conventions);
			void				SetCollator(const BCollator& newCollator);
			void				SetLanguage(const BLanguage& newLanguage);

			// see definitions in LocaleStrings.h
			const char*			GetString(uint32 id) const;

			void				FormatString(char* target, size_t maxSize,
									char* fmt, ...) const;
			void				FormatString(BString* buffer, char* fmt,
									...) const;

								// DateTime

								// TODO: drop some of these once BDateTimeFormat
								//       has been implemented!
			ssize_t				FormatDateTime(char* target, size_t maxSize,
									time_t time, BDateFormatStyle dateStyle,
									BTimeFormatStyle timeStyle) const;
			status_t			FormatDateTime(BString* buffer, time_t time,
									BDateFormatStyle dateStyle,
									BTimeFormatStyle timeStyle,
									const BTimeZone* timeZone = NULL) const;

								// Date

								// TODO: drop some of these once BDateFormat
								//       has been implemented!
			ssize_t				FormatDate(char* string, size_t maxSize,
									time_t time, BDateFormatStyle style) const;
			status_t			FormatDate(BString* string, time_t time,
									BDateFormatStyle style,
									const BTimeZone* timeZone = NULL) const;
			status_t			FormatDate(BString* string,
									int*& fieldPositions, int& fieldCount,
									time_t time, BDateFormatStyle style) const;
			status_t			GetDateFields(BDateElement*& fields,
									int& fieldCount, BDateFormatStyle style
									) const;

			status_t			GetStartOfWeek(BWeekday* weekday) const;

								// Time

								// TODO: drop some of these once BTimeFormat
								//       has been implemented!
			ssize_t				FormatTime(char* string, size_t maxSize,
									time_t time, BTimeFormatStyle style) const;
			ssize_t				FormatTime(char* string, size_t maxSize,
									time_t time, BString format) const;
			status_t			FormatTime(BString* string, time_t time,
									BTimeFormatStyle style,
									const BTimeZone* timeZone = NULL) const;
			status_t			FormatTime(BString* string, time_t time,
									BString format,
									const BTimeZone* timeZone) const;
			status_t			FormatTime(BString* string,
									int*& fieldPositions, int& fieldCount,
									time_t time, BTimeFormatStyle style) const;
			status_t			GetTimeFields(BDateElement*& fields,
									int& fieldCount, BTimeFormatStyle style
									) const;

								// numbers

			ssize_t				FormatNumber(char* string, size_t maxSize,
									double value) const;
			status_t			FormatNumber(BString* string,
									double value) const;
			ssize_t				FormatNumber(char* string, size_t maxSize,
									int32 value) const;
			status_t			FormatNumber(BString* string,
									int32 value) const;

								// monetary

			ssize_t				FormatMonetary(char* string, size_t maxSize,
									double value) const;
			status_t			FormatMonetary(BString* string,
									double value) const;

			// Collator short-hands
			int					StringCompare(const char* s1,
									const char* s2) const;
			int					StringCompare(const BString* s1,
									const BString* s2) const;

			void				GetSortKey(const char* string,
									BString* sortKey) const;

private:
			icu::DateFormat*	_CreateDateFormatter(
									const BString& format) const;
			icu::DateFormat*	_CreateTimeFormatter(
									const BString& format) const;

	mutable	BLocker				fLock;
			BCollator			fCollator;
			BFormattingConventions	fConventions;
			BLanguage			fLanguage;
};


//--- collator short-hands inlines ---
//	#pragma mark -

inline int
BLocale::StringCompare(const char* s1, const char* s2) const
{
	return fCollator.Compare(s1, s2);
}


inline int
BLocale::StringCompare(const BString* s1, const BString* s2) const
{
	return fCollator.Compare(s1->String(), s2->String());
}


inline void
BLocale::GetSortKey(const char* string, BString* sortKey) const
{
	fCollator.GetSortKey(string, sortKey);
}


#endif	/* _B_LOCALE_H_ */
