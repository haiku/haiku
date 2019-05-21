/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "LanguageMenuUtils.h"

#include <string.h>

#include <Application.h>
#include <MenuItem.h>
#include <Messenger.h>

#include "HaikuDepotConstants.h"
#include "Logger.h"


/*! This method will add the supplied languages to the menu.  It will
    add first the popular languages, followed by a separator and then
    other less popular languages.
*/

/* static */ void
LanguageMenuUtils::AddLanguagesToMenu(
	const LanguageList& languages, BMenu* menu)
{
	if (languages.IsEmpty() && Logger::IsInfoEnabled())
		printf("there are no languages defined\n");

	int32 addedPopular = LanguageMenuUtils::_AddLanguagesToMenu(
		languages, menu, true);

	if (addedPopular > 0)
		menu->AddSeparatorItem();

	int32 addedNonPopular = LanguageMenuUtils::_AddLanguagesToMenu(
		languages, menu, false);

	if (Logger::IsDebugEnabled()) {
		printf("did add %" B_PRId32 " popular languages and %" B_PRId32
			" non-popular languages to a menu\n", addedPopular,
			addedNonPopular);
	}
}


/* static */ void
LanguageMenuUtils::MarkLanguageInMenu(
	const BString& languageCode, BMenu* menu) {
	if (menu->CountItems() == 0) {
		debugger("menu contains no items; not able to set the "
			"language");
		return;
	}

	int32 index = LanguageMenuUtils::_IndexOfLanguageInMenu(
		languageCode, menu);

	if (index == -1) {
		printf("unable to find the language [%s] in the menu\n",
			languageCode.String());
		menu->ItemAt(0)->SetMarked(true);
	}
	else
		menu->ItemAt(index)->SetMarked(true);
}


/* static */ void
LanguageMenuUtils::_AddLanguageToMenu(
	const BString& code, const BString& name, BMenu* menu)
{
	BMessage* message = new BMessage(MSG_LANGUAGE_SELECTED);
	message->AddString("code", code);
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
	LanguageMenuUtils::_AddLanguageToMenu(language->Code(), name, menu);
}


/* static */ int32
LanguageMenuUtils::_AddLanguagesToMenu(const LanguageList& languages,
	BMenu* menu, bool isPopular)
{
	int32 count = 0;

	for (int32 i = 0; i < languages.CountItems(); i++) {
		const LanguageRef language = languages.ItemAtFast(i);

		if (language->IsPopular() == isPopular) {
			LanguageMenuUtils::_AddLanguageToMenu(language, menu);
			count++;
		}
	}

	return count;
}


/* static */ status_t
LanguageMenuUtils::_GetLanguageAtIndexInMenu(BMenu* menu, int32 index,
	BString* result)
{
	BMessage *itemMessage = menu->ItemAt(index)->Message();

	if (itemMessage == NULL)
		return B_ERROR;

	return itemMessage->FindString("code", result);
}


/* static */ int32
LanguageMenuUtils::_IndexOfLanguageInMenu(
	const BString& languageCode, BMenu* menu)
{
	BString itemLanguageCode;
	for (int32 i = 0; i < menu->CountItems(); i++) {
		if (_GetLanguageAtIndexInMenu(
			menu, i, &itemLanguageCode) == B_OK) {
			if (itemLanguageCode == languageCode) {
				return i;
			}
		}
	}

	return -1;
}