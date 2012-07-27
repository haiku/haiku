#include "GroupedMenu.h"

#include <stdlib.h>
#include <string.h>


using namespace BPrivate;


TMenuItemGroup::TMenuItemGroup(const char* name)
	:
	fMenu(NULL),
	fFirstItemIndex(-1),
	fItemsTotal(0),
	fHasSeparator(false)
{
	if (name != NULL && name[0] != '\0')
		fName = strdup(name);
	else
		fName = NULL;
}


TMenuItemGroup::~TMenuItemGroup()
{
	free((char*)fName);
	
	if (fMenu == NULL) {
		BMenuItem* item;
		while ((item = RemoveItem(0L)) != NULL)
			delete item;
	}
}


bool
TMenuItemGroup::AddItem(BMenuItem* item)
{
	if (!fList.AddItem(item))
		return false;

	if (fMenu)
		fMenu->AddGroupItem(this, item, fList.IndexOf(item));

	fItemsTotal++;
	return true;
}


bool
TMenuItemGroup::AddItem(BMenuItem* item, int32 atIndex)
{
	if (!fList.AddItem(item, atIndex))
		return false;

	if (fMenu)
		fMenu->AddGroupItem(this, item, atIndex);

	fItemsTotal++;
	return true;
}


bool
TMenuItemGroup::AddItem(BMenu* menu)
{
	BMenuItem* item = new BMenuItem(menu);
	if (item == NULL)
		return false;

	if (!AddItem(item)) {
		delete item;
		return false;
	}

	return true;
}


bool
TMenuItemGroup::AddItem(BMenu* menu, int32 atIndex)
{
	BMenuItem* item = new BMenuItem(menu);
	if (item == NULL)
		return false;

	if (!AddItem(item, atIndex)) {
		delete item;
		return false;
	}

	return true;
}


bool
TMenuItemGroup::RemoveItem(BMenuItem* item)
{
	if (fMenu)
		fMenu->RemoveGroupItem(this, item);

	return fList.RemoveItem(item);
}


bool
TMenuItemGroup::RemoveItem(BMenu* menu)
{
	BMenuItem* item = menu->Superitem();
	if (item == NULL)
		return false;

	return RemoveItem(item);
}


BMenuItem*
TMenuItemGroup::RemoveItem(int32 index)
{
	BMenuItem* item = ItemAt(index);
	if (item == NULL)
		return NULL;

	if (RemoveItem(item))
		return item;

	return NULL;
}


BMenuItem*
TMenuItemGroup::ItemAt(int32 index)
{
	return static_cast<BMenuItem*>(fList.ItemAt(index));
}


int32
TMenuItemGroup::CountItems()
{
	return fList.CountItems();
}


void
TMenuItemGroup::Separated(bool separated)
{
	if (separated == fHasSeparator)
		return;

	fHasSeparator = separated;

	if (separated)
		fItemsTotal++;
	else
		fItemsTotal--;
}


bool
TMenuItemGroup::HasSeparator()
{
	return fHasSeparator;
}


//	#pragma mark -


TGroupedMenu::TGroupedMenu(const char* name)
	: BMenu(name)
{
}


TGroupedMenu::~TGroupedMenu()
{
	TMenuItemGroup* group;
	while ((group = static_cast<TMenuItemGroup*>(fGroups.RemoveItem(0L))) != NULL)
		delete group;
}


bool
TGroupedMenu::AddGroup(TMenuItemGroup* group)
{
	if (!fGroups.AddItem(group))
		return false;

	group->fMenu = this;

	for (int32 i = 0; i < group->CountItems(); i++) {
		AddGroupItem(group, group->ItemAt(i), i);
	}

	return true;
}


bool
TGroupedMenu::AddGroup(TMenuItemGroup* group, int32 atIndex)
{
	if (!fGroups.AddItem(group, atIndex))
		return false;

	group->fMenu = this;

	for (int32 i = 0; i < group->CountItems(); i++) {
		AddGroupItem(group, group->ItemAt(i), i);
	}

	return true;
}


bool
TGroupedMenu::RemoveGroup(TMenuItemGroup* group)
{
	if (group->HasSeparator()) {
		delete RemoveItem(group->fFirstItemIndex);
		group->Separated(false);
	}

	group->fMenu = NULL;
	group->fFirstItemIndex = -1;

	for (int32 i = 0; i < group->CountItems(); i++) {
		RemoveItem(group->ItemAt(i));
	}

	return fGroups.RemoveItem(group);
}


TMenuItemGroup*
TGroupedMenu::GroupAt(int32 index)
{
	return static_cast<TMenuItemGroup*>(fGroups.ItemAt(index));
}


int32
TGroupedMenu::CountGroups()
{
	return fGroups.CountItems();
}


void
TGroupedMenu::AddGroupItem(TMenuItemGroup* group, BMenuItem* item, int32 atIndex)
{
	int32 groupIndex = fGroups.IndexOf(group);
	bool addSeparator = false;

	if (group->fFirstItemIndex == -1) {
		// find new home for this group
		if (groupIndex > 0) {
			// add this group after an existing one
			TMenuItemGroup* previous = GroupAt(groupIndex - 1);
			group->fFirstItemIndex = previous->fFirstItemIndex + previous->fItemsTotal;
			addSeparator = true;
		} else {
			// this is the first group
			TMenuItemGroup* successor = GroupAt(groupIndex + 1);
			if (successor != NULL) {
				group->fFirstItemIndex = successor->fFirstItemIndex;
				if (successor->fHasSeparator) {
					// we'll need one as well
					addSeparator = true;
				}
			} else {
				group->fFirstItemIndex = CountItems();
				if (group->fFirstItemIndex > 0)
					addSeparator = true;
			}
		}

		if (addSeparator) {
			AddItem(new BSeparatorItem(), group->fFirstItemIndex);
			group->Separated(true);
		}
	}

	// insert item for real

	AddItem(item, atIndex + group->fFirstItemIndex + (group->HasSeparator() ? 1 : 0));

	// move the groups after this one

	for (int32 i = groupIndex + 1; i < CountGroups(); i++) {
		group = GroupAt(i);
		group->fFirstItemIndex += addSeparator ? 2 : 1;
	}
}


void
TGroupedMenu::RemoveGroupItem(TMenuItemGroup* group, BMenuItem* item)
{
	int32 groupIndex = fGroups.IndexOf(group);
	bool removedSeparator = false;

	if (group->CountItems() == 1) {
		// this is the last item
		if (group->HasSeparator()) {
			RemoveItem(group->fFirstItemIndex);
			group->Separated(false);
			removedSeparator = true;
		}
	}

	// insert item for real

	RemoveItem(item);

	// move the groups after this one

	for (int32 i = groupIndex + 1; i < CountGroups(); i++) {
		group = GroupAt(i);
		group->fFirstItemIndex -= removedSeparator ? 2 : 1;
	}
}

