/*
* Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
* Copyright 2010, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
* Distributed under the terms of the MIT License.
*/


#include <Collator.h>
#include <UnicodeChar.h>
#include <String.h>
#include <Message.h>

#include <stdlib.h>
#include <typeinfo>
#include <ctype.h>

#include <unicode/coll.h>


BCollator::BCollator()
	:
	fStrength(B_COLLATE_PRIMARY),
	fIgnorePunctuation(true)
{
	// TODO: the collator construction will have to change; the default
	//	collator should be constructed by the Locale/LocaleRoster, so we
	//	only need a constructor where you specify all details

	UErrorCode error = U_ZERO_ERROR;
	fICUCollator = Collator::createInstance(error);
}


BCollator::BCollator(const char *locale, int8 strength,
	bool ignorePunctuation)
	:
	fStrength(strength),
	fIgnorePunctuation(ignorePunctuation)
{
	UErrorCode error = U_ZERO_ERROR;
	fICUCollator = Collator::createInstance(locale, error);
}


BCollator::BCollator(BMessage *archive)
	: BArchivable(archive),
	fICUCollator(NULL),
	fIgnorePunctuation(true)
{
#if HAIKU_TARGET_PLATFORM_HAIKU
	int32 data;
	if (archive->FindInt32("loc:strength", &data) == B_OK)
		fStrength = (uint8)data;
	else
		fStrength = B_COLLATE_PRIMARY;

	if (archive->FindBool("loc:punctuation", &fIgnorePunctuation) != B_OK)
		fIgnorePunctuation = true;

	// TODO : ICU collator ? or just store the locale name ?
#endif
}


BCollator::~BCollator()
{
	delete fICUCollator;
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
	// TODO : handle fIgnorePunctuation and strength
	if (strength == B_COLLATE_DEFAULT)
		strength = fStrength;

	int length = strlen(string);

	uint8_t* buffer = (uint8_t*)malloc(length * 2);
		// According to ICU documentation this should be enough in "most cases"
	if (buffer == NULL)
		return B_NO_MEMORY;

	int requiredSize = fICUCollator->getSortKey(UnicodeString(string, length),
		buffer, length * 2);
	if (requiredSize > length * 2) {
		buffer = (uint8_t*)realloc(buffer, requiredSize);
		if (buffer == NULL)
			return B_NO_MEMORY;
		fICUCollator->getSortKey(UnicodeString(string, length), buffer,
			requiredSize);
	}

	key->SetTo((char*)buffer);
	free(buffer);

	return B_OK;
}


int
BCollator::Compare(const char *a, const char *b, int32 length, int8 strength)
{
	// TODO : handle fIgnorePunctuation
	if (strength == B_COLLATE_DEFAULT)
		strength = fStrength;
	Collator::ECollationStrength icuStrength;
	switch(strength) {
		case B_COLLATE_PRIMARY:
			icuStrength = Collator::PRIMARY;
			break;
		case B_COLLATE_SECONDARY:
			icuStrength = Collator::SECONDARY;
			break;
		case B_COLLATE_TERTIARY:
		default:
			icuStrength = Collator::TERTIARY;
			break;
		case B_COLLATE_QUATERNARY:
			icuStrength = Collator::QUATERNARY;
			break;
		case B_COLLATE_IDENTICAL:
			icuStrength = Collator::IDENTICAL;
			break;
	}
	fICUCollator->setStrength(icuStrength);
	return fICUCollator->compare(a, b, length);
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

	/*
	BMessage collatorArchive;
	if (status == B_OK && deep
		&& typeid(*fCollator) != typeid(BCollatorAddOn)
			// only archive subclasses from BCollatorAddOn
		&& (status = fCollator->Archive(&collatorArchive, true)) == B_OK)
		status = archive->AddMessage("loc:collator", &collatorArchive);
	*/
	//TODO : archive fICUCollator

	return status;
}


BArchivable *
BCollator::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BCollator"))
		return new BCollator(archive);

	return NULL;
}


