/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <Collator.h>
#include <UnicodeChar.h>
#include <String.h>
#include <Message.h>

#include <typeinfo>
#include <ctype.h>


// conversion array for character ranges 192 - 223 & 224 - 255
static const uint8 kNoDiacrits[] = {
	'a','a','a','a','a','a','a',
	'c',
	'e','e','e','e',
	'i','i','i','i',
	240,	// eth
	'n',
	'o','o','o','o','o',
	247,	//
	'o',
	'u','u','u','u',
	'y',
	254,	// thorn
	'y'
};


static inline uint32
getPrimaryChar(uint32 c)
{
	if (c < 0x80)
		return tolower(c);

	// this automatically returns lowercase letters
	if (c >= 192 && c < 223)
		return kNoDiacrits[c - 192];
	if (c == 223)	// ß
		return 's';
	if (c >= 224 && c < 256)
		return kNoDiacrits[c - 224];

	return BUnicodeChar::ToLower(c);
}


BCollatorAddOn::input_context::input_context(bool ignorePunctuation)
	:
	ignore_punctuation(ignorePunctuation),
	next_char(0),
	reserved1(0),
	reserved2(0)
{
}


//	#pragma mark -


BCollator::BCollator()
	:
	fCollatorImage(B_ERROR),
	fStrength(B_COLLATE_PRIMARY),
	fIgnorePunctuation(true)
{
	// ToDo: the collator construction will have to change; the default
	//	collator should be constructed by the Locale/LocaleRoster, so we
	//	only need a constructor where you specify all details

	fCollator = new BCollatorAddOn();
}


BCollator::BCollator(BCollatorAddOn *collator, int8 strength, bool ignorePunctuation)
	:
	fCollator(collator),
	fCollatorImage(B_ERROR),
	fStrength(strength),
	fIgnorePunctuation(ignorePunctuation)
{
	if (collator == NULL)
		fCollator = new BCollatorAddOn();
}


BCollator::BCollator(BMessage *archive)
	: BArchivable(archive),
	fCollator(NULL),
	fCollatorImage(B_ERROR)
{
	int32 data;
	if (archive->FindInt32("loc:strength", &data) == B_OK)
		fStrength = (uint8)data;
	else
		fStrength = B_COLLATE_PRIMARY;

	if (archive->FindBool("loc:punctuation", &fIgnorePunctuation) != B_OK)
		fIgnorePunctuation = true;

	BMessage collatorArchive;
	if (archive->FindMessage("loc:collator", &collatorArchive) == B_OK) {
		BArchivable *unarchived = instantiate_object(&collatorArchive, &fCollatorImage);

		// do we really have a BCollatorAddOn here?
		fCollator = dynamic_cast<BCollatorAddOn *>(unarchived);
		if (fCollator == NULL)
			delete unarchived;
	}

	if (fCollator == NULL) {
		fCollator = new BCollatorAddOn();
		fCollatorImage = B_ERROR;
	}
}


BCollator::~BCollator()
{
	delete fCollator;

	if (fCollatorImage >= B_OK)
		unload_add_on(fCollatorImage);
}


void
BCollator::SetDefaultStrength(int8 strength)
{
	fStrength = strength;
}


int8
BCollator::DefaultStrength() const
{
	return fStrength;
}


void
BCollator::SetIgnorePunctuation(bool ignore)
{
	fIgnorePunctuation = ignore;
}


bool
BCollator::IgnorePunctuation() const
{
	return fIgnorePunctuation;
}


status_t
BCollator::GetSortKey(const char *string, BString *key, int8 strength)
{
	if (strength == B_COLLATE_DEFAULT)
		strength = fStrength;

	return fCollator->GetSortKey(string, key, strength, fIgnorePunctuation);
}


int
BCollator::Compare(const char *a, const char *b, int32 length, int8 strength)
{
	if (length == -1)	// match the whole string
		length = 0x7fffffff;

	return fCollator->Compare(a, b, length,
				strength == B_COLLATE_DEFAULT ? fStrength : strength, fIgnorePunctuation);
}


