/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef KERNEL_BOOT_MENU_H
#define KERNEL_BOOT_MENU_H


#include <SupportDefs.h>
#include <util/DoublyLinkedList.h>


class Menu;
class MenuItem;

typedef bool (*menu_item_hook)(Menu *, MenuItem *);

enum menu_item_type {
	MENU_ITEM_STANDARD = 1,
	MENU_ITEM_MARKABLE,
	MENU_ITEM_TITLE,
	MENU_ITEM_SEPARATOR,
};

class MenuItem {
	public:
		MenuItem(const char *label = NULL, Menu *subMenu = NULL);
		~MenuItem();

		void SetTarget(menu_item_hook target);

		void SetMarked(bool marked);
		bool IsMarked() const { return fIsMarked; }

		void Select(bool selected);
		bool IsSelected() const { return fIsSelected; }

		void SetType(menu_item_type type);
		menu_item_type Type() const { return fType; }

		const char *Label() const { return fLabel; }
		Menu *Submenu() const { return fSubMenu; }

		DoublyLinked::Link fLink;

	private:
		friend class Menu;
		void SetMenu(Menu *menu);

		const char		*fLabel;
		menu_item_hook	fTarget;
		bool			fIsMarked;
		bool			fIsSelected;
		menu_item_type	fType;
		Menu			*fMenu, *fSubMenu;
};

typedef DoublyLinked::List<MenuItem> MenuItemList;
typedef DoublyLinked::Iterator<MenuItem> MenuItemIterator;

enum menu_type {
	MAIN_MENU = 1,
	SAFE_MODE_MENU,
	STANDARD_MENU,
	CHOICE_MENU,
};

class Menu {
	public:
		Menu(menu_type type, const char *title = NULL);
		~Menu();

		menu_type Type() const { return fType; }

		void Hide() { fIsHidden = true; }
		void Show() { fIsHidden = false; }
		bool IsHidden() const { return fIsHidden; }

		MenuItemIterator ItemIterator() { return fItems.Iterator(); }
		MenuItem *ItemAt(int32 index);
		int32 IndexOf(MenuItem *item);
		int32 CountItems() const;

		MenuItem *FindMarked();
		MenuItem *FindSelected(int32 *_index = NULL);

		void AddItem(MenuItem *item);
		status_t AddSeparatorItem();

		MenuItem *RemoveItemAt(int32 index);
		void RemoveItem(MenuItem *item);

		const char *Title() const { return fTitle; }

		void Run();

	private:
		friend class MenuItem;
		void Draw(MenuItem *item);

		const char		*fTitle;
		int32			fCount;
		bool			fIsHidden;
		MenuItemList	fItems;
		menu_type		fType;
};

#endif	/* KERNEL_BOOT_MENU_H */
