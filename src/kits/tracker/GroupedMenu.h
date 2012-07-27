#ifndef GROUPED_MENU_H
#define GROUPED_MENU_H


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
		void AddGroupItem(TMenuItemGroup* group, BMenuItem* item, int32 atIndex);
		void RemoveGroupItem(TMenuItemGroup* group, BMenuItem* item);

	private:
		BList	fGroups;
};

}	// namespace BPrivate

#endif	// GROUPED_MENU_H