status_t 
BCollator::Archive(BMessage *archive, bool deep) const
{
	status_t status = BArchivable::Archive(archive, deep);
	if (status < B_OK)
		return status;

	if (status == B_OK)
		status = archive->AddInt32("loc:strength", fStrength);
	if (status == B_OK)
		status = archive->AddBool("loc:punctuation", fIgnorePunctuation);

	BMessage collatorArchive;
	if (status == B_OK && deep
		&& typeid(*fCollator) != typeid(BCollatorAddOn)
			// only archive subclasses from BCollatorAddOn
		&& (status = fCollator->Archive(&collatorArchive, true)) == B_OK)
		status = archive->AddMessage("loc:collator", &collatorArchive);

	return status;
}


BArchivable *
BCollator::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BCollator"))
		return new BCollator(archive);

	return NULL;
}


//	#pragma mark -


BCollatorAddOn::BCollatorAddOn()
{
}


BCollatorAddOn::BCollatorAddOn(BMessage *archive)
	: BArchivable(archive)
{
}


BCollatorAddOn::~BCollatorAddOn()
{
}


/** This returns the next Unicode character from the UTF-8 encoded
 *	input string, and bumps it to the next character.
 *	It will ignore punctuation if specified by the context, and
 *	might substitute characters if needed.
 */

uint32
BCollatorAddOn::GetNextChar(const char **string, input_context &context)
{
	uint32 c = context.next_char;
	if (c != 0) {
		context.next_char = 0;
		return c;
	}

	do {
		c = BUnicodeChar::FromUTF8(string);
	} while (context.ignore_punctuation
		&& (BUnicodeChar::IsPunctuation(c) || BUnicodeChar::IsSpace(c)));

	if (c == 223) {
		context.next_char = 's';
		return 's';
	}

	return c;
}


/** Fills the specified buffer with the primary sort key. The buffer
 *	has to be long enough to hold the key.
 *	It returns the position in the buffer immediately after the key;
 *	it does not add a terminating null byte!
 */

char *
BCollatorAddOn::PutPrimaryKey(const char *string, char *buffer, int32 length,
	bool ignorePunctuation)
{
	input_context context(ignorePunctuation);

	uint32 c;
	for (int32 i = 0; (c = GetNextChar(&string, context)) != 0 && i < length; i++) {
		if (c < 0x80)
			*buffer++ = tolower(c);
		else
			BUnicodeChar::ToUTF8(getPrimaryChar(c), &buffer);
	}

	return buffer;
}


size_t 
BCollatorAddOn::PrimaryKeyLength(size_t length)
{
	return length * 2;
		// the primary key needs to make space for doubled characters (like 'ß')
}


