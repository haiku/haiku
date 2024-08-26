/*
 * Copyright 2011-2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 *		Jorge Acereda, jacereda@gmail.com
 */


#include "ModifierKeysWindow.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Resources.h>
#include <SeparatorView.h>
#include <Size.h>
#include <StringView.h>

#include "KeymapApplication.h"
#include "StatusMenuField.h"


enum {
	CAPS_KEY = 0x00000001,
	SHIFT_KEY = 0x00000002,
	CONTROL_KEY = 0x00000004,
	OPTION_KEY = 0x00000008,
	COMMAND_KEY = 0x00000010,
};

enum {
	MENU_ITEM_CAPS,
	MENU_ITEM_SHIFT,
	MENU_ITEM_CONTROL,
	MENU_ITEM_OPTION,
	MENU_ITEM_COMMAND,
	MENU_ITEM_SEPARATOR,
	MENU_ITEM_DISABLED,

	MENU_ITEM_FIRST = MENU_ITEM_CAPS,
	MENU_ITEM_LAST = MENU_ITEM_DISABLED
};


static const uint32 kMsgUpdateStatus = 'stat';
static const uint32 kMsgUpdateModifier = 'upmd';
static const uint32 kMsgApplyModifiers = 'apmd';
static const uint32 kMsgRevertModifiers = 'rvmd';

static const int32 kUnset = 0;
static const int32 kDisabled = -1;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Modifier keys window"


