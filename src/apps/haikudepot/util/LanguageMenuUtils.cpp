/*
 * Copyright 2019-2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "LanguageMenuUtils.h"

#include <algorithm>

#include <string.h>

#include <Application.h>
#include <Collator.h>
#include <MenuItem.h>
#include <Messenger.h>

#include "AppUtils.h"
#include "HaikuDepotConstants.h"
#include "LocaleUtils.h"
#include "Logger.h"


static const char* kLanguageIdKey = "id";


/*! This method will add the supplied languages to the menu.  It will
    add first the popular languages, followed by a separator and then
    other less popular languages.
*/

/* static */ void
LanguageMenuUtils::AddLanguagesToMenu(const LanguageRepository* repository, BMenu* menu)
{
	if (repository->IsEmpty())
		HDINFO("there are no languages defined");

	// collect all of the languages into a vector so that they can be sorted
	// and used.

	std::vector<LanguageRef> languages(repository->CountLanguages());

	for (int32 i = repository->CountLanguages(); i >= 0; i--)
		languages[i] = repository->LanguageAtIndex(i);

	std::sort(languages.begin(), languages.end(), IsLanguageBefore);

	// now add the sorted languages to the menu.

	int32 addedPopular = LanguageMenuUtils::_AddLanguagesToMenu(languages, menu, true);

	if (addedPopular > 0)
		menu->AddSeparatorItem();

	int32 addedNonPopular = LanguageMenuUtils::_AddLanguagesToMenu(languages, menu, false);

	HDDEBUG("did add %" B_PRId32 " popular languages and %" B_PRId32
		" non-popular languages to a menu", addedPopular,
		addedNonPopular);
}


/* static */ void
LanguageMenuUtils::MarkLanguageInMenu(
	const BString& languageId, BMenu* menu) {
	AppUtils::MarkItemWithKeyValueInMenuOrFirst(menu, kLanguageIdKey, languageId);
}


/* static */ void
LanguageMenuUtils::_AddLanguageToMenu(
	const BString& id, const BString& name, BMenu* menu)
{
	BMessage* message = new BMessage(MSG_LANGUAGE_SELECTED);
	message->AddString(kLanguageIdKey, id);
	BMenuItem* item = new BMenuItem(name, message);
	menu->AddItem(item);
}


/* static */ void
LanguageMenuUtils::_AddLanguageToMenu(
	const LanguageRef& language, BMenu* menu)
{
	BString name;
	if (language->GetName(name) != B_OK || name.IsEmpty())
		name.SetTo("???");
	LanguageMenuUtils::_AddLanguageToMenu(language->ID(), name, menu);
}


/* static */ int32
LanguageMenuUtils::_AddLanguagesToMenu(const std::vector<LanguageRef>& languages,
	BMenu* menu, bool isPopular)
{
	int32 count = 0;

	for (uint32 i = 0; i < languages.size(); i++) {
		const LanguageRef language = languages[i];

		if (languages[i]->IsPopular() == isPopular) {
			LanguageMenuUtils::_AddLanguageToMenu(languages[i], menu);
			count++;
		}
	}

	return count;
}


/*static*/ int
LanguageMenuUtils::_LanguagesPresentationCompareFn(const LanguageRef& l1, const LanguageRef& l2)
{
	BCollator* collator = LocaleUtils::GetSharedCollator();
	BString name1;
	BString name2;
	int32 result = 0;

	if (l1->GetName(name1) == B_OK && l2->GetName(name2) == B_OK)
		result = collator->Compare(name1.String(), name2.String());
	if (0 == result)
		result = strcmp(l1->ID(), l2->ID());

	return result;
}


/*static*/ bool
LanguageMenuUtils::_IsLanguagePresentationBefore(const LanguageRef& l1, const LanguageRef& l2)
{
	return _LanguagesPresentationCompareFn(l1, l2) < 0;
}