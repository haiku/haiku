/*
 * Copyright 2018-2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "AppUtils.h"

#include <string.h>

#include <AppFileInfo.h>
#include <Application.h>
#include <MenuItem.h>
#include <Roster.h>
#include <String.h>

#include "HaikuDepotConstants.h"
#include "Logger.h"


/*! This method can be called to pop up an error in the user interface;
    typically in a background thread.
 */

/*static*/ void
AppUtils::NotifySimpleError(const char* title, const char* text,
	alert_type type)
{
	BMessage message(MSG_ALERT_SIMPLE_ERROR);

	if (title != NULL && strlen(title) != 0)
		message.AddString(KEY_ALERT_TITLE, title);

	if (text != NULL && strlen(text) != 0)
		message.AddString(KEY_ALERT_TEXT, text);

	message.AddInt32(KEY_ALERT_TYPE, static_cast<int>(type));

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


/*static*/ status_t
AppUtils::GetAppVersionString(BString& result)
{
	app_info info;

	if (be_app->GetAppInfo(&info) != B_OK) {
		HDERROR("Unable to get the application info");
		return B_ERROR;
	}

	BFile file(&info.ref, B_READ_ONLY);

	if (file.InitCheck() != B_OK) {
		HDERROR("Unable to access the application info file");
		return B_ERROR;
	}

	BAppFileInfo appFileInfo(&file);
	version_info versionInfo;

	if (appFileInfo.GetVersionInfo(
			&versionInfo, B_APP_VERSION_KIND) != B_OK) {
		HDERROR("Unable to establish the application version");
		return B_ERROR;
	}

	result.SetToFormat("%" B_PRId32 ".%" B_PRId32 ".%" B_PRId32,
		versionInfo.major, versionInfo.middle, versionInfo.minor);

	return B_OK;
}
