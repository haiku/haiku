/*
 * Copyright 2019-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "LanguageModel.h"

#include <algorithm>

#include <Collator.h>
#include <Locale.h>
#include <LocaleRoster.h>

#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "LocaleUtils.h"


static int32
_LanguagesCompareFn(const LanguageRef& l1, const LanguageRef& l2)
{
	BCollator* collator = LocaleUtils::GetSharedCollator();
	BString name1;
	BString name2;
	int32 result = 0;

	if (l1->GetName(name1) == B_OK && l2->GetName(name2) == B_OK)
		result = collator->Compare(name1.String(), name2.String());
	if (0 == result)
		result = strcmp(l1->Code(), l2->Code());

	return result;
}


static bool
_IsLanguageBefore(const LanguageRef& l1, const LanguageRef& l2)
{
	return _LanguagesCompareFn(l1, l2) < 0;
}


LanguageModel::LanguageModel()
	:
	fPreferredLanguage(LanguageRef(new Language(LANGUAGE_DEFAULT)))
{
	const Language defaultLanguage = _DeriveDefaultLanguage();
	fSupportedLanguages.push_back(LanguageRef(
		new Language(defaultLanguage), true));
	_SetPreferredLanguage(defaultLanguage);
}


LanguageModel::~LanguageModel()
{
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


int32
LanguageModel::IndexOfSupportedLanguage(const BString& languageCode) const
{
	for (uint32 i = 0; i < fSupportedLanguages.size(); i++) {
		if (fSupportedLanguages[i]->Code() == languageCode)
			return i;
	}

	return -1;
}


void
LanguageModel::AddSupportedLanguage(const LanguageRef& value)
{
	int32 index = IndexOfSupportedLanguage(value->Code());

	if (-1 == index) {
		std::vector<LanguageRef>::iterator itInsertionPt
			= std::lower_bound(
				fSupportedLanguages.begin(), fSupportedLanguages.end(),
				value, &_IsLanguageBefore);
		fSupportedLanguages.insert(itInsertionPt, value);
		HDTRACE("did add the supported language [%s]" , value->Code());
	}
	else {
		fSupportedLanguages[index] = value;
		HDTRACE("did replace the supported language [%s]", value->Code());
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
	HDDEBUG("set preferred language [%s]", language.Code());
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
	HDDEBUG("derived system default language [%s]", defaultLanguage.Code());

	// if there are no supported languages; as is the case to start with as the
	// application starts, the default language from the system is used anyway.
	// The data queried in HDS will handle the case where the language is not
	// 'known' at the HDS end so it doesn't matter if it is invalid.

	if (fSupportedLanguages.empty())
		return defaultLanguage;

	// if there are supported languages defined then the preferred language
	// needs to be one of the supported ones.

	Language* foundSupportedLanguage = _FindSupportedLanguage(
		defaultLanguage.Code());

	if (foundSupportedLanguage == NULL) {
		HDERROR("unable to find the language [%s] - looking for app default",
			defaultLanguage.Code());
		foundSupportedLanguage = _FindSupportedLanguage(
			LANGUAGE_DEFAULT.Code());
	}

	if (foundSupportedLanguage == NULL) {
		HDERROR("unable to find the app default language - using the first "
			"supported language");
		foundSupportedLanguage = fSupportedLanguages[0];
	}

	return Language(*foundSupportedLanguage);
}


/*static*/ Language
LanguageModel::_DeriveSystemDefaultLanguage()
{
	BLocaleRoster* localeRoster = BLocaleRoster::Default();
	if (localeRoster != NULL) {
		BMessage preferredLanguages;
		if (localeRoster->GetPreferredLanguages(&preferredLanguages) == B_OK) {
			BString language;
			BString languageIso;
			if (preferredLanguages.FindString(
				"language", 0, &language) == B_OK) {
				language.CopyInto(languageIso, 0, 2);
				return Language(languageIso, languageIso, true);
			}
		}
	}

	return LANGUAGE_DEFAULT;
}


Language*
LanguageModel::_FindSupportedLanguage(const BString& code) const
{
	int32 index = IndexOfSupportedLanguage(code);
	if (-1 == index)
		return NULL;
	return fSupportedLanguages[index];
}