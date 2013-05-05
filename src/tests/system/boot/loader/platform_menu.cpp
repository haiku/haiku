/*
 * Copyright 2004-2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "RootFileSystem.h"

#include <boot/platform.h>
#include <boot/vfs.h>
#include <boot/menu.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>


const char *kNormalColor = "\33[0m";
const char *kDisabledColor = "\33[31m";	// red
const char *kTitleColor = "\33[34m";	// blue


extern RootFileSystem *gRoot;
bool gShowMenu = false;


static void
print_item_at(int32 line, MenuItem *item, bool clearHelp = true)
{
	if (!item->IsEnabled())
		printf("%s    ", kDisabledColor);
	else
		printf("%2ld. ", line);

	if (item->Type() == MENU_ITEM_MARKABLE) {
		printf(" [");
		printf("%c", item->IsMarked() ? 'x' : ' ');
		printf("] ");
	} else
		printf(" ");

	printf(item->Label());

	if (item->Submenu() && item->Submenu()->Type() == CHOICE_MENU) {
		// show the current choice (if any)
		Menu *subMenu = item->Submenu();
		MenuItem *subItem = NULL;

		for (int32 i = subMenu->CountItems(); i-- > 0; ) {
			subItem = subMenu->ItemAt(i);
			if (subItem != NULL && subItem->IsMarked())
				break;
		}

		printf(" (Current: ");
		printf(subItem != NULL ? subItem->Label() : "None");
		putchar(')');
	}

	if (!item->IsEnabled())
		printf(kNormalColor);
	putchar('\n');
}


static void
draw_menu(Menu *menu)
{
	printf("\n %s--- ", kTitleColor);
	if (menu->Title())
		printf("%s", menu->Title());
	else
		printf("Welcome To The Haiku Bootloader");
	printf(" ---%s\n\n", kNormalColor);

	MenuItemIterator iterator = menu->ItemIterator();
	MenuItem *item;
	int32 i = 0;

	while ((item = iterator.Next()) != NULL) {
		if (item->Type() == MENU_ITEM_SEPARATOR) {
			putchar('\n');
			i++;
			continue;
		}

		print_item_at(i++, item, false);
	}

	int32 selected = -1;
	menu->FindSelected(&selected);
	printf("\n[%ld]? ", selected);
}


bool
dump_devices_hook(Menu *menu, MenuItem *item)
{
	puts("List of known root directories:");

	void *cookie;
	if (gRoot->Open(&cookie, O_RDONLY) == B_OK) {
		Directory *directory;
		while (gRoot->GetNextNode(cookie, (Node **)&directory) == B_OK) {
			char name[256];
			if (directory->GetName(name, sizeof(name)) == B_OK)
				printf("%s:: %s (%p)%s\n", kTitleColor, name, directory, kNormalColor);

			void *subCookie;
			if (directory->Open(&subCookie, O_RDONLY) == B_OK) {
				while (directory->GetNextEntry(subCookie, name, sizeof(name)) == B_OK) {
					printf("\t%s\n", name);
				}
				directory->Close(subCookie);
			}
		}
		gRoot->Close(cookie);
	}

	return false;
}


//	#pragma mark -


void
platform_add_menus(Menu *menu)
{
	MenuItem *item;

	switch (menu->Type()) {
		case MAIN_MENU:
			menu->AddItem(item = new MenuItem("Dump all recognized volumes"));
			item->SetTarget(dump_devices_hook);
			break;
		default:
			break;
	}
}


void
platform_update_menu_item(Menu *menu, MenuItem *item)
{
}


void
platform_run_menu(Menu *menu)
{
	// Get selected entry, or select the last one, if there is none
	int32 selected;
	MenuItem *item = menu->FindSelected(&selected);
	if (item == NULL) {
		selected = menu->CountItems() - 1;
		item = menu->ItemAt(selected);
		if (item != NULL)
			item->Select(true);
	}

	while (true) {
		draw_menu(menu);

		char buffer[32];
		if (fgets(buffer, sizeof(buffer), stdin) == NULL)
			return;

		if (buffer[0] != '\n')
			selected = atoi(buffer);

		item = menu->ItemAt(selected);
		if (item == NULL) {
			printf("Invalid choice.");
			continue;
		}

		item->Select(true);

		// leave the menu
		if (item->Submenu() != NULL) {
			menu->Hide();

			platform_run_menu(item->Submenu());
			if (item->Target() != NULL)
				(*item->Target())(menu, item);

			// restore current menu
			menu->FindSelected(&selected);
			menu->Show();
		} else if (item->Type() == MENU_ITEM_MARKABLE) {
			// toggle state
			item->SetMarked(!item->IsMarked());

			if (item->Target() != NULL)
				(*item->Target())(menu, item);
		} else if (item->Target() == NULL || (*item->Target())(menu, item))
			break;
	}
}


size_t
platform_get_user_input_text(Menu* menu, MenuItem* item, char* buffer,
	size_t bufferSize)
{
	return 0;
}
