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
								BLocale();
								~BLocale();

			const BCollator*	Collator() const { return &fCollator; }
			const BCountry*		Country() const { return &fCountry; }
			const BLanguage*	Language() const { return &fLanguage; }

			// see definitions in LocaleStrings.h
			const char*			GetString(uint32 id);

			void				FormatString(char* target, size_t maxSize,
									char* fmt, ...);
			void				FormatString(BString* buffer, char* fmt, ...);
			void				FormatDateTime(char* target, size_t maxSize,
									const char* fmt, time_t value);
			void				FormatDateTime(BString* buffer, const char* fmt,
									time_t value);

			// Country short-hands, TODO: all these should go, once the
			// Date...Format classes are done
			void				FormatDate(char* target, size_t maxSize,
									time_t value, bool longFormat);
			void				FormatDate(BString* target, time_t value,
									bool longFormat);
			void				FormatTime(char* target, size_t maxSize,
									time_t value, bool longFormat);
			void				FormatTime(BString* target, time_t value,
									bool longFormat);

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
};


// global objects
extern BLocale* be_locale;


//----------------------------------------------------------------------
//--- country short-hands inlines ---
inline void
BLocale::FormatDate(char* target, size_t maxSize, time_t timer, bool longFormat)
{
	fCountry.FormatDate(target, maxSize, timer, longFormat);
}


inline void
BLocale::FormatDate(BString* target, time_t timer, bool longFormat)
{
	fCountry.FormatDate(target, timer, longFormat);
}


inline void
BLocale::FormatTime(char* target, size_t maxSize, time_t timer, bool longFormat)
{
	fCountry.FormatTime(target, maxSize, timer, longFormat);
}


inline void
BLocale::FormatTime(BString* target, time_t timer, bool longFormat)
{
	fCountry.FormatTime(target, timer, longFormat);
}


//--- locale short-hands inlines ---
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