ModifierKeysWindow::ModifierKeysWindow()
	:
	BWindow(BRect(0, 0, 360, 220), B_TRANSLATE("Modifier keys"),
		B_FLOATING_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	get_key_map(&fCurrentMap, &fCurrentBuffer);
	get_key_map(&fSavedMap, &fSavedBuffer);

	BStringView* keyRole = new BStringView("key role",
		B_TRANSLATE_COMMENT("Role", "As in the role of a modifier key"));
	keyRole->SetAlignment(B_ALIGN_RIGHT);
	keyRole->SetFont(be_bold_font);

	BStringView* keyLabel = new BStringView("key label",
		B_TRANSLATE_COMMENT("Key", "As in a computer keyboard key"));
	keyLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	keyLabel->SetFont(be_bold_font);

	_CreateMenuField(&fCapsMenu, (BMenuField**)&fCapsField, MENU_ITEM_CAPS,
		B_TRANSLATE_COMMENT("Caps Lock:", "Caps Lock key role name"));
	_CreateMenuField(&fShiftMenu, (BMenuField**)&fShiftField, MENU_ITEM_SHIFT,
		B_TRANSLATE_COMMENT("Shift:", "Shift key role name"));
	_CreateMenuField(&fControlMenu, (BMenuField**)&fControlField, MENU_ITEM_CONTROL,
		B_TRANSLATE_COMMENT("Control:", "Control key role name"));
	_CreateMenuField(&fOptionMenu,(BMenuField**) &fOptionField, MENU_ITEM_OPTION,
		B_TRANSLATE_COMMENT("Option:", "Option key role name"));
	_CreateMenuField(&fCommandMenu, (BMenuField**)&fCommandField, MENU_ITEM_COMMAND,
		B_TRANSLATE_COMMENT("Command:", "Command key role name"));

	fCancelButton = new BButton("cancelButton", B_TRANSLATE("Cancel"),
		new BMessage(B_QUIT_REQUESTED));

	fRevertButton = new BButton("revertButton", B_TRANSLATE("Revert"),
		new BMessage(kMsgRevertModifiers));
	fRevertButton->SetEnabled(false);

	fOkButton = new BButton("okButton", B_TRANSLATE("Set modifier keys"),
		new BMessage(kMsgApplyModifiers));
	fOkButton->MakeDefault(true);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.AddGrid(B_USE_DEFAULT_SPACING, B_USE_SMALL_SPACING)
			.Add(keyRole, 0, 0)
			.Add(keyLabel, 1, 0)

			.Add(fCapsField->CreateLabelLayoutItem(), 0, 1)
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 1, 1)
				.Add(fCapsField->CreateMenuBarLayoutItem())
				.End()

			.Add(fShiftField->CreateLabelLayoutItem(), 0, 2)
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 1, 2)
				.Add(fShiftField->CreateMenuBarLayoutItem())
				.End()

			.Add(fControlField->CreateLabelLayoutItem(), 0, 3)
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 1, 3)
				.Add(fControlField->CreateMenuBarLayoutItem())
				.End()

			.Add(fOptionField->CreateLabelLayoutItem(), 0, 4)
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 1, 4)
				.Add(fOptionField->CreateMenuBarLayoutItem())
				.End()

			.Add(fCommandField->CreateLabelLayoutItem(), 0, 5)
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 1, 5)
				.Add(fCommandField->CreateMenuBarLayoutItem())
				.End()

			.End()
		.AddGlue()
		.AddGroup(B_HORIZONTAL)
			.Add(fRevertButton)
			.AddGlue()
			.Add(fCancelButton)
			.Add(fOkButton)
			.End()
		.SetInsets(B_USE_WINDOW_SPACING)
		.End();

	// mark menu items and update status icons
	_UpdateStatus();
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
			int32 menuitem = MENU_ITEM_FIRST;
			int32 key = kDisabled;

			for (; menuitem <= MENU_ITEM_LAST; menuitem++) {
				if (message->FindInt32(_KeyToString(menuitem), &key) == B_OK)
					break;
			}

			if (key == kDisabled)
				return;

			// menuitem contains the item we want to set
			// key contains the item we want to set it to.

			uint32 leftKey = _KeyToKeyCode(key);
			uint32 rightKey = _KeyToKeyCode(key, true);

			switch (menuitem) {
				case MENU_ITEM_CAPS:
					fCurrentMap->caps_key = leftKey;
					break;

				case MENU_ITEM_SHIFT:
					fCurrentMap->left_shift_key = leftKey;
					fCurrentMap->right_shift_key = rightKey;
					break;

				case MENU_ITEM_CONTROL:
					fCurrentMap->left_control_key = leftKey;
					fCurrentMap->right_control_key = rightKey;
					break;

				case MENU_ITEM_OPTION:
					fCurrentMap->left_option_key = leftKey;
					fCurrentMap->right_option_key = rightKey;
					break;

				case MENU_ITEM_COMMAND:
					fCurrentMap->left_command_key = leftKey;
					fCurrentMap->right_command_key = rightKey;
					break;
			}

			_UpdateStatus();

			// enable/disable revert button
			fRevertButton->SetEnabled(memcmp(fCurrentMap, fSavedMap, sizeof(key_map)));
			break;
		}

		// OK button
		case kMsgApplyModifiers:
		{
			// if duplicate modifiers are found, don't update
			if (_DuplicateKeys() != 0)
				break;

			BMessage* updateModifiers = new BMessage(kMsgUpdateModifierKeys);

			if (fCurrentMap->caps_key != fSavedMap->caps_key)
				updateModifiers->AddUInt32("caps_key", fCurrentMap->caps_key);

			if (fCurrentMap->left_shift_key != fSavedMap->left_shift_key)
				updateModifiers->AddUInt32("left_shift_key", fCurrentMap->left_shift_key);

			if (fCurrentMap->right_shift_key != fSavedMap->right_shift_key)
				updateModifiers->AddUInt32("right_shift_key", fCurrentMap->right_shift_key);

			if (fCurrentMap->left_control_key != fSavedMap->left_control_key)
				updateModifiers->AddUInt32("left_control_key", fCurrentMap->left_control_key);

			if (fCurrentMap->right_control_key != fSavedMap->right_control_key)
				updateModifiers->AddUInt32("right_control_key", fCurrentMap->right_control_key);

			if (fCurrentMap->left_option_key != fSavedMap->left_option_key)
				updateModifiers->AddUInt32("left_option_key", fCurrentMap->left_option_key);

			if (fCurrentMap->right_option_key != fSavedMap->right_option_key)
				updateModifiers->AddUInt32("right_option_key", fCurrentMap->right_option_key);

			if (fCurrentMap->left_command_key != fSavedMap->left_command_key)
				updateModifiers->AddUInt32("left_command_key", fCurrentMap->left_command_key);

			if (fCurrentMap->right_command_key != fSavedMap->right_command_key)
				updateModifiers->AddUInt32("right_command_key", fCurrentMap->right_command_key);

			// KeymapWindow updates the modifiers
			be_app->PostMessage(updateModifiers);

			// we are done here, close the window
			this->PostMessage(B_QUIT_REQUESTED);
			break;
		}

		// Revert button
		case kMsgRevertModifiers:
			memcpy(fCurrentMap, fSavedMap, sizeof(key_map));

			_UpdateStatus();

			fRevertButton->SetEnabled(false);
			break;

		default:
			BWindow::MessageReceived(message);
	}
}


