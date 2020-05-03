/*
 * Copyright 2019-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "LanguageModel.h"

#include <Collator.h>
#include <Locale.h>
#include <LocaleRoster.h>

#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "LocaleUtils.h"


static int32
LanguagesCompareFn(const LanguageRef& l1, const LanguageRef& l2)
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


LanguageModel::LanguageModel()
	:
	fSupportedLanguages(LanguagesCompareFn, NULL),
	fPreferredLanguage(LanguageRef(new Language(LANGUAGE_DEFAULT)))
{
	const Language defaultLanguage = _DeriveDefaultLanguage();
	fSupportedLanguages.Add(LanguageRef(
		new Language(defaultLanguage), true));
	_SetPreferredLanguage(defaultLanguage);
}


LanguageModel::~LanguageModel()
{
}


void
LanguageModel::AddSupportedLanguages(const LanguageList& languages)
{
	for (int32 i = 0; i < languages.CountItems(); i++) {
		const LanguageRef language = languages.ItemAt(i);
		int32 index = IndexOfSupportedLanguage(language->Code());

		if (index == -1)
			fSupportedLanguages.Add(language);
		else
			fSupportedLanguages.Replace(index, language);
	}

	// it could be that the preferred language does not exist in the
	// list.  In this case it is necessary to choose one from the list.
	_SetPreferredLanguage(_DeriveDefaultLanguage());
}


void
LanguageModel::_SetPreferredLanguage(const Language& language)
{
	fPreferredLanguage = LanguageRef(new Language(language));
	if(Logger::IsDebugEnabled())
		printf("set preferred language [%s]\n", language.Code());
}


int32
LanguageModel::IndexOfSupportedLanguage(const BString& languageCode) const
{
	for (int32 i = 0; i < fSupportedLanguages.CountItems(); i++) {
		if (fSupportedLanguages.ItemAt(i)->Code() == languageCode)
			return i;
	}

	return -1;
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

	if(Logger::IsDebugEnabled()) {
		printf("derived system default language [%s]\n",
			defaultLanguage.Code());
	}

	// if there are no supported languages; as is the case to start with as the
	// application starts, the default language from the system is used anyway.
	// The data queried in HDS will handle the case where the language is not
	// 'known' at the HDS end so it doesn't matter if it is invalid.

	if (fSupportedLanguages.IsEmpty())
		return defaultLanguage;

	// if there are supported languages defined then the preferred language
	// needs to be one of the supported ones.

	Language* foundSupportedLanguage = _FindSupportedLanguage(
		defaultLanguage.Code());

	if (foundSupportedLanguage == NULL) {
		printf("unable to find the language [%s] - looking for app default",
			defaultLanguage.Code());
		foundSupportedLanguage = _FindSupportedLanguage(
			LANGUAGE_DEFAULT.Code());
	}

	if (foundSupportedLanguage == NULL) {
		printf("unable to find the app default language - using the first "
			"supported language");
		foundSupportedLanguage = fSupportedLanguages.ItemAt(0);
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
	return fSupportedLanguages.ItemAt(index);
}