/*
 * Copyright 2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */

#include "ModifierKeysWindow.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Catalog.h>
#include <GroupLayout.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <LayoutBuilder.h>
#include <MenuItem.h>
#include <Message.h>
#include <Size.h>
#include <StringView.h>

#include "KeymapApplication.h"

enum {
	MENU_ITEM_CAPS_LOCK,
	MENU_ITEM_CONTROL,
	MENU_ITEM_OPTION,
	MENU_ITEM_COMMAND,
	MENU_ITEM_SEPERATOR,
	MENU_ITEM_DISABLED
};

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Modifier keys window"


static const uint32 kMsgCapsLockCapsLock	= 'clcl';
static const uint32 kMsgCapsLockControl		= 'clct';
static const uint32 kMsgCapsLockOption		= 'clop';
static const uint32 kMsgCapsLockCommand		= 'clcm';
static const uint32 kMsgCapsLockDisabled	= 'clds';

static const uint32 kMsgControlCapsLock		= 'ctcl';
static const uint32 kMsgControlControl		= 'ctct';
static const uint32 kMsgControlOption		= 'ctop';
static const uint32 kMsgControlCommand		= 'ctcm';

static const uint32 kMsgOptionCapsLock		= 'opcl';
static const uint32 kMsgOptionControl		= 'opct';
static const uint32 kMsgOptionOption		= 'opop';
static const uint32 kMsgOptionCommand		= 'opcm';

static const uint32 kMsgCommmandCapsLock	= 'cmcl';
static const uint32 kMsgCommmandControl		= 'cmct';
static const uint32 kMsgCommmandOption		= 'cmop';
static const uint32 kMsgCommmandCommand		= 'cmcm';

static const uint32 kMsgApplyModifiers 		= 'apmd';
static const uint32 kMsgRevertModifiers		= 'rvmd';


