/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "PreferredAppMenu.h"

#include <Menu.h>
#include <MenuItem.h>
#include <Mime.h>

#include <stdio.h>
#include <string.h>


static int
compare_menu_items(const void* _a, const void* _b)
{
	BMenuItem* a = *(BMenuItem**)_a;
	BMenuItem* b = *(BMenuItem**)_b;

	return strcasecmp(a->Label(), b->Label());
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

		if (preferredFrom == NULL && !strcasecmp(signature, preferred)
			|| preferredFrom != NULL && !strcasecmp(signature, preferredFrom))
			select = item;

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
	} else if (preferredFrom == NULL && preferred[0]
		|| preferredFrom && preferredFrom[0]) {
		// The preferred application is not an application that support
		// this file type!
		BMenuItem* item = create_application_item(preferredFrom
			? preferredFrom : preferred, what);

		menu->AddSeparatorItem();
		menu->AddItem(item);
		item->SetMarked(item);
	}
}

