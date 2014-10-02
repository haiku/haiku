/*
 * Copyright 2003-2014, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_LOCALE_H_
#define _B_LOCALE_H_


#include <Collator.h>
#include <FormattingConventions.h>
#include <Language.h>
#include <Locker.h>


class BCatalog;
class BString;


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

			// Collator short-hands
			int					StringCompare(const char* s1,
									const char* s2) const;
			int					StringCompare(const BString* s1,
									const BString* s2) const;

			void				GetSortKey(const char* string,
									BString* sortKey) const;

private:
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