ModifierKeysWindow::ModifierKeysWindow()
	:
	BWindow(BRect(80, 50, 400, 260), B_TRANSLATE("Modifiers Keys"),
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE
			| B_AUTO_UPDATE_SIZE_LIMITS)
{
	get_key_map(&fCurrentMap, &fCurrentBuffer);
	get_key_map(&fSavedMap, &fSavedBuffer);

	BStringView* capsLockStringView
		= new BStringView("caps", B_TRANSLATE("Caps Lock:"));
	capsLockStringView->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BStringView* controlStringView
		= new BStringView("control", B_TRANSLATE("Control:"));
	controlStringView->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BStringView* optionStringView
		= new BStringView("option", B_TRANSLATE("Option:"));
	optionStringView->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BStringView* commandStringView
		= new BStringView("command", B_TRANSLATE("Command:"));
	commandStringView->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fCancelButton = new BButton("CancelButton", B_TRANSLATE("Cancel"),
		new BMessage(B_QUIT_REQUESTED));

	fRevertButton = new BButton("revertButton", B_TRANSLATE("Revert"),
		new BMessage(kMsgRevertModifiers));
	fRevertButton->SetEnabled(false);

	fOkButton = new BButton("OkButton", B_TRANSLATE("Ok"),
		new BMessage(kMsgApplyModifiers));
	fOkButton->MakeDefault(true);

	// Build the layout
	SetLayout(new BGroupLayout(B_VERTICAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(BGridLayoutBuilder(10, 10)
			.Add(capsLockStringView, 0, 0)
			.Add(_CreateCapsLockMenuField(), 1, 0)

			.Add(controlStringView, 0, 1)
			.Add(_CreateControlMenuField(), 1, 1)

			.Add(optionStringView, 0, 2)
			.Add(_CreateOptionMenuField(), 1, 2)

			.Add(commandStringView, 0, 3)
			.Add(_CreateCommandMenuField(), 1, 3)
		)
		.AddGlue()
		.AddGroup(B_HORIZONTAL, 10)
			.Add(fCancelButton)
			.AddGlue()
			.Add(fRevertButton)
			.Add(fOkButton)
		.End()
		.SetInsets(10, 10, 10, 10)
	);

	_MarkMenuItems();

	CenterOnScreen();
}


ModifierKeysWindow::~ModifierKeysWindow()
{
	be_app->PostMessage(kMsgCloseModifierKeysWindow);
}


void
ModifierKeysWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		// Caps Lock
		case kMsgCapsLockCapsLock:
			fCurrentMap->caps_key = 0x3b;
			fCapsLockMenu->ItemAt(MENU_ITEM_CAPS_LOCK)->SetMarked(true);
			_EnableRevertButton();
			break;

		case kMsgCapsLockControl:
			fCurrentMap->caps_key = 0x5c;
			fCapsLockMenu->ItemAt(MENU_ITEM_CONTROL)->SetMarked(true);
			_EnableRevertButton();
			break;

		case kMsgCapsLockOption:
			fCurrentMap->caps_key = 0x66;
			fCapsLockMenu->ItemAt(MENU_ITEM_OPTION)->SetMarked(true);
			_EnableRevertButton();
			break;

		case kMsgCapsLockCommand:
			fCurrentMap->caps_key = 0x5d;
			fCapsLockMenu->ItemAt(MENU_ITEM_COMMAND)->SetMarked(true);
			_EnableRevertButton();
			break;

		case kMsgCapsLockDisabled:
			fCurrentMap->caps_key = 0;
			fCapsLockMenu->ItemAt(MENU_ITEM_DISABLED)->SetMarked(true);
			_EnableRevertButton();
			break;

		// Control
		case kMsgControlCapsLock:
			fCurrentMap->left_control_key = 0x3b;
			fControlMenu->ItemAt(MENU_ITEM_CAPS_LOCK)->SetMarked(true);
			_EnableRevertButton();
			break;

		case kMsgControlControl:
			fCurrentMap->left_control_key = 0x5c;
			fCurrentMap->right_control_key = 0x60;
			fControlMenu->ItemAt(MENU_ITEM_CONTROL)->SetMarked(true);
			_EnableRevertButton();
			break;

		case kMsgControlOption:
			fCurrentMap->left_control_key = 0x66;
			fCurrentMap->right_control_key = 0x67;
			fControlMenu->ItemAt(MENU_ITEM_OPTION)->SetMarked(true);
			_EnableRevertButton();
			break;

		case kMsgControlCommand:
			fCurrentMap->left_control_key = 0x5d;
			fCurrentMap->right_control_key = 0x5f;
			fControlMenu->ItemAt(MENU_ITEM_COMMAND)->SetMarked(true);
			_EnableRevertButton();
			break;

		// Option
		case kMsgOptionCapsLock:
			fCurrentMap->left_option_key = 0x3b;
			fOptionMenu->ItemAt(MENU_ITEM_CAPS_LOCK)->SetMarked(true);
			_EnableRevertButton();
			break;

		case kMsgOptionControl:
			fCurrentMap->left_option_key = 0x5c;
			fCurrentMap->right_option_key = 0x60;
			fOptionMenu->ItemAt(MENU_ITEM_CONTROL)->SetMarked(true);
			_EnableRevertButton();
			break;

		case kMsgOptionOption:
			fCurrentMap->left_option_key = 0x66;
			fCurrentMap->right_option_key = 0x67;
			fOptionMenu->ItemAt(MENU_ITEM_OPTION)->SetMarked(true);
			_EnableRevertButton();
			break;

		case kMsgOptionCommand:
			fCurrentMap->left_option_key = 0x5d;
			fCurrentMap->right_option_key = 0x5f;
			fOptionMenu->ItemAt(MENU_ITEM_COMMAND)->SetMarked(true);
			_EnableRevertButton();
			break;

		// Command
		case kMsgCommmandCapsLock:
			fCurrentMap->left_command_key = 0x3b;
			fCommandMenu->ItemAt(MENU_ITEM_CAPS_LOCK)->SetMarked(true);
			_EnableRevertButton();
			break;

		case kMsgCommmandControl:
			fCurrentMap->left_command_key = 0x5c;
			fCurrentMap->right_command_key = 0x60;
			fCommandMenu->ItemAt(MENU_ITEM_CONTROL)->SetMarked(true);
			_EnableRevertButton();
			break;

		case kMsgCommmandOption:
			fCurrentMap->left_command_key = 0x66;
			fCurrentMap->right_command_key = 0x67;
			fCommandMenu->ItemAt(MENU_ITEM_OPTION)->SetMarked(true);
			_EnableRevertButton();
			break;

		case kMsgCommmandCommand:
			fCurrentMap->left_command_key = 0x5d;
			fCurrentMap->right_command_key = 0x5f;
			fCommandMenu->ItemAt(MENU_ITEM_COMMAND)->SetMarked(true);
			_EnableRevertButton();
			break;

		// Ok button
		case kMsgApplyModifiers:
		{
			BMessage* updateModifiers = new BMessage(kMsgUpdateModifiers);

			if (fCurrentMap->caps_key != fSavedMap->caps_key)
				updateModifiers->AddUInt32("caps_key", fCurrentMap->caps_key);

			if (fCurrentMap->left_control_key != fSavedMap->left_control_key) {
				updateModifiers->AddUInt32("left_control_key",
					fCurrentMap->left_control_key);
			}

			if (fCurrentMap->right_control_key
				!= fSavedMap->right_control_key) {
				updateModifiers->AddUInt32("right_control_key",
					fCurrentMap->right_control_key);
			}

			if (fCurrentMap->left_option_key != fSavedMap->left_option_key) {
				updateModifiers->AddUInt32("left_option_key",
					fCurrentMap->left_option_key);
			}

			if (fCurrentMap->right_option_key != fSavedMap->right_option_key) {
				updateModifiers->AddUInt32("right_option_key",
					fCurrentMap->right_option_key);
			}

			if (fCurrentMap->left_command_key != fSavedMap->left_command_key) {
				updateModifiers->AddUInt32("left_command_key",
					fCurrentMap->left_command_key);
			}

			if (fCurrentMap->right_command_key
				!= fSavedMap->right_command_key) {
				updateModifiers->AddUInt32("right_command_key",
					fCurrentMap->right_command_key);
			}

			be_app->PostMessage(updateModifiers);
			this->PostMessage(B_QUIT_REQUESTED);
			break;
		}

		// Revert button
		case kMsgRevertModifiers:
			fCurrentMap->caps_key = fSavedMap->caps_key;
			fCurrentMap->left_control_key = fSavedMap->left_control_key;
			fCurrentMap->right_control_key = fSavedMap->right_control_key;
			fCurrentMap->left_option_key = fSavedMap->left_option_key;
			fCurrentMap->right_option_key = fSavedMap->right_option_key;
			fCurrentMap->left_command_key = fSavedMap->left_command_key;
			fCurrentMap->right_command_key = fSavedMap->right_command_key;

			_MarkMenuItems();
			fRevertButton->SetEnabled(false);
			break;

		default:
			BWindow::MessageReceived(message);
	}
}


