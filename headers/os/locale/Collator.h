/*
 * Copyright 2003-2010, Haiku, Inc.
 * Distributed under the terms of the MIT Licence.
*/


#ifndef _COLLATOR_H_
#define _COLLATOR_H_


#include <SupportDefs.h>
#include <Archivable.h>


namespace icu_44 {
	class Collator;
};


class BString;
class BCollatorAddOn;


enum collator_strengths {
	B_COLLATE_DEFAULT = -1,

	B_COLLATE_PRIMARY = 1,		// e.g.: no diacritical differences, e = é
	B_COLLATE_SECONDARY,		// diacritics are different from their base
								// characters, a != ä
	B_COLLATE_TERTIARY,			// case sensitive comparison
	B_COLLATE_QUATERNARY,

	B_COLLATE_IDENTICAL = 127	// Unicode value
};


class BCollator : public BArchivable {
	public:
		BCollator();
		BCollator(const char *locale, int8 strength, bool ignorePunctuation);
		BCollator(BMessage *archive);
		~BCollator();

		void SetDefaultStrength(int8 strength);
		int8 DefaultStrength() const;

		void SetIgnorePunctuation(bool ignore);
		bool IgnorePunctuation() const;

		status_t GetSortKey(const char *string, BString *key,
			int8 strength = B_COLLATE_DEFAULT);

		int Compare(const char *, const char *, int32 len = -1,
			int8 strength = B_COLLATE_DEFAULT);
		bool Equal(const char *, const char *, int32 len = -1,
			int8 strength = B_COLLATE_DEFAULT);
		bool Greater(const char *, const char *, int32 len = -1,
			int8 strength = B_COLLATE_DEFAULT);
		bool GreaterOrEqual(const char *, const char *, int32 len = -1,
			int8 strength = B_COLLATE_DEFAULT);

		// (un-)archiving API
		status_t Archive(BMessage *archive, bool deep) const;
		static BArchivable *Instantiate(BMessage *archive);

	private:
		icu_44::Collator* fICUCollator;
		int8			fStrength;
		bool			fIgnorePunctuation;
};


inline bool
BCollator::Equal(const char *s1, const char *s2, int32 len, int8 strength)
{
	return Compare(s1, s2, len, strength) == 0;
}


inline bool
BCollator::Greater(const char *s1, const char *s2, int32 len, int8 strength)
{
	return Compare(s1, s2, len, strength) > 0;
}


inline bool
BCollator::GreaterOrEqual(const char *s1, const char *s2, int32 len,
	int8 strength)
{
	return Compare(s1, s2, len, strength) >= 0;
}


#endif	/* _COLLATOR_H_ */
