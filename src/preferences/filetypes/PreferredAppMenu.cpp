/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "FileTypes.h"
#include "PreferredAppMenu.h"

#include <Alert.h>
#include <AppFileInfo.h>
#include <Catalog.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <String.h>

#include <stdio.h>
#include <strings.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Preferred App Menu"


static int
compare_menu_items(const void* _a, const void* _b)
{
	BMenuItem* a = *(BMenuItem**)_a;
	BMenuItem* b = *(BMenuItem**)_b;

	return strcasecmp(a->Label(), b->Label());
}


static bool
is_application_in_message(BMessage& applications, const char* app)
{
	const char* signature;
	int32 i = 0;
	while (applications.FindString("applications", i++, &signature) == B_OK) {
		if (!strcasecmp(signature, app))
			return true;
	}

	return false;
}


static void
add_signature(BMenuItem* item, const char* signature)
{
	const char* subType = strchr(signature, '/');
	if (subType == NULL)
		return;

	char label[B_MIME_TYPE_LENGTH];
	snprintf(label, sizeof(label), "%s (%s)", item->Label(), subType + 1);

	item->SetLabel(label);
}


static BMenuItem*
create_application_item(const char* signature, uint32 what)
{
	char name[B_FILE_NAME_LENGTH];

	BMessage* message = new BMessage(what);
	message->AddString("signature", signature);

	BMimeType applicationType(signature);
	if (applicationType.GetShortDescription(name) == B_OK)
		return new BMenuItem(name, message);

	return new BMenuItem(signature, message);
}


//	#pragma mark - Public functions


void
update_preferred_app_menu(BMenu* menu, BMimeType* type, uint32 what,
	const char* preferredFrom)
{
	// clear menu (but leave the first entry, ie. "None")

	for (int32 i = menu->CountItems(); i-- > 1;) {
		delete menu->RemoveItem(i);
	}

	// fill it again

	menu->ItemAt(0)->SetMarked(true);

	BMessage applications;
	if (type == NULL || type->GetSupportingApps(&applications) != B_OK)
		return;

	char preferred[B_MIME_TYPE_LENGTH];
	if (type->GetPreferredApp(preferred) != B_OK)
		preferred[0] = '\0';

	int32 lastFullSupport;
	if (applications.FindInt32("be:sub", &lastFullSupport) != B_OK)
		lastFullSupport = -1;

	BList subList;
	BList superList;

	const char* signature;
	int32 i = 0;
	while (applications.FindString("applications", i, &signature) == B_OK) {
		BMenuItem* item = create_application_item(signature, what);

		if (i < lastFullSupport)
			subList.AddItem(item);
		else
			superList.AddItem(item);

		i++;
	}

	// sort lists

	subList.SortItems(compare_menu_items);
	superList.SortItems(compare_menu_items);

	// add lists to the menu

	if (subList.CountItems() != 0 || superList.CountItems() != 0)
		menu->AddSeparatorItem();

	for (int32 i = 0; i < subList.CountItems(); i++) {
		menu->AddItem((BMenuItem*)subList.ItemAt(i));
	}

	// Add type separator
	if (superList.CountItems() != 0 && subList.CountItems() != 0)
		menu->AddSeparatorItem();

	for (int32 i = 0; i < superList.CountItems(); i++) {
		menu->AddItem((BMenuItem*)superList.ItemAt(i));
	}

	// make items unique and select current choice

	bool lastItemSame = false;
	const char* lastSignature = NULL;
	BMenuItem* last = NULL;
	BMenuItem* select = NULL;

	for (int32 index = 0; index < menu->CountItems(); index++) {
		BMenuItem* item = menu->ItemAt(index);
		if (item == NULL)
			continue;

		if (item->Message() == NULL
			|| item->Message()->FindString("signature", &signature) != B_OK)
			continue;

		if ((preferredFrom == NULL && !strcasecmp(signature, preferred))
			|| (preferredFrom != NULL
				&& !strcasecmp(signature, preferredFrom))) {
			select = item;
		}

		if (last == NULL || strcmp(last->Label(), item->Label())) {
			if (lastItemSame)
				add_signature(last, lastSignature);

			lastItemSame = false;
			last = item;
			lastSignature = signature;
			continue;
		}

		lastItemSame = true;
		add_signature(last, lastSignature);

		last = item;
		lastSignature = signature;
	}

	if (lastItemSame)
		add_signature(last, lastSignature);

	if (select != NULL) {
		// We don't select the item earlier, so that the menu field can
		// pick up the signature as well as label.
		select->SetMarked(true);
	} else if ((preferredFrom == NULL && preferred[0])
		|| (preferredFrom != NULL && preferredFrom[0])) {
		// The preferred application is not an application that support
		// this file type!
		BMenuItem* item = create_application_item(preferredFrom
			? preferredFrom : preferred, what);

		menu->AddSeparatorItem();
		menu->AddItem(item);
		item->SetMarked(true);
	}
}


