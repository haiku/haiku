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
	MENU_ITEM_CAPS_LOCK = 0,
	MENU_ITEM_CONTROL,
	MENU_ITEM_OPTION,
	MENU_ITEM_COMMAND,
	MENU_ITEM_SEPERATOR,
	MENU_ITEM_DISABLED
};

static const uint32 kMsgUpdateModifier		= 'upmd';
static const uint32 kMsgApplyModifiers 		= 'apmd';
static const uint32 kMsgRevertModifiers		= 'rvmd';


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Modifier Keys window"


ModifierKeysWindow::ModifierKeysWindow()
	:
	BWindow(BRect(80, 50, 400, 260), B_TRANSLATE("Modifier Keys"),
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
		case kMsgUpdateModifier:
		{
			int32 menu = MENU_ITEM_CAPS_LOCK;
			int32 key = -1;

			for (; menu <= MENU_ITEM_COMMAND; menu++) {
				if (message->FindInt32(_KeyToString(menu), &key) == B_OK)
					break;
			}

			if (key == -1) {
				// No option was found, don't update
				return;
			}

			// Now 'menu' contains the menu we want to set and 'key' contains
			// the option we want to set it to.

			switch (menu) {
				case MENU_ITEM_CAPS_LOCK:
					fCurrentMap->caps_key = _KeyToKeyCode(key);
					fCapsLockMenu->ItemAt(key)->SetMarked(true);
					break;

				case MENU_ITEM_CONTROL:
					fCurrentMap->left_control_key
						= _KeyToKeyCode(key);
					if (key != MENU_ITEM_CAPS_LOCK) {
						fCurrentMap->right_control_key
							= _KeyToKeyCode(key, true);
					}

					fControlMenu->ItemAt(key)->SetMarked(true);
					break;

				case MENU_ITEM_OPTION:
					fCurrentMap->left_option_key
						= _KeyToKeyCode(key);
					if (key != MENU_ITEM_CAPS_LOCK) {
						fCurrentMap->right_option_key
							= _KeyToKeyCode(key, true);
					}

					fOptionMenu->ItemAt(key)->SetMarked(true);
					break;

				case MENU_ITEM_COMMAND:
					fCurrentMap->left_command_key
						= _KeyToKeyCode(key);
					if (key != MENU_ITEM_CAPS_LOCK) {
						fCurrentMap->right_command_key
							= _KeyToKeyCode(key, true);
					}

					fCommandMenu->ItemAt(key)->SetMarked(true);
					break;
			}

			fRevertButton->SetEnabled(memcmp(fCurrentMap, fSavedMap,
				sizeof(key_map)));
			break;
		}

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

			// Tell KeymapWindow to update the modifier keys 
			be_app->PostMessage(updateModifiers);

			// We are done here, close the window
			this->PostMessage(B_QUIT_REQUESTED);
			break;
		}

		// Revert button
		case kMsgRevertModifiers:
			memcpy(fCurrentMap, fSavedMap, sizeof(key_map));

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
	fCapsLockMenu = new BPopUpMenu(
		B_TRANSLATE(_KeyToString(MENU_ITEM_CAPS_LOCK)), true, true);

	for (int32 key = MENU_ITEM_CAPS_LOCK; key <= MENU_ITEM_DISABLED; key++) {
		if (key == MENU_ITEM_SEPERATOR) {
			BSeparatorItem* separator = new BSeparatorItem;
			fCapsLockMenu->AddItem(separator, key);
		} else {
			BMessage* message = new BMessage(kMsgUpdateModifier);
			message->AddInt32(_KeyToString(MENU_ITEM_CAPS_LOCK), key);

			BMenuItem* item = new BMenuItem(B_TRANSLATE(_KeyToString(key)),
				message);

			if (fCurrentMap->caps_key == _KeyToKeyCode(key))
				item->SetMarked(true);

			fCapsLockMenu->AddItem(item, key);
		}
	}

	return new BMenuField(NULL, fCapsLockMenu);
}


BMenuField*
ModifierKeysWindow::_CreateControlMenuField()
{
	fControlMenu = new BPopUpMenu(
		B_TRANSLATE(_KeyToString(MENU_ITEM_CONTROL)), true, true);

	for (int32 key = MENU_ITEM_CAPS_LOCK; key <= MENU_ITEM_COMMAND; key++) {
		BMessage* message = new BMessage(kMsgUpdateModifier);
		message->AddInt32(_KeyToString(MENU_ITEM_CONTROL), key);

		BMenuItem* item = new BMenuItem(B_TRANSLATE(_KeyToString(key)),
			message);

		if (fCurrentMap->left_control_key == _KeyToKeyCode(key)
			&& fCurrentMap->right_control_key == _KeyToKeyCode(key, true))
			item->SetMarked(true);

		fControlMenu->AddItem(item, key);
	}

	return new BMenuField(NULL, fControlMenu);
}


