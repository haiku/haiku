/*
 * Copyright 2019-2025, Andrew Lindesay <apl@lindesay.co.nz>.
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
	if (format.Format(buffer, millis / 1000, B_SHORT_DATE_FORMAT, B_SHORT_TIME_FORMAT) != B_OK)
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
	if (format.Format(buffer, millis / 1000, B_SHORT_DATE_FORMAT) != B_OK)
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
LocaleUtils::DeriveDefaultLanguage(const std::vector<LanguageRef>& languages)
{
	LanguageRef defaultLanguage = _DeriveSystemDefaultLanguage();
	HDDEBUG("derived system default language [%s]", defaultLanguage->ID());

	// if there are no supported languages; as is the case to start with as the
	// application starts, the default language from the system is used anyway.
	// The data queried in HDS will handle the case where the language is not
	// 'known' at the HDS end so it doesn't matter if it is invalid when the
	// HaikuDepot application requests data from the HaikuDepotServer system.

	if (languages.empty()) {
		HDTRACE("no supported languages --> will use default language");
		return defaultLanguage;
	}

	// if there are supported languages defined then the preferred language
	// needs to be one of the supported ones.

	LanguageRef foundSupportedLanguage = _FindBestMatchingLanguage(languages,
		defaultLanguage->Code(), defaultLanguage->CountryCode(), defaultLanguage->ScriptCode());

	if (!foundSupportedLanguage.IsSet()) {
		Language appDefaultLanguage(LANGUAGE_DEFAULT_ID, "English", true);
		HDERROR("unable to find the language [%s] so will look for app default [%s]",
			defaultLanguage->ID(), appDefaultLanguage.ID());
		foundSupportedLanguage = _FindBestMatchingLanguage(languages, appDefaultLanguage.Code(),
			appDefaultLanguage.CountryCode(), appDefaultLanguage.ScriptCode());

		if (!foundSupportedLanguage.IsSet()) {
			foundSupportedLanguage = languages[0];
			HDERROR("unable to find the app default language [%s] in the supported language so"
					" will use the first supported language [%s]",
				appDefaultLanguage.ID(), foundSupportedLanguage->ID());
		}
	} else {
		HDTRACE("did find supported language [%s] as best match to [%s] from %" B_PRIu64
				" supported languages",
			foundSupportedLanguage->ID(), defaultLanguage->ID(),
			static_cast<uint64>(languages.size()));
	}

	return foundSupportedLanguage;
}


/*static*/ void
LocaleUtils::SetForcedSystemDefaultLanguageID(const BString& id)
{
	sForcedSystemDefaultLanguageID = id;
}

/*!	Returns a set of popular languages that can be used in the case that the loading
	of supported languages from the server is not working. These languages are
	assured to be present on the server.
*/
/*static*/ std::vector<LanguageRef>
LocaleUtils::WellKnownLanguages()
{
	std::vector<LanguageRef> languages;
	languages.push_back(LanguageRef(new Language("en", "English", true)));
	languages.push_back(LanguageRef(new Language("fr", "French", true)));
	languages.push_back(LanguageRef(new Language("de", "German", true)));
	languages.push_back(LanguageRef(new Language("ja", "Japanese", true)));
	languages.push_back(LanguageRef(new Language("ru", "Russian", true)));
	languages.push_back(LanguageRef(new Language("pt", "Portugese", true)));
	languages.push_back(LanguageRef(new Language("es", "Spanish", true)));
	languages.push_back(LanguageRef(new Language("zh", "Mandarin", true)));
	return languages;
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
LocaleUtils::_FindBestMatchingLanguage(const std::vector<LanguageRef>& languages, const char* code,
	const char* countryCode, const char* scriptCode)
{
	int32 index = _IndexOfBestMatchingLanguage(languages, code, countryCode, scriptCode);
	if (-1 != index)
		return languages[index];
	return LanguageRef();
}


/*! This will find the first supported language that matches the arguments
	provided. In the case where one of the arguments is `NULL`, is will not
	be considered.
*/

/*static*/ int32
LocaleUtils::_IndexOfBestMatchingLanguage(const std::vector<LanguageRef>& languages,
	const char* code, const char* countryCode, const char* scriptCode)
{
	int32 languageSize = languages.size();

	if (NULL != scriptCode) {
		for (int32 i = 0; i < languageSize; i++) {
			const char* lCode = languages[i]->Code();
			const char* lCountryCode = languages[i]->CountryCode();
			const char* lScriptCode = languages[i]->ScriptCode();

			if (0 == StringUtils::NullSafeCompare(code, lCode)
				&& 0 == StringUtils::NullSafeCompare(countryCode, lCountryCode)
				&& 0 == StringUtils::NullSafeCompare(scriptCode, lScriptCode)) {
				return i;
			}
		}
	}

	if (NULL != countryCode) {
		for (int32 i = 0; i < languageSize; i++) {
			const char* lCode = languages[i]->Code();
			const char* lCountryCode = languages[i]->CountryCode();

			if (0 == StringUtils::NullSafeCompare(code, lCode)
				&& 0 == StringUtils::NullSafeCompare(countryCode, lCountryCode)) {
				return i;
			}
		}
	}

	if (NULL != code) {
		for (int32 i = 0; i < languageSize; i++) {
			const char* lCode = languages[i]->Code();
			if (0 == StringUtils::NullSafeCompare(code, lCode))
				return i;
		}
	}

	return -1;
}
