/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered
trademarks of Be Incorporated in the United States and other countries. Other
brand product names are registered trademarks or trademarks of their
respective holders. All rights reserved.
*/
#ifndef _GROUPED_MENU_H
#define _GROUPED_MENU_H


#include <Menu.h>
#include <MenuItem.h>
#include <List.h>


namespace BPrivate {

class TGroupedMenu;

class TMenuItemGroup {
	public:
		TMenuItemGroup(const char* name);
		~TMenuItemGroup();

		bool AddItem(BMenuItem* item);
		bool AddItem(BMenuItem* item, int32 atIndex);
		bool AddItem(BMenu* menu);
		bool AddItem(BMenu* menu, int32 atIndex);

		bool RemoveItem(BMenuItem* item);
		bool RemoveItem(BMenu* menu);
		BMenuItem* RemoveItem(int32 index);

		BMenuItem* ItemAt(int32 index);
		int32 CountItems();

	private:
		friend class TGroupedMenu;
		void Separated(bool separated);
		bool HasSeparator();

	private:
		const char*		fName;
		BList			fList;
		TGroupedMenu*	fMenu;
		int32			fFirstItemIndex;
		int32			fItemsTotal;
		bool			fHasSeparator;
};


class TGroupedMenu : public BMenu {
	public:
		TGroupedMenu(const char* name);
		~TGroupedMenu();

		bool AddGroup(TMenuItemGroup* group);
		bool AddGroup(TMenuItemGroup* group, int32 atIndex);

		bool RemoveGroup(TMenuItemGroup* group);

		TMenuItemGroup* GroupAt(int32 index);
		int32 CountGroups();

	private:
		friend class TMenuItemGroup;
		void AddGroupItem(TMenuItemGroup* group, BMenuItem* item,
			int32 atIndex);
		void RemoveGroupItem(TMenuItemGroup* group, BMenuItem* item);

	private:
		BList	fGroups;
};

}	// namespace BPrivate


#endif	// _GROUPED_MENU_H