BMenuField*
ModifierKeysWindow::_CreateCapsLockMenuField()
{
	fCapsLockMenu = new BPopUpMenu(B_TRANSLATE("Caps Lock"), true, true);

	BMenuItem* capsLock = new BMenuItem(B_TRANSLATE("Caps Lock"),
		new BMessage(kMsgCapsLockCapsLock));
	fCapsLockMenu->AddItem(capsLock, MENU_ITEM_CAPS_LOCK);

	BMenuItem* control = new BMenuItem(B_TRANSLATE("Control"),
		new BMessage(kMsgCapsLockControl));
	fCapsLockMenu->AddItem(control, MENU_ITEM_CONTROL);

	BMenuItem* option = new BMenuItem(B_TRANSLATE("Option"),
		new BMessage(kMsgCapsLockOption));
	fCapsLockMenu->AddItem(option, MENU_ITEM_OPTION);

	BMenuItem* command = new BMenuItem(B_TRANSLATE("Command"),
		new BMessage(kMsgCapsLockCommand));
	fCapsLockMenu->AddItem(command, MENU_ITEM_COMMAND);

	BSeparatorItem* separator = new BSeparatorItem;
	fCapsLockMenu->AddItem(separator, MENU_ITEM_SEPERATOR);

	BMenuItem* disabled = new BMenuItem(B_TRANSLATE("Disabled"),
		new BMessage(kMsgCapsLockDisabled));
	fCapsLockMenu->AddItem(disabled, MENU_ITEM_DISABLED);

	return new BMenuField(NULL, fCapsLockMenu);
}


