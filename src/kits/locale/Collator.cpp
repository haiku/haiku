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
#include <unicode/tblcoll.h>


BCollator::BCollator()
	:
	fFallbackICUCollator(NULL),
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
	fFallbackICUCollator(NULL),
	fStrength(strength),
	fIgnorePunctuation(ignorePunctuation)
{
	UErrorCode error = U_ZERO_ERROR;
	fICUCollator = Collator::createInstance(locale, error);
}


BCollator::BCollator(BMessage *archive)
	: BArchivable(archive),
	fICUCollator(NULL),
	fFallbackICUCollator(NULL),
	fIgnorePunctuation(true)
{
	int32 data;
	if (archive->FindInt32("loc:strength", &data) == B_OK)
		fStrength = (uint8)data;
	else
		fStrength = B_COLLATE_PRIMARY;

	if (archive->FindBool("loc:punctuation", &fIgnorePunctuation) != B_OK)
		fIgnorePunctuation = true;

	UErrorCode error = U_ZERO_ERROR;
	fFallbackICUCollator = static_cast<RuleBasedCollator*>
		(Collator::createInstance(error));

	ssize_t size;
	const void* buffer = NULL;
	if (archive->FindData("loc:collator", B_RAW_TYPE, &buffer, &size) == B_OK) {
		fICUCollator = new RuleBasedCollator((const uint8_t*)buffer, (int)size,
			fFallbackICUCollator, error);
		if (fICUCollator == NULL) {
			fICUCollator = fFallbackICUCollator;
				// Unarchiving failed, so we revert to the fallback collator
		}
	}
}


BCollator::~BCollator()
{
	delete fICUCollator;
	fICUCollator = NULL;
	delete fFallbackICUCollator;
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

	UErrorCode error = U_ZERO_ERROR;
	int size = static_cast<RuleBasedCollator*>(fICUCollator)->cloneBinary(NULL,
		0, error);
		// This WILL fail with U_BUFFER_OVERFLOW_ERROR. But we get the needed
		// size.
	error = U_ZERO_ERROR;
	uint8_t* buffer = (uint8_t*)malloc(size);
	static_cast<RuleBasedCollator*>(fICUCollator)->cloneBinary(buffer, size,
		error);

	if (status == B_OK && error == U_ZERO_ERROR)
		status = archive->AddData("loc:collator", B_RAW_TYPE, buffer, size);
	delete buffer;

	if (error == U_ZERO_ERROR)
		return status;
	else
		return B_ERROR;
}


BArchivable *
BCollator::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BCollator"))
		return new BCollator(archive);

	return NULL;
}


