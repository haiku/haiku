/*
 * Copyright 2018-2026, Andrew Lindesay <apl@lindesay.co.nz>.
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
AppUtils::NotifySimpleError(const SimpleAlert& alertSimple)
{
	BMessage message(MSG_ALERT_SIMPLE_ERROR);
	if (alertSimple.Archive(&message) != B_OK)
		HDERROR("unable to archive alert");
	else
		be_app->PostMessage(&message);
}


/*static*/ status_t
AppUtils::MarkItemWithKeyValueInMenuOrFirst(BMenu* menu, const BString& key, const BString& value)
{
	status_t result = AppUtils::MarkItemWithKeyValueInMenu(menu, key, value);
	if (result != B_OK && menu->CountItems() > 0)
		menu->ItemAt(0)->SetMarked(true);
	return result;
}


/*static*/ status_t
AppUtils::MarkItemWithKeyValueInMenu(BMenu* menu, const BString& key, const BString& value)
{
	if (menu->CountItems() == 0)
		HDFATAL("menu contains no items; not able to mark the item");

	int32 index = AppUtils::IndexOfKeyValueInMenu(menu, key, value);

	if (index == -1) {
		HDINFO("unable to find the menu item with [%s] = [%s]", key.String(), value.String());
		return B_ERROR;
	}

	menu->ItemAt(index)->SetMarked(true);
	return B_OK;
}


/*static*/ int32
AppUtils::IndexOfKeyValueInMenu(BMenu* menu, const BString& key, const BString& value)
{
	BString itemCode;
	for (int32 i = 0; i < menu->CountItems(); i++) {
		if (AppUtils::GetValueForKeyAtIndexInMenu(menu, i, key, &itemCode) == B_OK) {
			if (itemCode == value)
				return i;
		}
	}

	return -1;
}


/*static*/ status_t
AppUtils::GetValueForKeyAtIndexInMenu(BMenu* menu, int32 index, const BString& key, BString* result)
{
	BMessage *itemMessage = menu->ItemAt(index)->Message();
	if (itemMessage == NULL)
		return B_ERROR;
	return itemMessage->FindString(key, result);
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
