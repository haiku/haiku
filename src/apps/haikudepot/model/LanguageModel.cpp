/*
 * Copyright 2019-2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "LanguageModel.h"

#include <algorithm>

#include <Locale.h>
#include <LocaleRoster.h>

#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "LocaleUtils.h"


LanguageModel::LanguageModel()
	:
	fPreferredLanguage(LanguageRef(new Language(LANGUAGE_DEFAULT)))
{
	const Language defaultLanguage = _DeriveDefaultLanguage();
	fSupportedLanguages.push_back(LanguageRef(
		new Language(defaultLanguage), true));
	_SetPreferredLanguage(defaultLanguage);
}


LanguageModel::LanguageModel(BString forcedSystemDefaultLanguage)
	:
	fForcedSystemDefaultLanguage(forcedSystemDefaultLanguage)
{
}


LanguageModel::~LanguageModel()
{
}


void
LanguageModel::ClearSupportedLanguages()
{
	fSupportedLanguages.clear();
	HDINFO("did clear the supported languages");
}


const int32
LanguageModel::CountSupportedLanguages() const
{
	return fSupportedLanguages.size();
}


const LanguageRef
LanguageModel::SupportedLanguageAt(int32 index) const
{
	return fSupportedLanguages[index];
}


void
LanguageModel::AddSupportedLanguage(const LanguageRef& value)
{
	int32 index = _IndexOfSupportedLanguage(value->Code(), value->CountryCode(),
		value->ScriptCode());

	if (-1 == index) {
		std::vector<LanguageRef>::iterator itInsertionPt
			= std::lower_bound(
				fSupportedLanguages.begin(), fSupportedLanguages.end(),
				value, &_IsLanguageBefore);
		fSupportedLanguages.insert(itInsertionPt, value);
		HDTRACE("did add the supported language [%s]" , value->ID());
	}
	else {
		fSupportedLanguages[index] = value;
		HDTRACE("did replace the supported language [%s]", value->ID());
	}
}


void
LanguageModel::SetPreferredLanguageToSystemDefault()
{
	// it could be that the preferred language does not exist in the
	// list.  In this case it is necessary to choose one from the list.
	_SetPreferredLanguage(_DeriveDefaultLanguage());
}


void
LanguageModel::_SetPreferredLanguage(const Language& language)
{
	fPreferredLanguage = LanguageRef(new Language(language));
	HDDEBUG("set preferred language [%s]", fPreferredLanguage->ID());
}


/*! This will derive the default language.  If there are no other
    possible languages configured then the default language will be
    assumed to exist.  Otherwise if there is a set of possible languages
    then this method will ensure that the default language is in that
    set.
*/

Language
LanguageModel::_DeriveDefaultLanguage() const
{
	Language defaultLanguage = _DeriveSystemDefaultLanguage();
	HDDEBUG("derived system default language [%s]", defaultLanguage.ID());

	// if there are no supported languages; as is the case to start with as the
	// application starts, the default language from the system is used anyway.
	// The data queried in HDS will handle the case where the language is not
	// 'known' at the HDS end so it doesn't matter if it is invalid when the
	// HaikuDepot application requests data from the HaikuDepotServer system.

	if (fSupportedLanguages.empty()) {
		HDTRACE("no supported languages --> will use default language");
		return defaultLanguage;
	}

	// if there are supported languages defined then the preferred language
	// needs to be one of the supported ones.

	Language* foundSupportedLanguage = _FindBestSupportedLanguage(
		defaultLanguage.Code(),
		defaultLanguage.CountryCode(),
		defaultLanguage.ScriptCode());

	if (foundSupportedLanguage == NULL) {
		HDERROR("unable to find the language [%s] so will look for app default [%s]",
			defaultLanguage.ID(), LANGUAGE_DEFAULT.ID());
		foundSupportedLanguage = _FindBestSupportedLanguage(
			LANGUAGE_DEFAULT.Code(),
			LANGUAGE_DEFAULT.CountryCode(),
			LANGUAGE_DEFAULT.ScriptCode());

		if (foundSupportedLanguage == NULL) {
			HDERROR("unable to find the app default language [%s] in the supported language so"
				" will use the first supported language [%s]", LANGUAGE_DEFAULT.ID(),
				fSupportedLanguages[0]->ID());
			foundSupportedLanguage = fSupportedLanguages[0];
		}
	} else {
		HDTRACE("did find supported language [%s] as best match to [%s] from %" B_PRIu32
			" supported languages", foundSupportedLanguage->ID(), defaultLanguage.ID(),
			CountSupportedLanguages());
	}

	return Language(*foundSupportedLanguage);
}


