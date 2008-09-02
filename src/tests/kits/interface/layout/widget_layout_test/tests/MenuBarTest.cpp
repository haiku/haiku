/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "MenuBarTest.h"

#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>

#include "CheckBox.h"


enum {
	MSG_THIRD_ITEM			= '3rdi',
	MSG_CHILD_MENU			= 'chmn',
	MSG_CHANGE_ITEM_TEXT	= 'chit'
};


MenuBarTest::MenuBarTest()
	: Test("MenuBar", NULL),
	  fMenuBar(new BMenuBar("The Menu"))

{
	SetView(fMenuBar);

	// add a few items
	fMenuBar->AddItem(fFirstItem = new BMenuItem("Menu item 1", NULL));
	fMenuBar->AddItem(new BMenuItem("Menu item 2", NULL));
	fThirdItem = new BMenuItem("Menu item 3", NULL);
	fChildMenu = new BMenu("Child menu");
}


Test*
MenuBarTest::CreateTest()
{
	return new MenuBarTest;
}


// ActivateTest
void
MenuBarTest::ActivateTest(View* controls)
{
	GroupView* group = new GroupView(B_VERTICAL);
	group->SetFrame(controls->Bounds());
	group->SetSpacing(0, 8);
	controls->AddChild(group);

	// third item
	fThirdItemCheckBox = new LabeledCheckBox("Third item",
		new BMessage(MSG_THIRD_ITEM), this);
	group->AddChild(fThirdItemCheckBox);

	// child menu
	fChildMenuCheckBox = new LabeledCheckBox("Child menu",
		new BMessage(MSG_CHILD_MENU), this);
	group->AddChild(fChildMenuCheckBox);

	// long text
	fLongTextCheckBox = new LabeledCheckBox("Long label text",
		new BMessage(MSG_CHANGE_ITEM_TEXT), this);
	group->AddChild(fLongTextCheckBox);

	group->AddChild(new Glue());

	UpdateThirdItem();
	UpdateLongText();
}


// DectivateTest
void
MenuBarTest::DectivateTest()
{
}


// MessageReceived
void
MenuBarTest::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_THIRD_ITEM:
			UpdateThirdItem();
			break;
		case MSG_CHILD_MENU:
			UpdateChildMenu();
			break;
		case MSG_CHANGE_ITEM_TEXT:
			UpdateLongText();
			break;
		default:
			Test::MessageReceived(message);
			break;
	}
}


// UpdateThirdItem
void
MenuBarTest::UpdateThirdItem()
{
	if (!fThirdItemCheckBox || !fMenuBar)
		return;

	if (fThirdItemCheckBox->IsSelected() == (fThirdItem->Menu() != NULL))
		return;

	if (fThirdItemCheckBox->IsSelected())
		fMenuBar->AddItem(fThirdItem);
	else
		fMenuBar->RemoveItem(fThirdItem);
}


// UpdateChildMenu
void
MenuBarTest::UpdateChildMenu()
{
	if (!fChildMenuCheckBox || !fMenuBar)
		return;

	if (fChildMenuCheckBox->IsSelected() == (fChildMenu->Supermenu() != NULL))
		return;

	if (fChildMenuCheckBox->IsSelected())
		fMenuBar->AddItem(fChildMenu);
	else
		fMenuBar->RemoveItem(fChildMenu);
}


// 	UpdateLongText
void
MenuBarTest::UpdateLongText()
{
	if (!fLongTextCheckBox || !fMenuBar)
		return;

	fFirstItem->SetLabel(fLongTextCheckBox->IsSelected()
		? "Very long text for a menu item"
		: "Menu item 1");
}
