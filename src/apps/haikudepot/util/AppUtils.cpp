/*
 * Copyright 2018-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "AppUtils.h"

#include <string.h>

#include <Application.h>
#include <MenuItem.h>
#include <String.h>

#include "HaikuDepotConstants.h"
#include "Logger.h"


/*! This method can be called to pop up an error in the user interface;
    typically in a background thread.
 */

/*static*/ void
AppUtils::NotifySimpleError(const char* title, const char* text)
{
	BMessage message(MSG_ALERT_SIMPLE_ERROR);

	if (title != NULL && strlen(title) != 0)
		message.AddString(KEY_ALERT_TITLE, title);

	if (text != NULL && strlen(text) != 0)
		message.AddString(KEY_ALERT_TEXT, text);

	be_app->PostMessage(&message);
}


/*static*/ status_t
AppUtils::MarkItemWithCodeInMenuOrFirst(const BString& code, BMenu* menu)
{
	status_t result = AppUtils::MarkItemWithCodeInMenu(code, menu);
	if (result != B_OK)
		menu->ItemAt(0)->SetMarked(true);
	return result;
}


/*static*/ status_t
AppUtils::MarkItemWithCodeInMenu(const BString& code, BMenu* menu)
{
	if (menu->CountItems() == 0)
		HDFATAL("menu contains no items; not able to mark the item");

	int32 index = AppUtils::IndexOfCodeInMenu(code, menu);

	if (index == -1) {
		HDINFO("unable to find the menu item [%s]", code.String());
		return B_ERROR;
	}

	menu->ItemAt(index)->SetMarked(true);
	return B_OK;
}


/*static*/ int32
AppUtils::IndexOfCodeInMenu(const BString& code, BMenu* menu)
{
	BString itemCode;
	for (int32 i = 0; i < menu->CountItems(); i++) {
		if (AppUtils::GetCodeAtIndexInMenu(menu, i, &itemCode) == B_OK
				&& itemCode == code) {
			return i;
		}
	}

	return -1;
}


/*static*/ status_t
AppUtils::GetCodeAtIndexInMenu(BMenu* menu, int32 index, BString* result)
{
	BMessage *itemMessage = menu->ItemAt(index)->Message();
	if (itemMessage == NULL)
		return B_ERROR;
	return itemMessage->FindString("code", result);
}
