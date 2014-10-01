/*
 * Copyright 2003-2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_LOCALE_H_
#define _B_LOCALE_H_


#include <Collator.h>
#include <DateTimeFormat.h>
#include <FormattingConventions.h>
#include <Language.h>
#include <Locker.h>


namespace icu {
	class DateFormat;
}


class BCatalog;
class BDateFormat;
class BString;
class BTimeZone;


enum BNumberElement {
	B_NUMBER_ELEMENT_INVALID = B_BAD_DATA,
	B_NUMBER_ELEMENT_INTEGER = 0,
	B_NUMBER_ELEMENT_FRACTIONAL,
	B_NUMBER_ELEMENT_CURRENCY
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
