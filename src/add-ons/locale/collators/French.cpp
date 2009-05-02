/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <Collator.h>
#include <UnicodeChar.h>
#include <String.h>
#include <Message.h>

#include <ctype.h>


class FrenchCollator : public BCollatorAddOn {
	struct compare_context {
		compare_context(bool ignorePunctuation)
			:
			inputA(ignorePunctuation),
			inputB(ignorePunctuation)
		{
		}

		typedef BCollatorAddOn::input_context input_context;

		const char		*a;
		const char		*b;
		input_context	inputA;
		input_context	inputB;
		size_t			length;
	};

	public:
		FrenchCollator();
		FrenchCollator(BMessage *archive);
		~FrenchCollator();

		virtual status_t GetSortKey(const char *string, BString *key, int8 strength,
						bool ignorePunctuation);
		virtual int Compare(const char *a, const char *b, int32 length, int8 strength,
						bool ignorePunctuation);

		// (un-)archiving API
		virtual status_t Archive(BMessage *archive, bool deep) const;
		static BArchivable *Instantiate(BMessage *archive);

	private:
		int CompareSecondary(compare_context &context);

		typedef BCollatorAddOn _inherited;
};


static const char *kSignature = "application/x-vnd.locale-collator.french";


inline char
uint32_to_char(uint32 c)
{
	if (c > 255)
		return 255;

	return (char)c;
}


//	#pragma mark -


FrenchCollator::FrenchCollator()
{
}


FrenchCollator::FrenchCollator(BMessage *archive)
	: BCollatorAddOn(archive)
{
}


FrenchCollator::~FrenchCollator()
{
}


int
FrenchCollator::CompareSecondary(compare_context &context)
{
	if (context.length-- <= 0)
		return 0;

	uint32 charA = BUnicodeChar::ToLower(GetNextChar(&context.a, context.inputA));
	uint32 charB = BUnicodeChar::ToLower(GetNextChar(&context.b, context.inputB));

	// the two strings does have the same size when we get here
	// (unfortunately, "length" is specified in bytes, not characters [if it was -1])
	if (charA == 0)
		return 0;

	int compare = CompareSecondary(context);
	if (compare != 0)
		return compare;

	return (int32)charA - (int32)charB;
}


status_t
FrenchCollator::GetSortKey(const char *string, BString *key, int8 strength,
	bool ignorePunctuation)
{
	if (strength == B_COLLATE_PRIMARY)
		return _inherited::GetSortKey(string, key, strength, ignorePunctuation);

	size_t length = strlen(string);

	if (strength > B_COLLATE_QUATERNARY) {
		key->SetTo(string, length);
		return B_OK;
			// what can we do about that?
	}

	if (strength >= B_COLLATE_QUATERNARY) {
		// the difference between tertiary and quaternary collation strength
		// are usually a different handling of punctuation characters
		ignorePunctuation = false;
	}

	size_t keyLength = PrimaryKeyLength(length) + length + 1;
		// the primary key + the secondary key + separator char
	if (strength != B_COLLATE_SECONDARY) {
		keyLength += length + 1;
			// the secondary key + the tertiary key + separator char
	}

	char *begin = key->LockBuffer(keyLength);
	if (begin == NULL)
		return B_NO_MEMORY;

	char *buffer = PutPrimaryKey(string, begin, length, ignorePunctuation);
	*buffer++ = '\01';
		// separator

	char *secondary = buffer;

	input_context context(ignorePunctuation);
	const char *source = string;
	uint32 c;
	for (uint32 i = 0; (c = GetNextChar(&source, context)) && i < length; i++) {
		*buffer++ = uint32_to_char(BUnicodeChar::ToLower(c));
			// we only support Latin-1 characters here
			// ToDo: this creates a non UTF-8 sort key - is that what we want?
	}

	// reverse key

	char *end = buffer - 1;
	while (secondary < end) {
		char c = secondary[0];

		*secondary++ = end[0];
		*end-- = c;
	}

	// apply tertiary collation if necessary

	if (strength != B_COLLATE_SECONDARY) {
		*buffer++ = '\01';
			// separator

		input_context context(ignorePunctuation);
		source = string;
		uint32 c;
		for (uint32 i = 0; (c = GetNextChar(&source, context)) && i < length; i++) {
			// ToDo: same problem as above, no UTF-8 key
			if (BUnicodeChar::IsLower(c))
				*buffer++ = uint32_to_char(BUnicodeChar::ToUpper(c));
			else
				*buffer++ = uint32_to_char(BUnicodeChar::ToLower(c));
		}
	}

	*buffer = '\0';
	key->UnlockBuffer(buffer - begin);

	return B_OK;
}


int
FrenchCollator::Compare(const char *a, const char *b, int32 length, int8 strength,
	bool ignorePunctuation)
{
	int compare = _inherited::Compare(a, b, length, B_COLLATE_PRIMARY, ignorePunctuation);
	if (strength == B_COLLATE_PRIMARY || compare != 0)
		return compare;
	else if (strength >= B_COLLATE_QUATERNARY) {
		// the difference between tertiary and quaternary collation strength
		// are usually a different handling of punctuation characters
		ignorePunctuation = false;
	}

	compare_context context(ignorePunctuation);
	context.a = a;
	context.b = b;
	context.length = length;

	switch (strength) {
		case B_COLLATE_SECONDARY:
			return CompareSecondary(context);

		case B_COLLATE_TERTIARY:
		case B_COLLATE_QUATERNARY:
		{
			// diacriticals can only change the order between equal strings
			int32 compare = Compare(a, b, length, B_COLLATE_SECONDARY, ignorePunctuation);
			if (compare != 0)
				return compare;

			for (int32 i = 0; i < length; i++) {
				uint32 charA = GetNextChar(&a, context.inputA);
				uint32 charB = GetNextChar(&b, context.inputB);

				// the two strings does have the same size when we get here
				if (charA == 0)
					return 0;

				if (charA != charB)
					return (int32)charB - (int32)charA;
			}
			return 0;
		}

		case B_COLLATE_IDENTICAL:
		default:
			return strncmp(a, b, length);
	}
}


status_t
FrenchCollator::Archive(BMessage *archive, bool deep) const
{
	status_t status = BArchivable::Archive(archive, deep);

	// add the add-on signature, so that the roster can load
	// us on demand!
	if (status == B_OK)
		status = archive->AddString("add_on", kSignature);

	return status;
}


BArchivable *
FrenchCollator::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "FrenchCollator"))
		return new FrenchCollator(archive);

	return NULL;
}


//	#pragma mark -


extern "C" BCollatorAddOn *
instantiate_collator(void)
{
	return new FrenchCollator();
}