//	#pragma mark - ModifierKeysWindow private methods


void
ModifierKeysWindow::_CreateMenuField(BPopUpMenu** _menu, BMenuField** _menuField, uint32 key,
	const char* label)
{
	const char* keyName = _KeyToString(key);
	const char* name = B_TRANSLATE_NOCOLLECT(keyName);
	BPopUpMenu* menu = new BPopUpMenu(name, true, true);

	for (int32 key = MENU_ITEM_FIRST; key <= MENU_ITEM_LAST; key++) {
		if (key == MENU_ITEM_SEPARATOR) {
			// add separator item
			BSeparatorItem* separator = new BSeparatorItem;
			menu->AddItem(separator, MENU_ITEM_SEPARATOR);
			continue;
		}

		BMessage* message = new BMessage(kMsgUpdateModifier);
		message->AddInt32(keyName, key);
		StatusMenuItem* item
			= new StatusMenuItem(B_TRANSLATE_NOCOLLECT(_KeyToString(key)), message);
		menu->AddItem(item, key);
	}

	menu->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_VERTICAL_UNSET));

	BMenuField* menuField = new StatusMenuField(label, menu);
	menuField->SetAlignment(B_ALIGN_RIGHT);

	*_menu = menu;
	*_menuField = menuField;
}


void
ModifierKeysWindow::_MarkMenuItems()
{
	_MarkMenuItem("caps lock", fCapsMenu, fCurrentMap->caps_key, fCurrentMap->caps_key);
		// mark but don't set unmatched for Caps Lock
	fShiftField->SetUnmatched(_MarkMenuItem("shift", fShiftMenu,
		fCurrentMap->left_shift_key, fCurrentMap->right_shift_key) == false);
	fControlField->SetUnmatched(_MarkMenuItem("control", fControlMenu,
		fCurrentMap->left_control_key, fCurrentMap->right_control_key) == false);
	fOptionField->SetUnmatched(_MarkMenuItem("option", fOptionMenu,
		fCurrentMap->left_option_key, fCurrentMap->right_option_key) == false);
	fCommandField->SetUnmatched(_MarkMenuItem("command", fCommandMenu,
		fCurrentMap->left_command_key, fCurrentMap->right_command_key) == false);
}


bool
ModifierKeysWindow::_MarkMenuItem(const char* role, BPopUpMenu* menu, uint32 left, uint32 right)
{
	for (int32 key = MENU_ITEM_FIRST; key <= MENU_ITEM_LAST; key++) {
		if (key == MENU_ITEM_SEPARATOR)
			continue;

		uint32 leftKey = _KeyToKeyCode(key);
		uint32 rightKey = _KeyToKeyCode(key, true);
		if (leftKey != rightKey && key == MENU_ITEM_CAPS) {
			// mark Caps Lock on Caps Lock role if either left or right is caps
			// otherwise mark Caps Lock if left side is caps
			uint32 capsKey = _KeyToKeyCode(MENU_ITEM_CAPS);
			if (strcmp(role, "caps lock") == 0 && (left == capsKey || right == capsKey))
				menu->ItemAt(key)->SetMarked(true);
			else if (left == capsKey)
				menu->ItemAt(key)->SetMarked(true);
		} else if (left == leftKey && right == rightKey)
			menu->ItemAt(key)->SetMarked(true);
	}

	return menu->FindMarked() != NULL;
}


// get the string for a modifier key
const char*
ModifierKeysWindow::_KeyToString(int32 key)
{
	switch (key) {
		case MENU_ITEM_CAPS:
			return B_TRANSLATE_COMMENT("Caps Lock key",
				"Label of key above Shift, usually Caps Lock");

		case MENU_ITEM_SHIFT:
			return B_TRANSLATE_COMMENT("Shift key",
				"Label of key above Ctrl, usually Shift");

		case MENU_ITEM_CONTROL:
			return B_TRANSLATE_COMMENT("Ctrl key",
				"Label of key farthest from the spacebar, usually Ctrl"
				"e.g. Strg for German keyboard");

		case MENU_ITEM_OPTION:
			return B_TRANSLATE_COMMENT("Win/Cmd key",
				"Label of the \"Windows\" key (PC)/Command key (Mac)");

		case MENU_ITEM_COMMAND:
			return B_TRANSLATE_COMMENT("Alt/Opt key",
				"Label of Alt key (PC)/Option key (Mac)");

		case MENU_ITEM_DISABLED:
			return B_TRANSLATE_COMMENT("Disabled", "Do nothing");
	}

	return B_EMPTY_STRING;
}


