/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include "menu.h"
#include "loader.h"
#include "RootFileSystem.h"

#include <OS.h>

#include <util/kernel_cpp.h>
#include <boot/menu.h>
#include <boot/stage2.h>
#include <boot/vfs.h>
#include <boot/platform.h>
#include <boot/stdio.h>

#include <string.h>


MenuItem::MenuItem(const char *label, Menu *subMenu)
	:
	fLabel(strdup(label)),
	fTarget(NULL),
	fIsMarked(false),
	fIsSelected(false),
	fType(MENU_ITEM_STANDARD),
	fMenu(NULL),
	fSubMenu(subMenu),
	fData(NULL),
	fHelpText(NULL)
{
}


MenuItem::~MenuItem()
{
	free(const_cast<char *>(fLabel));
}


void
MenuItem::SetTarget(menu_item_hook target)
{
	fTarget = target;
}


void
MenuItem::SetMarked(bool marked)
{
	if (fIsMarked == marked)
		return;

	if (marked && fMenu != NULL && fMenu->Type() == CHOICE_MENU) {
		// unmark previous item
		MenuItem *markedItem = fMenu->FindMarked();
		if (markedItem != NULL)
			markedItem->SetMarked(false);
	}

	fIsMarked = marked;

	if (fMenu != NULL)
		fMenu->Draw(this);
}


void
MenuItem::Select(bool selected)
{
	if (fIsSelected == selected)
		return;

	if (selected && fMenu != NULL) {
		// unselect previous item
		MenuItem *selectedItem = fMenu->FindSelected();
		if (selectedItem != NULL)
			selectedItem->Select(false);
	}

	fIsSelected = selected;

	if (fMenu != NULL)
		fMenu->Draw(this);
}


void
MenuItem::SetType(menu_item_type type)
{
	fType = type;
}


void
MenuItem::SetData(void *data)
{
	fData = data;
}


/** This sets a help text that is shown when the item is
 *	selected.
 *	Note, unlike the label, the string is not copied, it's
 *	just referenced and has to stay valid as long as the
 *	item's menu is being used.
 */

void
MenuItem::SetHelpText(const char *text)
{
	fHelpText = text;
}


void 
MenuItem::SetMenu(Menu *menu)
{
	fMenu = menu;
}


//	#pragma mark -


Menu::Menu(menu_type type, const char *title)
	:
	fTitle(title),
	fCount(0),
	fIsHidden(true),
	fType(type)
{
}


Menu::~Menu()
{
	// take all remaining items with us

	MenuItem *item;
	while ((item = fItems.RemoveHead()) != NULL) {
		delete item;
	}
}


MenuItem *
Menu::ItemAt(int32 index)
{
	if (index < 0 || index >= fCount)
		return NULL;

	MenuItemIterator iterator = ItemIterator();
	MenuItem *item;

	while ((item = iterator.Next()) != NULL) {
		if (index-- == 0)
			return item;
	}

	return NULL;
}


int32 
Menu::IndexOf(MenuItem *searchedItem)
{
	MenuItemIterator iterator = ItemIterator();
	MenuItem *item;
	int32 index = 0;

	while ((item = iterator.Next()) != NULL) {
		if (item == searchedItem)
			return index;

		index++;
	}

	return -1;
}


int32
Menu::CountItems() const
{
	return fCount;
}


MenuItem *
Menu::FindMarked()
{
	MenuItemIterator iterator = ItemIterator();
	MenuItem *item;

	while ((item = iterator.Next()) != NULL) {
		if (item->IsMarked())
			return item;
	}

	return NULL;
}


MenuItem *
Menu::FindSelected(int32 *_index)
{
	MenuItemIterator iterator = ItemIterator();
	MenuItem *item;
	int32 index = 0;

	while ((item = iterator.Next()) != NULL) {
		if (item->IsSelected()) {
			if (_index != NULL)
				*_index = index;
			return item;
		}

		index++;
	}

	return NULL;
}


void
Menu::AddItem(MenuItem *item)
{
	item->fMenu = this;
	fItems.Add(item);
	fCount++;
}


status_t
Menu::AddSeparatorItem()
{
	MenuItem *item = new MenuItem();
	if (item == NULL)
		return B_NO_MEMORY;

	item->SetType(MENU_ITEM_SEPARATOR);

	AddItem(item);
	return B_OK;
}


MenuItem *
Menu::RemoveItemAt(int32 index)
{
	if (index < 0 || index >= fCount)
		return NULL;

	MenuItemIterator iterator = ItemIterator();
	MenuItem *item;

	while ((item = iterator.Next()) != NULL) {
		if (index-- == 0) {
			RemoveItem(item);
			return item;
		}
	}

	return NULL;
}


void
Menu::RemoveItem(MenuItem *item)
{
	item->fMenu = NULL;
	fItems.Remove(item);
	fCount--;
}


void
Menu::Draw(MenuItem *item)
{
	if (!IsHidden())
		platform_update_menu_item(this, item);
}


void
Menu::Run()
{
	platform_run_menu(this);
}


//	#pragma mark -


static Menu *
add_boot_volume_menu(Directory *bootVolume)
{
	Menu *menu = new Menu(CHOICE_MENU, "Select Boot Volume");
	MenuItem *item;
	void *cookie;

	if (gRoot->Open(&cookie, O_RDONLY) == B_OK) {
		Directory *volume;
		while (gRoot->GetNextNode(cookie, (Node **)&volume) == B_OK) {
			// only list bootable volumes
			if (!is_bootable(volume))
				continue;

			char name[B_FILE_NAME_LENGTH];
			if (volume->GetName(name, sizeof(name)) == B_OK) {
				menu->AddItem(item = new MenuItem(name));
				if (volume == bootVolume) {
					item->SetMarked(true);
					item->Select(true);
				}
			}
		}
		gRoot->Close(cookie);
	}

	menu->AddSeparatorItem();
	menu->AddItem(item = new MenuItem("Rescan volumes"));
	item->SetType(MENU_ITEM_NO_CHOICE);
	menu->AddItem(item = new MenuItem("Return to main menu"));
	item->SetType(MENU_ITEM_NO_CHOICE);

	return menu;
}


static Menu *
add_safe_mode_menu()
{
	Menu *safeMenu = new Menu(SAFE_MODE_MENU, "Safe Mode Options");
	MenuItem *item;

	safeMenu->AddItem(item = new MenuItem("Disable user add-ons"));
	item->SetType(MENU_ITEM_MARKABLE);

	platform_add_menus(safeMenu);

	safeMenu->AddSeparatorItem();
	safeMenu->AddItem(item = new MenuItem("Return to main menu"));

	return safeMenu;
}


status_t
user_menu(Directory **_bootVolume)
{
	Menu *menu = new Menu(MAIN_MENU);
	MenuItem *item;

	// Add boot volume
	menu->AddItem(item = new MenuItem("Select boot volume", add_boot_volume_menu(*_bootVolume)));

	// Add safe mode
	menu->AddItem(item = new MenuItem("Select safe mode options", add_safe_mode_menu()));

	// Add platform dependent menus
	platform_add_menus(menu);

	menu->AddSeparatorItem();
	menu->AddItem(item = new MenuItem("Continue booting"));

	menu->Run();
	return B_OK;
}

