/*
 * Copyright 2004-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_MENU_H
#define KERNEL_BOOT_MENU_H


#include <SupportDefs.h>
#include <util/DoublyLinkedList.h>


class Menu;
class MenuItem;

typedef bool (*menu_item_hook)(Menu* menu, MenuItem* item);
typedef void (*shortcut_hook)(char key);


enum menu_item_type {
	MENU_ITEM_STANDARD = 1,
	MENU_ITEM_MARKABLE,
	MENU_ITEM_TITLE,
	MENU_ITEM_NO_CHOICE,
	MENU_ITEM_SEPARATOR,
};


class MenuItem : public DoublyLinkedListLinkImpl<MenuItem> {
public:
								MenuItem(const char* label = NULL,
									Menu* subMenu = NULL);
								~MenuItem();

			void				SetTarget(menu_item_hook target);
			menu_item_hook		Target() const { return fTarget; }

			void				SetMarked(bool marked);
			bool				IsMarked() const { return fIsMarked; }

			void				Select(bool selected);
			bool				IsSelected() const { return fIsSelected; }

			void				SetEnabled(bool enabled);
			bool				IsEnabled() const { return fIsEnabled; }

			void				SetType(menu_item_type type);
			menu_item_type		Type() const { return fType; }

			void				SetData(const void* data);
			const void*			Data() const { return fData; }

			void				SetHelpText(const char* text);
			const char*			HelpText() const { return fHelpText; }

			void				SetShortcut(char key);
			char				Shortcut() const { return fShortcut; }

			const char*			Label() const { return fLabel; }
			Menu*				Submenu() const { return fSubMenu; }

private:
			friend class Menu;
			void				SetMenu(Menu* menu);

private:
			const char*			fLabel;
			menu_item_hook		fTarget;
			bool				fIsMarked;
			bool				fIsSelected;
			bool				fIsEnabled;
			menu_item_type		fType;
			Menu*				fMenu;
			Menu*				fSubMenu;
			const void*			fData;
			const char*			fHelpText;
			char				fShortcut;
};


typedef DoublyLinkedList<MenuItem> MenuItemList;
typedef MenuItemList::Iterator MenuItemIterator;


enum menu_type {
	MAIN_MENU = 1,
	SAFE_MODE_MENU,
	STANDARD_MENU,
	CHOICE_MENU,
};


class Menu {
public:
								Menu(menu_type type, const char* title = NULL);
								~Menu();

			menu_type			Type() const { return fType; }

			void				Hide() { fIsHidden = true; }
			void				Show() { fIsHidden = false; }
			bool				IsHidden() const { return fIsHidden; }

			MenuItemIterator	ItemIterator() { return fItems.GetIterator(); }
			MenuItem*			ItemAt(int32 index);
			int32				IndexOf(MenuItem* item);
			int32				CountItems() const;

			MenuItem*			FindItem(const char* label);
			MenuItem*			FindMarked();
			MenuItem*			FindSelected(int32* _index = NULL);

			void				AddItem(MenuItem* item);
			status_t			AddSeparatorItem();

			MenuItem*			RemoveItemAt(int32 index);
			void				RemoveItem(MenuItem* item);

			MenuItem*			Superitem() const { return fSuperItem; }
			Menu*				Supermenu() const
									{ return fSuperItem
										? fSuperItem->fMenu : NULL; }

			const char*			Title() const { return fTitle; }

			void				SetChoiceText(const char* text)
									{ fChoiceText = text; }
			const char*			ChoiceText() const { return fChoiceText; }

			void				AddShortcut(char key, shortcut_hook function);
			shortcut_hook		FindShortcut(char key) const;
			MenuItem*			FindItemByShortcut(char key);

			void				Run();

private:
			friend class MenuItem;
			void				Draw(MenuItem* item);

private:
			struct shortcut {
				shortcut*		next;
				shortcut_hook	function;
				char			key;
			};

			const char*			fTitle;
			const char*			fChoiceText;
			int32				fCount;
			bool				fIsHidden;
			MenuItemList		fItems;
			menu_type			fType;
			MenuItem*			fSuperItem;
			shortcut*			fShortcuts;
};


#endif	/* KERNEL_BOOT_MENU_H */
