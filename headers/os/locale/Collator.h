/*
 * Copyright 2003-2011, Haiku, Inc.
 * Distributed under the terms of the MIT Licence.
 */
#ifndef _COLLATOR_H_
#define _COLLATOR_H_


#include <Archivable.h>
#include <SupportDefs.h>


namespace icu {
	class Collator;
	class RuleBasedCollator;
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


// N.B.: This class is not multithread-safe, as Compare() and GetKey() change
//       the ICUCollator (the strength). So if you want to use a BCollator from
//       more than one thread, you need to protect it with a lock
class BCollator : public BArchivable {
public:
								BCollator();
								BCollator(const char* locale,
									int8 strength = B_COLLATE_PRIMARY,
									bool ignorePunctuation = false);
								BCollator(BMessage* archive);

								BCollator(const BCollator& other);

								~BCollator();

			BCollator&			operator=(const BCollator& source);

			void				SetDefaultStrength(int8 strength);
			int8				DefaultStrength() const;

			void				SetIgnorePunctuation(bool ignore);
			bool				IgnorePunctuation() const;

			status_t			GetSortKey(const char* string, BString* key,
									int8 strength = B_COLLATE_DEFAULT) const;

			int					Compare(const char* s1, const char* s2,
									int8 strength = B_COLLATE_DEFAULT) const;
			bool				Equal(const char* s1, const char* s2,
									int8 strength = B_COLLATE_DEFAULT) const;
			bool				Greater(const char* s1, const char* s2,
									int8 strength = B_COLLATE_DEFAULT) const;
			bool				GreaterOrEqual(const char* s1, const char* s2,
									int8 strength = B_COLLATE_DEFAULT) const;

								// (un-)archiving API
			status_t			Archive(BMessage* archive, bool deep) const;
	static	BArchivable*		Instantiate(BMessage* archive);

private:
			status_t			_SetStrength(int8 strength) const;

			mutable icu::Collator*	fICUCollator;
			int8				fDefaultStrength;
			bool				fIgnorePunctuation;
};


inline bool
BCollator::Equal(const char *s1, const char *s2, int8 strength) const
{
	return Compare(s1, s2, strength) == 0;
}


inline bool
BCollator::Greater(const char *s1, const char *s2, int8 strength) const
{
	return Compare(s1, s2, strength) > 0;
}


inline bool
BCollator::GreaterOrEqual(const char *s1, const char *s2, int8 strength) const
{
	return Compare(s1, s2, strength) >= 0;
}


#endif	/* _COLLATOR_H_ */
