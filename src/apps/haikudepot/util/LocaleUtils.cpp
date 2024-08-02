/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "LocaleUtils.h"

#include <stdlib.h>

#include <Catalog.h>
#include <Collator.h>
#include <DateFormat.h>
#include <DateTimeFormat.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <StringFormat.h>

#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "StringUtils.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "LocaleUtils"


BCollator* LocaleUtils::sSharedCollator = NULL;
BString LocaleUtils::sForcedSystemDefaultLanguageID;


/*static*/ BCollator*
LocaleUtils::GetSharedCollator()
{
	if (sSharedCollator == NULL) {
		sSharedCollator = new BCollator();
		_GetCollator(sSharedCollator);
	}

	return sSharedCollator;
}


/*static*/ void
LocaleUtils::_GetCollator(BCollator* collator)
{
	const BLocale* locale = BLocaleRoster::Default()->GetDefaultLocale();

	if (locale->GetCollator(collator) != B_OK)
		HDFATAL("unable to get the locale's collator");
}


/*static*/ BString
LocaleUtils::TimestampToDateTimeString(uint64 millis)
{
	if (millis == 0)
		return "?";

	BDateTimeFormat format;
	BString buffer;
	if (format.Format(buffer, millis / 1000, B_SHORT_DATE_FORMAT,
			B_SHORT_TIME_FORMAT) != B_OK)
		return "!";

	return buffer;
}


/*static*/ BString
LocaleUtils::TimestampToDateString(uint64 millis)
{
	if (millis == 0)
		return "?";

	BDateFormat format;
	BString buffer;
	if (format.Format(buffer, millis / 1000, B_SHORT_DATE_FORMAT)
			!= B_OK)
		return "!";

	return buffer;
}


/*!	This is used in situations where the user is required to confirm that they
	are as old or older than some minimal age.  This is associated with agreeing
	to the user usage conditions.
*/

/*static*/ BString
LocaleUtils::CreateTranslatedIAmMinimumAgeSlug(int minimumAge)
{
	BString slug;
	static BStringFormat format(B_TRANSLATE("{0, plural,"
 		"one{I am at least one year old}"
		"other{I am # years of age or older}}"));
	format.Format(slug, minimumAge);
	return slug;
}


/*!	This will derive the default language.  If there are no other
	possible languages configured then the default language will be
	assumed to exist.  Otherwise if there is a set of possible languages
	then this method will ensure that the default language is in that
	set.
*/

/*static*/ LanguageRef
LocaleUtils::DeriveDefaultLanguage(LanguageRepository* repository)
{
	LanguageRef defaultLanguage = _DeriveSystemDefaultLanguage();
	HDDEBUG("derived system default language [%s]", defaultLanguage->ID());

	// if there are no supported languages; as is the case to start with as the
	// application starts, the default language from the system is used anyway.
	// The data queried in HDS will handle the case where the language is not
	// 'known' at the HDS end so it doesn't matter if it is invalid when the
	// HaikuDepot application requests data from the HaikuDepotServer system.

	if (repository->IsEmpty()) {
		HDTRACE("no supported languages --> will use default language");
		return defaultLanguage;
	}

	// if there are supported languages defined then the preferred language
	// needs to be one of the supported ones.

	LanguageRef foundSupportedLanguage = _FindBestMatchingLanguage(repository,
		defaultLanguage->Code(), defaultLanguage->CountryCode(), defaultLanguage->ScriptCode());

	if (!foundSupportedLanguage.IsSet()) {
		Language appDefaultLanguage(LANGUAGE_DEFAULT_ID, "English", true);
		HDERROR("unable to find the language [%s] so will look for app default [%s]",
			defaultLanguage->ID(), appDefaultLanguage.ID());
		foundSupportedLanguage = _FindBestMatchingLanguage(repository, appDefaultLanguage.Code(),
			appDefaultLanguage.CountryCode(), appDefaultLanguage.ScriptCode());

		if (!foundSupportedLanguage.IsSet()) {
			foundSupportedLanguage = repository->LanguageAtIndex(0);
			HDERROR("unable to find the app default language [%s] in the supported language so"
					" will use the first supported language [%s]",
				appDefaultLanguage.ID(), foundSupportedLanguage->ID());
		}
	} else {
		HDTRACE("did find supported language [%s] as best match to [%s] from %" B_PRIu32
				" supported languages",
			foundSupportedLanguage->ID(), defaultLanguage->ID(), repository->CountLanguages());
	}

	return foundSupportedLanguage;
}


/*static*/ void
LocaleUtils::SetForcedSystemDefaultLanguageID(const BString& id)
{
	sForcedSystemDefaultLanguageID = id;
}


/*static*/ LanguageRef
LocaleUtils::_DeriveSystemDefaultLanguage()
{
	if (!sForcedSystemDefaultLanguageID.IsEmpty()) {
		return LanguageRef(
			new Language(sForcedSystemDefaultLanguageID, sForcedSystemDefaultLanguageID, true));
	}

	BLocaleRoster* localeRoster = BLocaleRoster::Default();
	if (localeRoster != NULL) {
		BMessage preferredLanguages;
		if (localeRoster->GetPreferredLanguages(&preferredLanguages) == B_OK) {
			BString language;
			if (preferredLanguages.FindString("language", 0, &language) == B_OK)
				return LanguageRef(new Language(language, language, true));
		}
	}

	return LanguageRef(new Language(LANGUAGE_DEFAULT_ID, "English", true), true);
}


/*! This method will take the supplied codes and will attempt to find the
	supported language that best matches the codes. If there is really no
	match then it will return `NULL`.
*/

/*static*/ LanguageRef
LocaleUtils::_FindBestMatchingLanguage(LanguageRepository* repository, const char* code,
	const char* countryCode, const char* scriptCode)
{
	int32 index = _IndexOfBestMatchingLanguage(repository, code, countryCode, scriptCode);
	if (-1 != index)
		return repository->LanguageAtIndex(index);
	return LanguageRef();
}


/*! This will find the first supported language that matches the arguments
	provided. In the case where one of the arguments is `NULL`, is will not
	be considered.
*/

/*static*/ int32
LocaleUtils::_IndexOfBestMatchingLanguage(LanguageRepository* repository, const char* code,
	const char* countryCode, const char* scriptCode)
{
	int32 languagesCount = repository->CountLanguages();

	if (NULL != scriptCode) {
		int32 index = repository->IndexOfLanguage(code, countryCode, scriptCode);
			// looking for an exact match
		if (-1 == index)
			return index;
	}

	if (NULL != countryCode) {
		for (int32 i = 0; i < languagesCount; i++) {
			LanguageRef language = repository->LanguageAtIndex(i);
			if (0 == StringUtils::NullSafeCompare(code, language->Code())
				&& 0 == StringUtils::NullSafeCompare(countryCode, language->CountryCode())) {
				return i;
			}
		}
	}

	if (NULL != code) {
		for (int32 i = 0; i < languagesCount; i++) {
			LanguageRef language = repository->LanguageAtIndex(i);
			if (0 == StringUtils::NullSafeCompare(code, language->Code()))
				return i;
		}
	}

	return -1;
}
