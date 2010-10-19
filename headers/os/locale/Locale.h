/*
 * Copyright 2003-2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_LOCALE_H_
#define _B_LOCALE_H_


#include <Collator.h>
#include <Country.h>
#include <Language.h>
#include <Locker.h>


class BCatalog;
class BString;
class BTimeZone;


typedef enum {
	B_DATE_ELEMENT_INVALID = B_BAD_DATA,
	B_DATE_ELEMENT_YEAR = 0,
	B_DATE_ELEMENT_MONTH,
	B_DATE_ELEMENT_DAY,
	B_DATE_ELEMENT_AM_PM,
	B_DATE_ELEMENT_HOUR,
	B_DATE_ELEMENT_MINUTE,
	B_DATE_ELEMENT_SECOND
} BDateElement;

typedef enum {
	B_NUMBER_ELEMENT_INVALID = B_BAD_DATA,
	B_NUMBER_ELEMENT_INTEGER = 0,
	B_NUMBER_ELEMENT_FRACTIONAL,
	B_NUMBER_ELEMENT_CURRENCY
} BNumberElement;


class BLocale {
public:
								BLocale(const BLanguage* language = NULL,
									const BCountry* country = NULL);
								BLocale(const BLocale& other);
								~BLocale();

			BLocale&			operator=(const BLocale& other);

			status_t			GetCollator(BCollator* collator) const;
			status_t			GetLanguage(BLanguage* language) const;
			status_t			GetCountry(BCountry* country) const;

			void				SetCountry(const BCountry& newCountry);
			void				SetCollator(const BCollator& newCollator);
			void				SetLanguage(const BLanguage& newLanguage);

			bool				GetName(BString& name) const;

			// see definitions in LocaleStrings.h
			const char*			GetString(uint32 id) const;

			void				FormatString(char* target, size_t maxSize,
									char* fmt, ...) const;
			void				FormatString(BString* buffer, char* fmt,
									...) const;
			status_t			FormatDateTime(char* target, size_t maxSize,
									time_t time, bool longFormat) const;
			status_t			FormatDateTime(BString* buffer, time_t time,
									bool longFormat,
									const BTimeZone* timeZone = NULL) const;

								// Date

								// TODO: drop some of these once BDateFormat
								//       has been implemented!
			status_t			FormatDate(char* string, size_t maxSize,
									time_t time, bool longFormat) const;
			status_t			FormatDate(BString* string, time_t time,
									bool longFormat,
									const BTimeZone* timeZone = NULL) const;
			status_t			FormatDate(BString* string,
									int*& fieldPositions, int& fieldCount,
									time_t time, bool longFormat) const;
			status_t			GetDateFields(BDateElement*& fields,
									int& fieldCount, bool longFormat) const;
			status_t			GetDateFormat(BString&, bool longFormat) const;
			status_t			SetDateFormat(const char* formatString,
									bool longFormat = true);

			int					StartOfWeek() const;

								// Time

								// TODO: drop some of these once BTimeFormat
								//       has been implemented!
			status_t			FormatTime(char* string, size_t maxSize,
									time_t time, bool longFormat) const;
			status_t			FormatTime(BString* string, time_t time,
									bool longFormat,
									const BTimeZone* timeZone = NULL) const;
			status_t			FormatTime(BString* string,
									int*& fieldPositions, int& fieldCount,
									time_t time, bool longFormat) const;
			status_t			GetTimeFields(BDateElement*& fields,
									int& fieldCount, bool longFormat) const;

			status_t			SetTimeFormat(const char* formatString,
									bool longFormat = true);
			status_t			GetTimeFormat(BString& out,
									bool longFormat) const;

								// numbers

			status_t			FormatNumber(char* string, size_t maxSize,
									double value) const;
			status_t			FormatNumber(BString* string,
									double value) const;
			status_t			FormatNumber(char* string, size_t maxSize,
									int32 value) const;
			status_t			FormatNumber(BString* string,
									int32 value) const;

								// monetary

			ssize_t				FormatMonetary(char* string, size_t maxSize,
									double value) const;
			ssize_t				FormatMonetary(BString* string,
									double value) const;
			status_t			FormatMonetary(BString* string,
									int*& fieldPositions,
									BNumberElement*& fieldTypes,
									int& fieldCount, double value) const;
			status_t			GetCurrencySymbol(BString& result) const;

			// Collator short-hands
			int					StringCompare(const char* s1,
									const char* s2) const;
			int					StringCompare(const BString* s1,
									const BString* s2) const;

			void				GetSortKey(const char* string,
									BString* key) const;

private:
			void				_UpdateFormats();

	mutable	BLocker				fLock;
			BCollator			fCollator;
			BCountry			fCountry;
			BLanguage			fLanguage;

			BString				fLongDateFormat;
			BString				fShortDateFormat;
			BString				fLongTimeFormat;
			BString				fShortTimeFormat;

};


// global locale object
extern const BLocale* be_locale;


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
BLocale::GetSortKey(const char* string, BString* key) const
{
	fCollator.GetSortKey(string, key);
}


#endif	/* _B_LOCALE_H_ */
