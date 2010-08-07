/*
 * Copyright 2003-2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_LOCALE_H_
#define _B_LOCALE_H_


#include <Collator.h>
#include <Country.h>
#include <Language.h>


class BCatalog;
class BString;


class BLocale {
public:
								BLocale(const char* languageAndCountryCode
									= "en_US");
								BLocale(const BLocale& other);
								BLocale& operator=(const BLocale& other);
								~BLocale();

			const BCollator*	Collator() const { return &fCollator; }
			const BCountry*		Country() const { return &fCountry; }
			const BLanguage*	Language() const { return &fLanguage; }
			const char*			Code() const;
			bool				GetName(BString& name) const;

			void				SetCountry(const BCountry& newCountry);
			void				SetCollator(const BCollator& newCollator);
			void				SetLanguage(const char* languageCode);

			// see definitions in LocaleStrings.h
			const char*			GetString(uint32 id);

			void				FormatString(char* target, size_t maxSize,
									char* fmt, ...);
			void				FormatString(BString* buffer, char* fmt, ...);
			status_t			FormatDateTime(char* target, size_t maxSize,
									time_t time, bool longFormat);
			status_t			FormatDateTime(BString* buffer, time_t time,
									bool longFormat);

								// Date

			status_t			FormatDate(char* string, size_t maxSize,
									time_t time, bool longFormat);
			status_t			FormatDate(BString* string, time_t time,
									bool longFormat);
			status_t			FormatDate(BString* string,
									int*& fieldPositions, int& fieldCount,
									time_t time, bool longFormat);
			status_t			GetDateFields(BDateElement*& fields,
									int& fieldCount, bool longFormat) const;
			status_t			GetDateFormat(BString&, bool longFormat) const;
			status_t			SetDateFormat(const char* formatString,
									bool longFormat = true);

			int					StartOfWeek() const;

								// Time

			status_t			FormatTime(char* string, size_t maxSize,
									time_t time, bool longFormat);
			status_t			FormatTime(BString* string, time_t time,
									bool longFormat);
			status_t			FormatTime(BString* string,
									int*& fieldPositions, int& fieldCount,
									time_t time, bool longFormat);
			status_t			GetTimeFields(BDateElement*& fields,
									int& fieldCount, bool longFormat) const;

			status_t			SetTimeFormat(const char* formatString,
									bool longFormat = true);
			status_t			GetTimeFormat(BString& out,
									bool longFormat) const;

								// numbers

			status_t			FormatNumber(char* string, size_t maxSize,
									double value);
			status_t			FormatNumber(BString* string, double value);
			status_t			FormatNumber(char* string, size_t maxSize,
									int32 value);
			status_t			FormatNumber(BString* string, int32 value);

								// monetary

			ssize_t				FormatMonetary(char* string, size_t maxSize,
									double value);
			ssize_t				FormatMonetary(BString* string, double value);

			// Collator short-hands
			int					StringCompare(const char* s1,
									const char* s2) const;
			int					StringCompare(const BString* s1,
									const BString* s2) const;

			void				GetSortKey(const char* string,
									BString* key) const;

protected:
			BCollator			fCollator;
			BCountry			fCountry;
			BLanguage			fLanguage;

			icu_44::Locale*		fICULocale;
			BString				fLongDateFormat;
			BString				fShortDateFormat;
			BString				fLongTimeFormat;
			BString				fShortTimeFormat;
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
BLocale::GetSortKey(const char* string, BString* key) const
{
	fCollator.GetSortKey(string, key);
}


#endif	/* _B_LOCALE_H_ */
