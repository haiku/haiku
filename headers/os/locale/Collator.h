/*
 * Copyright 2003-2017, Haiku, Inc.
 * Distributed under the terms of the MIT Licence.
 */
#ifndef _COLLATOR_H_
#define _COLLATOR_H_


#include <Archivable.h>
#include <SupportDefs.h>


#ifndef U_ICU_NAMESPACE
  #define U_ICU_NAMESPACE icu
#endif
namespace U_ICU_NAMESPACE {
	class Collator;
};

class BString;


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
								BCollator(const char* locale,
									int8 strength = B_COLLATE_PRIMARY,
									bool ignorePunctuation = false);
								BCollator(BMessage* archive);

								BCollator(const BCollator& other);

								~BCollator();

			BCollator&			operator=(const BCollator& source);

			status_t			SetStrength(int8 strength) const;

			void				SetIgnorePunctuation(bool ignore);
			bool				IgnorePunctuation() const;

			status_t			SetNumericSorting(bool enable);

			status_t			GetSortKey(const char* string, BString* key)
									const;

			int					Compare(const char* s1, const char* s2)
									const;
			bool				Equal(const char* s1, const char* s2)
									const;
			bool				Greater(const char* s1, const char* s2)
									const;
			bool				GreaterOrEqual(const char* s1, const char* s2)
									const;

								// (un-)archiving API
			status_t			Archive(BMessage* archive, bool deep) const;
	static	BArchivable*		Instantiate(BMessage* archive);

private:

			mutable U_ICU_NAMESPACE::Collator*	fICUCollator;
			bool				fIgnorePunctuation;
};


inline bool
BCollator::Equal(const char *s1, const char *s2) const
{
	return Compare(s1, s2) == 0;
}


inline bool
BCollator::Greater(const char *s1, const char *s2) const
{
	return Compare(s1, s2) > 0;
}


inline bool
BCollator::GreaterOrEqual(const char *s1, const char *s2) const
{
	return Compare(s1, s2) >= 0;
}


#endif	/* _COLLATOR_H_ */