status_t
retrieve_preferred_app(BMessage* message, bool sameAs, const char* forType,
	BString& preferredApp)
{
	entry_ref ref;
	if (message == NULL || message->FindRef("refs", &ref) != B_OK)
		return B_BAD_VALUE;

	BFile file(&ref, B_READ_ONLY);
	status_t status = file.InitCheck();

	char preferred[B_MIME_TYPE_LENGTH];

	if (status == B_OK) {
		if (sameAs) {
			// get preferred app from file
			BNodeInfo nodeInfo(&file);
			status = nodeInfo.InitCheck();
			if (status == B_OK) {
				if (nodeInfo.GetPreferredApp(preferred) != B_OK)
					preferred[0] = '\0';

				if (!preferred[0]) {
					// get MIME type from file
					char type[B_MIME_TYPE_LENGTH];
					if (nodeInfo.GetType(type) == B_OK) {
						BMimeType mimeType(type);
						mimeType.GetPreferredApp(preferred);
					}
				}
			}
		} else {
			// get application signature
			BAppFileInfo appInfo(&file);
			status = appInfo.InitCheck();

			if (status == B_OK && appInfo.GetSignature(preferred) != B_OK)
				preferred[0] = '\0';
		}
	}

	if (status != B_OK) {
		error_alert(B_TRANSLATE("File could not be opened"),
			status, B_STOP_ALERT);
		return status;
	}

	if (!preferred[0]) {
		error_alert(sameAs
			? B_TRANSLATE("Could not retrieve preferred application of this "
				"file")
			: B_TRANSLATE("Could not retrieve application signature"));
		return B_ERROR;
	}

	// Check if the application chosen supports this type

	BMimeType mimeType(forType);
	bool found = false;

	BMessage applications;
	if (mimeType.GetSupportingApps(&applications) == B_OK
		&& is_application_in_message(applications, preferred))
		found = true;

	applications.MakeEmpty();

	if (!found && mimeType.GetWildcardApps(&applications) == B_OK
		&& is_application_in_message(applications, preferred))
		found = true;

	if (!found) {
		// warn user
		BMimeType appType(preferred);
		char description[B_MIME_TYPE_LENGTH];
		if (appType.GetShortDescription(description) != B_OK)
			description[0] = '\0';

		char warning[512];
		snprintf(warning, sizeof(warning), B_TRANSLATE("The application "
			"\"%s\" does not support this file type.\n"
			"Are you sure you want to set it anyway?"),
			description[0] ? description : preferred);

		BAlert* alert = new BAlert(B_TRANSLATE("FileTypes request"), warning,
			B_TRANSLATE("Set preferred application"), B_TRANSLATE("Cancel"),
			NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetShortcut(1, B_ESCAPE);
		if (alert->Go() == 1)
			return B_ERROR;
	}

	preferredApp = preferred;
	return B_OK;
}

