/*
 * Copyright (c) 2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 * 		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <TimeZone.h>

#include <new>

#include <unicode/locid.h>
#include <unicode/timezone.h>
#include <ICUWrapper.h>

#include <Language.h>


const char* BTimeZone::kNameOfGmtZone = "GMT";


static const BString skEmptyString;


static const uint32 skNameField 					= 1U << 0;
static const uint32 skDaylightSavingNameField 		= 1U << 1;
static const uint32 skShortNameField 				= 1U << 2;
static const uint32 skShortDaylightSavingNameField 	= 1U << 3;
static const uint32 skLongGenericNameField 			= 1U << 4;
static const uint32 skGenericLocationNameField 		= 1U << 5;
static const uint32 skShortCommonlyUsedNameField	= 1U << 6;
static const uint32 skSupportsDaylightSavingField   = 1U << 7;
static const uint32 skOffsetFromGMTField			= 1U << 8;


BTimeZone::BTimeZone(const char* zoneID, const BLanguage* language)
	:
	fIcuTimeZone(NULL),
	fIcuLocale(NULL),
	fInitStatus(B_NO_INIT),
	fInitializedFields(0)
{
	SetTo(zoneID, language);
}


BTimeZone::BTimeZone(const BTimeZone& other)
	:
	fIcuTimeZone(other.fIcuTimeZone == NULL
		? NULL
		: other.fIcuTimeZone->clone()),
	fIcuLocale(other.fIcuLocale == NULL
		? NULL
		: other.fIcuLocale->clone()),
	fInitStatus(other.fInitStatus),
	fInitializedFields(other.fInitializedFields),
	fZoneID(other.fZoneID),
	fName(other.fName),
	fDaylightSavingName(other.fDaylightSavingName),
	fShortName(other.fShortName),
	fShortDaylightSavingName(other.fShortDaylightSavingName),
	fOffsetFromGMT(other.fOffsetFromGMT),
	fSupportsDaylightSaving(other.fSupportsDaylightSaving)
{
}


BTimeZone::~BTimeZone()
{
	delete fIcuLocale;
	delete fIcuTimeZone;
}


BTimeZone& BTimeZone::operator=(const BTimeZone& source)
{
	delete fIcuTimeZone;
	fIcuTimeZone = source.fIcuTimeZone == NULL
		? NULL
		: source.fIcuTimeZone->clone();
	fIcuLocale = source.fIcuLocale == NULL
		? NULL
		: source.fIcuLocale->clone();
	fInitStatus = source.fInitStatus;
	fInitializedFields = source.fInitializedFields;
	fZoneID = source.fZoneID;
	fName = source.fName;
	fDaylightSavingName = source.fDaylightSavingName;
	fShortName = source.fShortName;
	fShortDaylightSavingName = source.fShortDaylightSavingName;
	fOffsetFromGMT = source.fOffsetFromGMT;
	fSupportsDaylightSaving = source.fSupportsDaylightSaving;

	return *this;
}


const BString&
BTimeZone::ID() const
{
	return fZoneID;
}


const BString&
BTimeZone::Name() const
{
	if ((fInitializedFields & skNameField) == 0) {
		UnicodeString unicodeString;
		if (fIcuLocale != NULL) {
			fIcuTimeZone->getDisplayName(false, TimeZone::GENERIC_LOCATION,
				*fIcuLocale, unicodeString);
		} else {
			fIcuTimeZone->getDisplayName(false, TimeZone::GENERIC_LOCATION,
				unicodeString);
		}
		BStringByteSink sink(&fName);
		unicodeString.toUTF8(sink);
		fInitializedFields |= skNameField;
	}

	return fName;
}


const BString&
BTimeZone::DaylightSavingName() const
{
	if ((fInitializedFields & skDaylightSavingNameField) == 0) {
		UnicodeString unicodeString;
		if (fIcuLocale != NULL) {
			fIcuTimeZone->getDisplayName(true, TimeZone::GENERIC_LOCATION,
				*fIcuLocale, unicodeString);
		} else {
			fIcuTimeZone->getDisplayName(true, TimeZone::GENERIC_LOCATION,
				unicodeString);
		}
		BStringByteSink sink(&fDaylightSavingName);
		unicodeString.toUTF8(sink);
		fInitializedFields |= skDaylightSavingNameField;
	}

	return fDaylightSavingName;
}


const BString&
BTimeZone::ShortName() const
{
	if ((fInitializedFields & skShortNameField) == 0) {
		UnicodeString unicodeString;
		if (fIcuLocale != NULL) {
			fIcuTimeZone->getDisplayName(false, TimeZone::SHORT_COMMONLY_USED,
				*fIcuLocale, unicodeString);
		} else {
			fIcuTimeZone->getDisplayName(false, TimeZone::SHORT_COMMONLY_USED,
				unicodeString);
		}
		BStringByteSink sink(&fShortName);
		unicodeString.toUTF8(sink);
		fInitializedFields |= skShortNameField;
	}

	return fShortName;
}


const BString&
BTimeZone::ShortDaylightSavingName() const
{
	if ((fInitializedFields & skShortDaylightSavingNameField) == 0) {
		UnicodeString unicodeString;
		if (fIcuLocale != NULL) {
			fIcuTimeZone->getDisplayName(true, TimeZone::SHORT_COMMONLY_USED,
				*fIcuLocale, unicodeString);
		} else {
			fIcuTimeZone->getDisplayName(true, TimeZone::SHORT_COMMONLY_USED,
				unicodeString);
		}
		BStringByteSink sink(&fShortDaylightSavingName);
		unicodeString.toUTF8(sink);
		fInitializedFields |= skShortDaylightSavingNameField;
	}

	return fShortDaylightSavingName;
}


int
BTimeZone::OffsetFromGMT() const
{
	if ((fInitializedFields & skOffsetFromGMTField) == 0) {
		int32_t rawOffset;
		int32_t dstOffset;
		UDate nowMillis = 1000 * (double)time(NULL);

		UErrorCode error = U_ZERO_ERROR;
		fIcuTimeZone->getOffset(nowMillis, FALSE, rawOffset, dstOffset, error);
		if (!U_SUCCESS(error))
			fOffsetFromGMT = 0;
		else {
			fOffsetFromGMT = (rawOffset + dstOffset) / 1000;
				// we want seconds, not ms (which ICU gives us)
		}
		fInitializedFields |= skOffsetFromGMTField;
	}

	return fOffsetFromGMT;
}


bool
BTimeZone::SupportsDaylightSaving() const
{
	if ((fInitializedFields & skSupportsDaylightSavingField) == 0) {
		fSupportsDaylightSaving = fIcuTimeZone->useDaylightTime();
		fInitializedFields |= skSupportsDaylightSavingField;
	}

	return fSupportsDaylightSaving;
}


status_t
BTimeZone::InitCheck() const
{
	return fInitStatus;
}


status_t
BTimeZone::SetLanguage(const BLanguage* language)
{
	return SetTo(fZoneID, language);
}


status_t
BTimeZone::SetTo(const char* zoneID, const BLanguage* language)
{
	delete fIcuLocale;
	fIcuLocale = NULL;
	delete fIcuTimeZone;
	fInitializedFields = 0;

	if (zoneID == NULL || zoneID[0] == '\0')
		fIcuTimeZone = TimeZone::createDefault();
	else
		fIcuTimeZone = TimeZone::createTimeZone(zoneID);

	if (fIcuTimeZone == NULL) {
		fInitStatus = B_NAME_NOT_FOUND;
		return fInitStatus;
	}

	if (language != NULL) {
		fIcuLocale = new Locale(language->Code());
		if (fIcuLocale == NULL) {
			fInitStatus = B_NO_MEMORY;
			return fInitStatus;
		}
	}

	UnicodeString unicodeString;
	fIcuTimeZone->getID(unicodeString);
	BStringByteSink sink(&fZoneID);
	unicodeString.toUTF8(sink);

	fInitStatus = B_OK;

	return fInitStatus;
}
