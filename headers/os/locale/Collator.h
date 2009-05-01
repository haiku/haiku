#ifndef _COLLATOR_H_
#define _COLLATOR_H_


#include <SupportDefs.h>
#include <Archivable.h>

#include <LocaleBuild.h>

class BString;
class BCollatorAddOn;


enum collator_strengths {
	B_COLLATE_DEFAULT = -1,

	B_COLLATE_PRIMARY = 1,		// e.g.: no diacritical differences, e = é
	B_COLLATE_SECONDARY,		// diacritics are different from their base characters, a != ä
	B_COLLATE_TERTIARY,			// case sensitive comparison
	B_COLLATE_QUATERNARY,

	B_COLLATE_IDENTICAL = 127	// Unicode value
};


class _IMPEXP_LOCALE BCollator : public BArchivable {
	public:
		BCollator();
		BCollator(BCollatorAddOn *collator, int8 strength, bool ignorePunctuation);
		BCollator(BMessage *archive);
		~BCollator();

		void SetDefaultStrength(int8 strength);
		int8 DefaultStrength() const;

		void SetIgnorePunctuation(bool ignore);
		bool IgnorePunctuation() const;

		status_t GetSortKey(const char *string, BString *key, int8 strength = B_COLLATE_DEFAULT);

		int Compare(const char *, const char *, int32 len = -1, int8 strength = B_COLLATE_DEFAULT);
		bool Equal(const char *, const char *, int32 len = -1, int8 strength = B_COLLATE_DEFAULT);
		bool Greater(const char *, const char *, int32 len = -1, int8 strength = B_COLLATE_DEFAULT);
		bool GreaterOrEqual(const char *, const char *, int32 len = -1, int8 strength = B_COLLATE_DEFAULT);

		// (un-)archiving API
		status_t Archive(BMessage *archive, bool deep) const;
		static BArchivable *Instantiate(BMessage *archive);

	private:
		BCollatorAddOn	*fCollator;
		image_id		fCollatorImage;
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
BCollator::GreaterOrEqual(const char *s1, const char *s2, int32 len, int8 strength)
{
	return Compare(s1, s2, len, strength) >= 0;
}


/************************************************************************/

// For BCollator add-on implementations:

class _IMPEXP_LOCALE BCollatorAddOn : public BArchivable {
	public:
		BCollatorAddOn();
		BCollatorAddOn(BMessage *archive);
		virtual ~BCollatorAddOn();

		virtual status_t GetSortKey(const char *string, BString *key, int8 strength,
						bool ignorePunctuation);
		virtual int Compare(const char *a, const char *b, int32 length, int8 strength,
						bool ignorePunctuation);

		// (un-)archiving API
		virtual status_t Archive(BMessage *archive, bool deep) const;
		static BArchivable *Instantiate(BMessage *archive);

	protected:
		struct input_context {
			input_context(bool ignorePunctuation);

			bool	ignore_punctuation;
			uint32	next_char;
			int32	reserved1;
			int32	reserved2;
		};

		virtual uint32 GetNextChar(const char **string, input_context &context);
		virtual size_t PrimaryKeyLength(size_t length);
		virtual char *PutPrimaryKey(const char *string, char *buffer, int32 length,
						bool ignorePunctuation);
};

// If your add-on should work with the standard tool to create a language, it
// should export that function. However, once the language file has been written
// only the archived collator is used, and that function won't be called anymore.
extern "C" _IMPEXP_LOCALE BCollatorAddOn *instantiate_collator(void);

#endif	/* _COLLATOR_H_ */
