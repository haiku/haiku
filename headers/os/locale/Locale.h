#ifndef _B_LOCALE_H_
#define _B_LOCALE_H_


#include <LocaleBuild.h>

#include <Collator.h>
#include <Language.h>
#include <Country.h>


class BCatalog;
class BString;


class _IMPEXP_LOCALE BLocale {
	public:
		BLocale();
		~BLocale();

		BCollator *Collator() const { return fCollator; }
		BCountry *Country() const { return fCountry; }
		BLanguage *Language() const { return fLanguage; }

		// see definitions in LocaleStrings.h
		const char *GetString(uint32 id);

		void FormatString(char *target, size_t maxSize, char *fmt, ...);
		void FormatString(BString *, char *fmt, ...);
		void FormatDateTime(char *target, size_t maxSize, const char *fmt, time_t);
		void FormatDateTime(BString *, const char *fmt, time_t);

		// Country short-hands

		void FormatDate(char *target, size_t maxSize, time_t, bool longFormat);
		void FormatDate(BString *target, time_t, bool longFormat);
		void FormatTime(char *target, size_t maxSize, time_t, bool longFormat);
		void FormatTime(BString *target, time_t, bool longFormat);

		// Collator short-hands

		int StringCompare(const char *, const char *, int32 len = -1, int8 strength = B_COLLATE_DEFAULT) const;
		int StringCompare(const BString *, const BString *, int32 len = -1, int8 strength = B_COLLATE_DEFAULT) const;

		void GetSortKey(const char *string, BString *key);
		
		status_t GetAppCatalog(BCatalog *);

	protected:
		BCollator	*fCollator;
		BLanguage	*fLanguage;
		BCountry	*fCountry;
};

// global objects
extern _IMPEXP_LOCALE BLocale *be_locale;
extern _IMPEXP_LOCALE BLocaleRoster *be_locale_roster;

//----------------------------------------------------------------------
//--- country short-hands inlines ---

inline void 
BLocale::FormatDate(char *target, size_t maxSize, time_t timer, bool longFormat)
{
	fCountry->FormatDate(target, maxSize, timer, longFormat);
}


inline void 
BLocale::FormatDate(BString *target, time_t timer, bool longFormat)
{
	fCountry->FormatDate(target, timer, longFormat);
}


inline void 
BLocale::FormatTime(char *target, size_t maxSize, time_t timer, bool longFormat)
{
	fCountry->FormatTime(target, maxSize, timer, longFormat);
}


inline void 
BLocale::FormatTime(BString *target, time_t timer, bool longFormat)
{
	fCountry->FormatTime(target, timer, longFormat);
}


//--- locale short-hands inlines ---
//	#pragma mark -

inline int 
BLocale::StringCompare(const char *string1, const char *string2, int32 length, int8 strength) const
{
	return fCollator->Compare(string1, string2, length, strength);
}


inline int 
BLocale::StringCompare(const BString *string1, const BString *string2, int32 length, int8 strength) const
{
	return fCollator->Compare(string1->String(), string2->String(), length, strength);
}


inline void
BLocale::GetSortKey(const char *string, BString *key)
{
	fCollator->GetSortKey(string, key);
}

#endif	/* _B_LOCALE_H_ */