BMenuField*
ModifierKeysWindow::_CreateControlMenuField()
{
	fControlMenu = new BPopUpMenu(B_TRANSLATE("Control"), true, true);

	BMenuItem* capsLock = new BMenuItem(B_TRANSLATE("Caps Lock"),
		new BMessage(kMsgControlCapsLock));
	fControlMenu->AddItem(capsLock, MENU_ITEM_CAPS_LOCK);

	BMenuItem* control = new BMenuItem(B_TRANSLATE("Control"),
		new BMessage(kMsgControlControl));
	fControlMenu->AddItem(control, MENU_ITEM_CONTROL);

	BMenuItem* option = new BMenuItem(B_TRANSLATE("Option"),
		new BMessage(kMsgControlOption));
	fControlMenu->AddItem(option, MENU_ITEM_OPTION);

	BMenuItem* command = new BMenuItem(B_TRANSLATE("Command"),
		new BMessage(kMsgControlCommand));
	fControlMenu->AddItem(command, MENU_ITEM_COMMAND);

	return new BMenuField(NULL, fControlMenu);
}


BMenuField*
ModifierKeysWindow::_CreateOptionMenuField()
{
	fOptionMenu = new BPopUpMenu(B_TRANSLATE("Option"), true, true);

	BMenuItem* capsLock = new BMenuItem(B_TRANSLATE("Caps Lock"),
		new BMessage(kMsgOptionCapsLock));
	fOptionMenu->AddItem(capsLock, MENU_ITEM_CAPS_LOCK);

	BMenuItem* control = new BMenuItem(B_TRANSLATE("Control"),
		new BMessage(kMsgOptionControl));
	fOptionMenu->AddItem(control, MENU_ITEM_CONTROL);

	BMenuItem* option = new BMenuItem(B_TRANSLATE("Option"),
		new BMessage(kMsgOptionOption));
	fOptionMenu->AddItem(option, MENU_ITEM_OPTION);

	BMenuItem* command = new BMenuItem(B_TRANSLATE("Command"),
		new BMessage(kMsgOptionCommand));
	fOptionMenu->AddItem(command, MENU_ITEM_COMMAND);

	return new BMenuField(NULL, fOptionMenu);
}


BMenuField*
ModifierKeysWindow::_CreateCommandMenuField()
{
	fCommandMenu = new BPopUpMenu(B_TRANSLATE("Command"), true, true);

	BMenuItem* capsLock = new BMenuItem(B_TRANSLATE("Caps Lock"),
		new BMessage(kMsgCommmandCapsLock));
	fCommandMenu->AddItem(capsLock, MENU_ITEM_CAPS_LOCK);

	BMenuItem* control = new BMenuItem(B_TRANSLATE("Control"),
		new BMessage(kMsgCommmandControl));
	fCommandMenu->AddItem(control, MENU_ITEM_CONTROL);

	BMenuItem* option = new BMenuItem(B_TRANSLATE("Option"),
		new BMessage(kMsgCommmandOption));
	fCommandMenu->AddItem(option, MENU_ITEM_OPTION);

	BMenuItem* command = new BMenuItem(B_TRANSLATE("Command"),
		new BMessage(kMsgCommmandCommand));
	fCommandMenu->AddItem(command, MENU_ITEM_COMMAND);

	return new BMenuField(NULL, fCommandMenu);
}


