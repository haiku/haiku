/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "MenuTest.h"

#include <Menu.h>
#include <MenuItem.h>

class TestMenu : public BMenu {
public:
	TestMenu(const char* title)
		: BMenu(BRect(), title, B_FOLLOW_NONE,
		B_WILL_DRAW | B_FRAME_EVENTS | B_SUPPORTS_LAYOUT,
		B_ITEMS_IN_COLUMN, false)
	{
	}
};


MenuTest::MenuTest()
	: Test("Menu", NULL),
	  fMenu(new TestMenu("The Menu"))
{
	SetView(fMenu);

	// add a few items
	for (int32 i = 0; i < 15; i++) {
		BString itemText("menu item ");
		itemText << i;
		fMenu->AddItem(new BMenuItem(itemText.String(), NULL));
	}
}


Test*
MenuTest::CreateTest()
{
	return new MenuTest;
}