status_t 
BCollatorAddOn::GetSortKey(const char *string, BString *key, int8 strength,
	bool ignorePunctuation)
{
	if (strength >= B_COLLATE_QUATERNARY) {
		// the difference between tertiary and quaternary collation strength
		// are usually a different handling of punctuation characters
		ignorePunctuation = false;
	}

	size_t length = strlen(string);

	switch (strength) {
		case B_COLLATE_PRIMARY:
		{
			char *begin = key->LockBuffer(PrimaryKeyLength(length));
			if (begin == NULL)
				return B_NO_MEMORY;

			char *end = PutPrimaryKey(string, begin, length, ignorePunctuation);
			*end = '\0';

			key->UnlockBuffer(end - begin);
			break;
		}

		case B_COLLATE_SECONDARY:
		{
			char *begin = key->LockBuffer(PrimaryKeyLength(length) + length + 1);
				// the primary key + the secondary key + separator char
			if (begin == NULL)
				return B_NO_MEMORY;

			char *buffer = PutPrimaryKey(string, begin, length, ignorePunctuation);
			*buffer++ = '\01';
				// separator

			input_context context(ignorePunctuation);
			uint32 c;
			for (uint32 i = 0; (c = GetNextChar(&string, context)) && i < length; i++) {
				if (c < 0x80)
					*buffer++ = tolower(c);
				else
					BUnicodeChar::ToUTF8(BUnicodeChar::ToLower(c), &buffer);
			}
			*buffer = '\0';

			key->UnlockBuffer(buffer - begin);
			break;
		}

		case B_COLLATE_TERTIARY:
		case B_COLLATE_QUATERNARY:
		{
			char *begin = key->LockBuffer(PrimaryKeyLength(length) + length + 1);
				// the primary key + the tertiary key + separator char
			if (begin == NULL)
				return B_NO_MEMORY;

			char *buffer = PutPrimaryKey(string, begin, length, ignorePunctuation);
			*buffer++ = '\01';
				// separator

			input_context context(ignorePunctuation);
			uint32 c;
			for (uint32 i = 0; (c = GetNextChar(&string, context)) && i < length; i++) {
				BUnicodeChar::ToUTF8(c, &buffer);
			}
			*buffer = '\0';

			key->UnlockBuffer(buffer + length - begin);
			break;
		}

		case B_COLLATE_IDENTICAL:
		default:
			key->SetTo(string, length);
				// is there any way to check if BString::SetTo() actually succeeded?
			break;
	}
	return B_OK;
}


int 
BCollatorAddOn::Compare(const char *a, const char *b, int32 length, int8 strength,
	bool ignorePunctuation)
{
	if (strength >= B_COLLATE_QUATERNARY) {
		// the difference between tertiary and quaternary collation strength
		// are usually a different handling of punctuation characters
		ignorePunctuation = false;
	}

	input_context contextA(ignorePunctuation);
	input_context contextB(ignorePunctuation);

	switch (strength) {
		case B_COLLATE_PRIMARY:
		{
			for (int32 i = 0; i < length; i++) {
				uint32 charA = GetNextChar(&a, contextA);
				uint32 charB = GetNextChar(&b, contextB);
				if (charA == 0)
					return charB == 0 ? 0 : -(int32)charB;
				else if (charB == 0)
					return (int32)charA;

				charA = getPrimaryChar(charA);
				charB = getPrimaryChar(charB);

				if (charA != charB)
					return (int32)charA - (int32)charB;
			}
			return 0;
		}

		case B_COLLATE_SECONDARY:
		{
			// diacriticals can only change the order between equal strings
			int32 compare = Compare(a, b, length, B_COLLATE_PRIMARY, ignorePunctuation);
			if (compare != 0)
				return compare;

			for (int32 i = 0; i < length; i++) {
				uint32 charA = BUnicodeChar::ToLower(GetNextChar(&a, contextA));
				uint32 charB = BUnicodeChar::ToLower(GetNextChar(&b, contextB));

				// the two strings does have the same size when we get here
				if (charA == 0)
					return 0;

				if (charA != charB)
					return (int32)charA - (int32)charB;
			}
			return 0;
		}

		case B_COLLATE_TERTIARY:
		case B_COLLATE_QUATERNARY:
		{
			// diacriticals can only change the order between equal strings
			int32 compare = Compare(a, b, length, B_COLLATE_PRIMARY, ignorePunctuation);
			if (compare != 0)
				return compare;

			for (int32 i = 0; i < length; i++) {
				uint32 charA = GetNextChar(&a, contextA);
				uint32 charB = GetNextChar(&b, contextB);

				// the two strings does have the same size when we get here
				if (charA == 0)
					return 0;

				if (charA != charB)
					return (int32)charA - (int32)charB;
			}
			return 0;
		}

		case B_COLLATE_IDENTICAL:
		default:
			return strncmp(a, b, length);
	}
}


status_t 
BCollatorAddOn::Archive(BMessage *archive, bool deep) const
{
	return BArchivable::Archive(archive, deep);
}


BArchivable *
BCollatorAddOn::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BCollatorAddOn"))
		return new BCollatorAddOn(archive);

	return NULL;
}

