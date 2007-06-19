/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "MenuFieldTest.h"

#include <stdio.h>

#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>

#include "CheckBox.h"
#include "GroupView.h"


enum {
	MSG_CHANGE_LABEL_TEXT	= 'chlt',
	MSG_CHANGE_LABEL_FONT	= 'chlf'
};


// constructor
MenuFieldTest::MenuFieldTest()
	: ControlTest("MenuField"),
	  fLongTextCheckBox(NULL),
	  fBigFontCheckBox(NULL),
	  fDefaultFont(NULL),
	  fBigFont(NULL)
{
	BMenu* menu = new BMenu("The Menu");

	// add a few items
	for (int32 i = 0; i < 10; i++) {
		BString itemText("Menu item ");
		itemText << i;
		menu->AddItem(new BMenuItem(itemText.String(), NULL));
	}

	fMenuField = new BMenuField("", menu);

	SetView(fMenuField);
}


// destructor
MenuFieldTest::~MenuFieldTest()
{
	delete fDefaultFont;
	delete fBigFont;
}


// CreateTest
Test*
MenuFieldTest::CreateTest()
{
	return new MenuFieldTest;
}


// ActivateTest
void
MenuFieldTest::ActivateTest(View* controls)
{
	GroupView* group = new GroupView(B_VERTICAL);
	group->SetFrame(controls->Bounds());
	group->SetSpacing(0, 8);
	controls->AddChild(group);

	// BMenuField sets its background color to that of its parent in
	// AttachedToWindow(). Override.
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	fMenuField->SetViewColor(background);
	fMenuField->SetLowColor(background);

	// long text
	fLongTextCheckBox = new LabeledCheckBox("Long label text",
		new BMessage(MSG_CHANGE_LABEL_TEXT), this);
	group->AddChild(fLongTextCheckBox);

	// big font
	fBigFontCheckBox = new LabeledCheckBox("Big label font",
		new BMessage(MSG_CHANGE_LABEL_FONT), this);
	group->AddChild(fBigFontCheckBox);

	UpdateLabelText();
	UpdateLabelFont();

	group->AddChild(new Glue());
}


// DectivateTest
void
MenuFieldTest::DectivateTest()
{
}


// MessageReceived
void
MenuFieldTest::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_CHANGE_LABEL_TEXT:
			UpdateLabelText();
			break;
		case MSG_CHANGE_LABEL_FONT:
			UpdateLabelFont();
			break;
		default:
			Test::MessageReceived(message);
			break;
	}
}


// UpdateLabelText
void
MenuFieldTest::UpdateLabelText()
{
	if (!fLongTextCheckBox || !fMenuField)
		return;

	fMenuField->SetLabel(fLongTextCheckBox->IsSelected()
		? "Pretty long menu field label"
		: "Short label");
}


// UpdateLabelFont
void
MenuFieldTest::UpdateLabelFont()
{
	if (!fBigFontCheckBox || !fMenuField || !fMenuField->Window())
		return;

	// get default font lazily
	if (!fDefaultFont) {
		fDefaultFont = new BFont;
		fMenuField->GetFont(fDefaultFont);

		fBigFont = new BFont(fDefaultFont);
		fBigFont->SetSize(20);
	}

	// set font
	fMenuField->SetFont(fBigFontCheckBox->IsSelected()
		? fBigFont : fDefaultFont);
	fMenuField->InvalidateLayout();
	fMenuField->Invalidate();
}