/*! This method will create a `Language` object that represents the user's
	preferred language based on their system preferences. If it cannot find
	any such language then it will return the absolute default (English).
*/

Language
LanguageModel::_DeriveSystemDefaultLanguage() const
{
	if (!fForcedSystemDefaultLanguage.IsEmpty())
		return Language(fForcedSystemDefaultLanguage, fForcedSystemDefaultLanguage, true);

	BLocaleRoster* localeRoster = BLocaleRoster::Default();
	if (localeRoster != NULL) {
		BMessage preferredLanguages;
		if (localeRoster->GetPreferredLanguages(&preferredLanguages) == B_OK) {
			BString language;
			if (preferredLanguages.FindString(
				"language", 0, &language) == B_OK) {
				return Language(language, language, true);
			}
		}
	}

	return LANGUAGE_DEFAULT;
}


/*! This method will take the supplied codes and will attempt to find the
	supported language that best matches the codes. If there is really no
	match then it will return `NULL`.
*/

Language*
LanguageModel::_FindBestSupportedLanguage(
	const char* code,
	const char* countryCode,
	const char* scriptCode) const
{
	int32 index = _IndexOfBestMatchingSupportedLanguage(code, countryCode, scriptCode);
	if (-1 != index)
		return SupportedLanguageAt(index);
	return NULL;
}


/*! The supplied `languageId` here is the ICU code with the components separated
	by underscore. This string can be obtained from the `BLanguage` using the
	`ID()` method.
*/

int32
LanguageModel::_IndexOfSupportedLanguage(
	const char* code,
    const char* countryCode,
    const char* scriptCode) const
{
	size_t supportedLanguageSize = fSupportedLanguages.size();
	for (uint32 i = 0; i < supportedLanguageSize; i++) {
		const char *suppLangCode = fSupportedLanguages[i]->Code();
		const char *suppLangCountryCode = fSupportedLanguages[i]->CountryCode();
		const char *suppLangScriptCode = fSupportedLanguages[i]->ScriptCode();

		if( 0 == _NullSafeStrCmp(code, suppLangCode)
			&& 0 == _NullSafeStrCmp(countryCode, suppLangCountryCode)
			&& 0 == _NullSafeStrCmp(scriptCode, suppLangScriptCode)) {
			return i;
		}
	}

	return -1;
}


/*! This will find the first supported language that matches the arguments
	provided. In the case where one of the arguments is `NULL`, is will not
	be considered.
*/

int32
LanguageModel::_IndexOfBestMatchingSupportedLanguage(
	const char* code,
	const char* countryCode,
	const char* scriptCode) const
{
	size_t supportedLanguageSize = fSupportedLanguages.size();

	if (NULL != scriptCode) {
		int32 index = _IndexOfSupportedLanguage(code, countryCode, scriptCode);
			// looking for an exact match
		if (-1 == index)
			return index;
	}

	if (NULL != countryCode) {
		for (uint32 i = 0; i < supportedLanguageSize; i++) {
			if( 0 == _NullSafeStrCmp(code, fSupportedLanguages[i]->Code())
				&& 0 == _NullSafeStrCmp(countryCode, fSupportedLanguages[i]->CountryCode()) ) {
				return i;
			}
		}
	}

	if (NULL != code) {
		for (uint32 i = 0; i < supportedLanguageSize; i++) {
			if(0 == _NullSafeStrCmp(code, fSupportedLanguages[i]->Code()))
				return i;
		}
	}

	return -1;
}


/*static*/ int
LanguageModel::_NullSafeStrCmp(const char* s1, const char* s2) {
	if ((NULL == s1) && (NULL == s2))
		return 0;
	if (NULL == s1)
		return -1;
	if (NULL == s2)
    	return 1;
	return strcmp(s1, s2);
}


/*static*/ int
LanguageModel::_LanguagesCompareFn(const LanguageRef& l1, const LanguageRef& l2)
{
	int result = _NullSafeStrCmp(l1->Code(), l2->Code());
	if (0 == result)
		result = _NullSafeStrCmp(l1->CountryCode(), l2->CountryCode());
	if (0 == result)
		result = _NullSafeStrCmp(l1->ScriptCode(), l2->ScriptCode());
	return result;
}


/*static*/ bool
LanguageModel::_IsLanguageBefore(const LanguageRef& l1, const LanguageRef& l2)
{
	return _LanguagesCompareFn(l1, l2) < 0;
}