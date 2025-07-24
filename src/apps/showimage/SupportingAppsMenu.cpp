/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2023, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "IconMenuItem.h"
#include "SupportingAppsMenu.h"

#include <AppFileInfo.h>
#include <Menu.h>
#include <MenuItem.h>


static int
compare_menu_items(const void* _a, const void* _b)
{
	BMenuItem* a = *(BMenuItem**)_a;
	BMenuItem* b = *(BMenuItem**)_b;

	return strcasecmp(a->Label(), b->Label());
}


static IconMenuItem*
create_application_item(const char* signature, uint32 what)
{
	char name[B_FILE_NAME_LENGTH];

	BMessage* message = new BMessage(what);
	message->AddString("signature", signature);

	BMimeType applicationType(signature);
	if (applicationType.GetShortDescription(name) == B_OK)
		return new IconMenuItem(name, message, signature);

	return new IconMenuItem(signature, message, signature);
}


//	#pragma mark - Public functions


void
update_supporting_apps_menu(BMenu* menu, BMimeType* type, uint32 what, BHandler* target)
{
	// clear menu
	for (int32 i = menu->CountItems(); i-- > 0;)
		delete menu->RemoveItem(i);

	// fill it again
	BMessage applications;
	if (type == NULL || type->GetSupportingApps(&applications) != B_OK)
		return;

	int32 lastFullSupport;
	if (applications.FindInt32("be:sub", &lastFullSupport) != B_OK)
		lastFullSupport = -1;

	BList subList;
	BList superList;

	const char* signature;
	int32 i = 0;
	while (applications.FindString("applications", i, &signature) == B_OK) {
		if (!strcasecmp(signature, kApplicationSignature)) {
			i++;
			continue;
		}

		BMenuItem* item = create_application_item(signature, what);
		item->SetTarget(target);

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
	for (int32 i = 0; i < subList.CountItems(); i++)
		menu->AddItem((BMenuItem*)subList.ItemAt(i));

	// Add type separator
	if (superList.CountItems() != 0 && subList.CountItems() != 0)
		menu->AddSeparatorItem();

	for (int32 i = 0; i < superList.CountItems(); i++)
		menu->AddItem((BMenuItem*)superList.ItemAt(i));

	for (int32 index = 0; index < menu->CountItems(); index++) {
		BMenuItem* item = menu->ItemAt(index);
		if (item == NULL)
			continue;

		if (item->Message() == NULL
			|| item->Message()->FindString("signature", &signature) != B_OK)
			continue;
	}
}