// get the keycode for a modifier key
int32
ModifierKeysWindow::_KeyToKeyCode(int32 key, bool right)
{
	switch (key) {
		case MENU_ITEM_CAPS:
			return 0x3b;

		case MENU_ITEM_SHIFT:
			if (right)
				return 0x56;
			return 0x4b;

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
			return kDisabled;
	}

	return kUnset;
}


// validate duplicate keys
void
ModifierKeysWindow::_ValidateDuplicateKeys()
{
	uint32 dupMask = _DuplicateKeys();
	_ValidateDuplicateKey(fCapsField, CAPS_KEY & dupMask);
	_ValidateDuplicateKey(fShiftField, SHIFT_KEY & dupMask);
	_ValidateDuplicateKey(fControlField, CONTROL_KEY & dupMask);
	_ValidateDuplicateKey(fOptionField, OPTION_KEY & dupMask);
	_ValidateDuplicateKey(fCommandField, COMMAND_KEY & dupMask);
	fOkButton->SetEnabled(dupMask == 0);
}


void
ModifierKeysWindow::_ValidateDuplicateKey(StatusMenuField* field, uint32 mask)
{
	if (mask != 0) // don't override if false
		field->SetDuplicate(true);
}


// return a mask marking which keys are duplicates of each other for
// validation.
uint32
ModifierKeysWindow::_DuplicateKeys()
{
	uint32 duplicateMask = 0;

	int32 testLeft, testRight, left, right;
	for (int32 testKey = MENU_ITEM_FIRST; testKey < MENU_ITEM_SEPARATOR; testKey++) {
		testLeft = kUnset;
		testRight = kUnset;

		switch (testKey) {
			case MENU_ITEM_CAPS:
				testLeft = fCurrentMap->caps_key;
				testRight = kDisabled;
				break;

			case MENU_ITEM_SHIFT:
				testLeft = fCurrentMap->left_shift_key;
				testRight = fCurrentMap->right_shift_key;
				break;

			case MENU_ITEM_CONTROL:
				testLeft = fCurrentMap->left_control_key;
				testRight = fCurrentMap->right_control_key;
				break;

			case MENU_ITEM_OPTION:
				testLeft = fCurrentMap->left_option_key;
				testRight = fCurrentMap->right_option_key;
				break;

			case MENU_ITEM_COMMAND:
				testLeft = fCurrentMap->left_command_key;
				testRight = fCurrentMap->right_command_key;
				break;
		}

		if (testLeft == kUnset && (testRight == kUnset || testRight == kDisabled))
			continue;

		for (int32 key = MENU_ITEM_FIRST; key < MENU_ITEM_SEPARATOR; key++) {
			if (key == testKey) {
				// skip over yourself
				continue;
			}

			left = kUnset;
			right = kUnset;

			switch (key) {
				case MENU_ITEM_CAPS:
					left = fCurrentMap->caps_key;
					right = kDisabled;
					break;

				case MENU_ITEM_SHIFT:
					left = fCurrentMap->left_shift_key;
					right = fCurrentMap->right_shift_key;
					break;

				case MENU_ITEM_CONTROL:
					left = fCurrentMap->left_control_key;
					right = fCurrentMap->right_control_key;
					break;

				case MENU_ITEM_OPTION:
					left = fCurrentMap->left_option_key;
					right = fCurrentMap->right_option_key;
					break;

				case MENU_ITEM_COMMAND:
					left = fCurrentMap->left_command_key;
					right = fCurrentMap->right_command_key;
					break;
			}

			if (left == kUnset && (right == kUnset || right == kDisabled))
				continue;

			// left or right is set

			if (left == testLeft || right == testRight) {
				duplicateMask |= 1 << testKey;
				duplicateMask |= 1 << key;
			}
		}
	}

	return duplicateMask;
}


void
ModifierKeysWindow::_UpdateStatus()
{
	// the order is important
	_MarkMenuItems();
	_ValidateDuplicateKeys();
}