BMenuField*
ModifierKeysWindow::_CreateOptionMenuField()
{
	fOptionMenu = new BPopUpMenu(
		B_TRANSLATE(_KeyToString(MENU_ITEM_OPTION)), true, true);

	for (int32 key = MENU_ITEM_CAPS_LOCK; key <= MENU_ITEM_COMMAND; key++) {
		BMessage* message = new BMessage(kMsgUpdateModifier);
		message->AddInt32(_KeyToString(MENU_ITEM_OPTION), key);

		BMenuItem* item = new BMenuItem(B_TRANSLATE(_KeyToString(key)),
			message);

		if (fCurrentMap->left_option_key == _KeyToKeyCode(key)
			&& fCurrentMap->right_option_key == _KeyToKeyCode(key, true))
			item->SetMarked(true);

		fOptionMenu->AddItem(item, key);
	}

	return new BMenuField(NULL, fOptionMenu);
}


BMenuField*
ModifierKeysWindow::_CreateCommandMenuField()
{
	fCommandMenu = new BPopUpMenu(
		B_TRANSLATE(_KeyToString(MENU_ITEM_COMMAND)), true, true);

	for (int32 key = MENU_ITEM_CAPS_LOCK; key <= MENU_ITEM_COMMAND; key++) {
		BMessage* message = new BMessage(kMsgUpdateModifier);
		message->AddInt32(_KeyToString(MENU_ITEM_COMMAND), key);

		BMenuItem* item = new BMenuItem(B_TRANSLATE(_KeyToString(key)),
			message);

		if (fCurrentMap->left_command_key == _KeyToKeyCode(key)
			&& fCurrentMap->right_command_key == _KeyToKeyCode(key, true))
			item->SetMarked(true);

		fCommandMenu->AddItem(item, key);
	}

	return new BMenuField(NULL, fCommandMenu);
}


void
ModifierKeysWindow::_MarkMenuItems()
{
	for (int32 key = MENU_ITEM_CAPS_LOCK; key <= MENU_ITEM_COMMAND; key++) {
		if (fCurrentMap->caps_key == _KeyToKeyCode(key))
			fCapsLockMenu->ItemAt(key)->SetMarked(true);

		if (fCurrentMap->left_control_key == _KeyToKeyCode(key)
			&& fCurrentMap->right_control_key == _KeyToKeyCode(key, true))
			fControlMenu->ItemAt(key)->SetMarked(true);

		if (fCurrentMap->left_option_key == _KeyToKeyCode(key)
			&& fCurrentMap->right_option_key == _KeyToKeyCode(key, true))
			fOptionMenu->ItemAt(key)->SetMarked(true);

		if (fCurrentMap->left_command_key == _KeyToKeyCode(key)
			&& fCurrentMap->right_command_key == _KeyToKeyCode(key, true))
			fCommandMenu->ItemAt(key)->SetMarked(true);
	}

	// Check if caps lock is disabled
	if (fCurrentMap->caps_key == _KeyToKeyCode(MENU_ITEM_DISABLED))
		fCapsLockMenu->ItemAt(MENU_ITEM_DISABLED)->SetMarked(true);
}


const char*
ModifierKeysWindow::_KeyToString(int32 key)
{
	switch (key) {
		case MENU_ITEM_CAPS_LOCK:
			return "Caps Lock";
		case MENU_ITEM_CONTROL:
			return "Control";
		case MENU_ITEM_OPTION:
			return "Option";
		case MENU_ITEM_COMMAND:
			return "Command";
		case MENU_ITEM_DISABLED:
			return "Disabled";
	}

	return "";
}


uint32
ModifierKeysWindow::_KeyToKeyCode(int32 key, bool right = false)
{
	switch (key) {
		case MENU_ITEM_CAPS_LOCK:
			return 0x3b;
		case MENU_ITEM_CONTROL:
			if (right)
				return 0x60;
			return 0x5c;
		case MENU_ITEM_OPTION:
			if (right)
				return 0x67;
			return 0x66;
		case MENU_ITEM_COMMAND:
			if (right)
				return 0x5f;
			return 0x5d;
		case MENU_ITEM_DISABLED:
			return 0;
	}

	return 0;
}