void
ModifierKeysWindow::_MarkMenuItems()
{
	// Caps Lock Menu
	if (fCurrentMap->caps_key == 0x3b)
		fCapsLockMenu->ItemAt(MENU_ITEM_CAPS_LOCK)->SetMarked(true);
	else if (fCurrentMap->caps_key == 0x5c)
		fCapsLockMenu->ItemAt(MENU_ITEM_CONTROL)->SetMarked(true);
	else if (fCurrentMap->caps_key == 0x66)
		fCapsLockMenu->ItemAt(MENU_ITEM_OPTION)->SetMarked(true);
	else if (fCurrentMap->caps_key == 0x5d)
		fCapsLockMenu->ItemAt(MENU_ITEM_COMMAND)->SetMarked(true);
	else if (fCurrentMap->caps_key == 0)
		fCapsLockMenu->ItemAt(MENU_ITEM_DISABLED)->SetMarked(true);

	// Control Menu
	if (fCurrentMap->left_control_key == 0x3b)
		fControlMenu->ItemAt(MENU_ITEM_CAPS_LOCK)->SetMarked(true);
	else if (fCurrentMap->left_control_key == 0x5c)
		fControlMenu->ItemAt(MENU_ITEM_CONTROL)->SetMarked(true);
	else if (fCurrentMap->left_control_key == 0x66)
		fControlMenu->ItemAt(MENU_ITEM_OPTION)->SetMarked(true);
	else if (fCurrentMap->left_control_key == 0x5d)
		fControlMenu->ItemAt(MENU_ITEM_COMMAND)->SetMarked(true);

	// Option Menu
	if (fCurrentMap->left_option_key == 0x3b)
		fOptionMenu->ItemAt(MENU_ITEM_CAPS_LOCK)->SetMarked(true);
	else if (fCurrentMap->left_option_key == 0x5c)
		fOptionMenu->ItemAt(MENU_ITEM_CONTROL)->SetMarked(true);
	else if (fCurrentMap->left_option_key == 0x66)
		fOptionMenu->ItemAt(MENU_ITEM_OPTION)->SetMarked(true);
	else if (fCurrentMap->left_option_key == 0x5d)
		fOptionMenu->ItemAt(MENU_ITEM_COMMAND)->SetMarked(true);

	// Command menu
	if (fCurrentMap->left_command_key == 0x3b)
		fCommandMenu->ItemAt(MENU_ITEM_CAPS_LOCK)->SetMarked(true);
	else if (fCurrentMap->left_command_key == 0x5c)
		fCommandMenu->ItemAt(MENU_ITEM_CONTROL)->SetMarked(true);
	else if (fCurrentMap->left_command_key == 0x66)
		fCommandMenu->ItemAt(MENU_ITEM_OPTION)->SetMarked(true);
	else if (fCurrentMap->left_command_key == 0x5d)
		fCommandMenu->ItemAt(MENU_ITEM_COMMAND)->SetMarked(true);
}


void
ModifierKeysWindow::_EnableRevertButton()
{
	if (fCurrentMap->caps_key != fSavedMap->caps_key
		|| fCurrentMap->left_control_key != fSavedMap->left_control_key
		|| fCurrentMap->right_control_key != fSavedMap->right_control_key
		|| fCurrentMap->left_option_key != fSavedMap->left_option_key
		|| fCurrentMap->right_option_key != fSavedMap->right_option_key
		|| fCurrentMap->left_command_key != fSavedMap->left_command_key
		|| fCurrentMap->right_command_key != fSavedMap->right_command_key)
		fRevertButton->SetEnabled(true);
	else
		fRevertButton->SetEnabled(false);
}
