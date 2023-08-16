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

#include <Bitmap.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <FindDirectory.h>
#include <IconUtils.h>
#include <InterfaceDefs.h>
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


#ifdef DEBUG_ALERT
#	define FTRACE(x) fprintf(x)
#else
#	define FTRACE(x) /* nothing */
#endif


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


static const uint32 kMsgUpdateModifier = 'upmd';
static const uint32 kMsgApplyModifiers = 'apmd';
static const uint32 kMsgRevertModifiers = 'rvmd';


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Modifier keys window"


//	#pragma mark - ConflictView


ConflictView::ConflictView(const char* name)
	:
	BView(BRect(BPoint(0, 0), be_control_look->ComposeIconSize(B_MINI_ICON)),
		name, B_FOLLOW_NONE, B_WILL_DRAW),
	fIcon(NULL),
	fStopIcon(NULL),
	fWarnIcon(NULL)
{
	_FillIcons();
}


ConflictView::~ConflictView()
{
	delete fStopIcon;
	delete fWarnIcon;
}


void
ConflictView::Draw(BRect updateRect)
{
	// Draw background
	if (Parent())
		SetLowColor(Parent()->ViewColor());
	else
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	FillRect(updateRect, B_SOLID_LOW);

	if (fIcon == NULL)
		return;

	// Draw icon
	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	DrawBitmapAsync(fIcon, BPoint(0, 0));
}


// get the icon
BBitmap*
ConflictView::Icon()
{
	return fIcon;
}


// show or hide the stop icon
void
ConflictView::SetStopIcon(bool show)
{
	fIcon = show ? fStopIcon : NULL;
	const char* tip = show ? B_TRANSLATE("Error: duplicate keys") : NULL;
	SetToolTip(tip);
}


// show or hide the warn icon
void
ConflictView::SetWarnIcon(bool show)
{
	fIcon = show ? fWarnIcon : NULL;
	const char* tip = show ? B_TRANSLATE("Warning: left and right key roles do not match") : NULL;
	SetToolTip(tip);
}


//	#pragma mark - ConflictView private methods


// fill out the icons with the stop and warn symbols from app_server
void
ConflictView::_FillIcons()
{
	if (fStopIcon == NULL) {
		// Allocate the fStopIcon bitmap
		fStopIcon = new (std::nothrow) BBitmap(Bounds(), 0, B_RGBA32);
		if (fStopIcon->InitCheck() != B_OK) {
			FTRACE((stderr, "_FillIcons() - No memory for stop bitmap\n"));
			delete fStopIcon;
			fStopIcon = NULL;
			return;
		}

		// load dialog-error icon bitmap
		if (BIconUtils::GetSystemIcon("dialog-error", fStopIcon) != B_OK) {
			delete fStopIcon;
			fStopIcon = NULL;
			return;
		}
	}

	if (fWarnIcon == NULL) {
		// Allocate the fWarnIcon bitmap
		fWarnIcon = new (std::nothrow) BBitmap(Bounds(), 0, B_RGBA32);
		if (fWarnIcon->InitCheck() != B_OK) {
			FTRACE((stderr, "_FillIcons() - No memory for warn bitmap\n"));
			delete fWarnIcon;
			fWarnIcon = NULL;
			return;
		}

		// load dialog-warning icon bitmap
		if (BIconUtils::GetSystemIcon("dialog-warning", fWarnIcon) != B_OK) {
			delete fWarnIcon;
			fWarnIcon = NULL;
			return;
		}
	}
}


//	#pragma mark - ModifierKeysWindow


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

	BMenuField* capsMenuField;
	_CreateMenuField(&fCapsMenu, &capsMenuField, MENU_ITEM_CAPS,
		B_TRANSLATE_COMMENT("Caps:", "Caps key role name"));

	BMenuField* shiftMenuField;
	_CreateMenuField(&fShiftMenu, &shiftMenuField, MENU_ITEM_SHIFT,
		B_TRANSLATE_COMMENT("Shift:", "Shift key role name"));

	BMenuField* controlMenuField;
	_CreateMenuField(&fControlMenu, &controlMenuField, MENU_ITEM_CONTROL,
		B_TRANSLATE_COMMENT("Control:", "Control key role name"));

	BMenuField* optionMenuField;
	_CreateMenuField(&fOptionMenu, &optionMenuField, MENU_ITEM_OPTION,
		B_TRANSLATE_COMMENT("Option:", "Option key role name"));

	BMenuField* commandMenuField;
	_CreateMenuField(&fCommandMenu, &commandMenuField, MENU_ITEM_COMMAND,
		B_TRANSLATE_COMMENT("Command:", "Command key role name"));

	fCapsConflictView = new ConflictView("caps lock warning view");
	fCapsConflictView->SetExplicitMaxSize(fCapsConflictView->Bounds().Size());

	fShiftConflictView = new ConflictView("shift warning view");
	fShiftConflictView->SetExplicitMaxSize(fShiftConflictView->Bounds().Size());

	fControlConflictView = new ConflictView("control warning view");
	fControlConflictView->SetExplicitMaxSize(fControlConflictView->Bounds().Size());

	fOptionConflictView = new ConflictView("option warning view");
	fOptionConflictView->SetExplicitMaxSize(fOptionConflictView->Bounds().Size());

	fCommandConflictView = new ConflictView("command warning view");
	fCommandConflictView->SetExplicitMaxSize(fCommandConflictView->Bounds().Size());

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

			.Add(capsMenuField->CreateLabelLayoutItem(), 0, 1)
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 1, 1)
				.Add(capsMenuField->CreateMenuBarLayoutItem())
				.Add(fCapsConflictView)
				.End()

			.Add(shiftMenuField->CreateLabelLayoutItem(), 0, 2)
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 1, 2)
				.Add(shiftMenuField->CreateMenuBarLayoutItem())
				.Add(fShiftConflictView)
				.End()

			.Add(controlMenuField->CreateLabelLayoutItem(), 0, 3)
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 1, 3)
				.Add(controlMenuField->CreateMenuBarLayoutItem())
				.Add(fControlConflictView)
				.End()

			.Add(optionMenuField->CreateLabelLayoutItem(), 0, 4)
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 1, 4)
				.Add(optionMenuField->CreateMenuBarLayoutItem())
				.Add(fOptionConflictView)
				.End()

			.Add(commandMenuField->CreateLabelLayoutItem(), 0, 5)
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 1, 5)
				.Add(commandMenuField->CreateMenuBarLayoutItem())
				.Add(fCommandConflictView)
				.End()

			.End()
		.AddGlue()
		.AddGroup(B_HORIZONTAL)
			.Add(fCancelButton)
			.AddGlue()
			.Add(fRevertButton)
			.Add(fOkButton)
			.End()
		.SetInsets(B_USE_WINDOW_SPACING)
		.End();

	_MarkMenuItems();
	_ValidateDuplicateKeys();
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
			int32 key = -1;

			for (; menuitem <= MENU_ITEM_LAST; menuitem++) {
				if (message->FindInt32(_KeyToString(menuitem), &key) == B_OK)
					break;
			}

			if (key == -1)
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

			_MarkMenuItems();
			_ValidateDuplicateKeys();

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

			if (fCurrentMap->caps_key != fSavedMap->caps_key)
				updateModifiers->AddUInt32("caps_key", fCurrentMap->caps_key);

			// KeymapWindow updates the modifiers
			be_app->PostMessage(updateModifiers);

			// we are done here, close the window
			this->PostMessage(B_QUIT_REQUESTED);
			break;
		}

		// Revert button
		case kMsgRevertModifiers:
			memcpy(fCurrentMap, fSavedMap, sizeof(key_map));

			_MarkMenuItems();
			_ValidateDuplicateKeys();

			fRevertButton->SetEnabled(false);
			break;

		default:
			BWindow::MessageReceived(message);
	}
}


//	#pragma mark - ModifierKeysWindow private methods

void
ModifierKeysWindow::_CreateMenuField(
	BPopUpMenu** outMenu, BMenuField** outField, uint32 inKey, const char* comment)
{
	const char* sKey = _KeyToString(inKey);
	const char* tKey = B_TRANSLATE_NOCOLLECT(sKey);
	BPopUpMenu* menu = new BPopUpMenu(tKey, true, true);

	for (int32 key = MENU_ITEM_FIRST; key <= MENU_ITEM_LAST; key++) {
		if (key == MENU_ITEM_SEPARATOR) {
			// add separator item
			BSeparatorItem* separator = new BSeparatorItem;
			menu->AddItem(separator, MENU_ITEM_SEPARATOR);
			continue;
		}

		BMessage* message = new BMessage(kMsgUpdateModifier);
		message->AddInt32(sKey, key);
		BMenuItem* item = new BMenuItem(B_TRANSLATE_NOCOLLECT(_KeyToString(key)), message);
		menu->AddItem(item, key);
	}

	menu->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_VERTICAL_UNSET));

	BMenuField* menuField = new BMenuField(comment, menu);
	menuField->SetAlignment(B_ALIGN_RIGHT);

	*outMenu = menu;
	*outField = menuField;
}


void
ModifierKeysWindow::_MarkMenuItems()
{
	_MarkMenuItem(fShiftMenu, fShiftConflictView,
		fCurrentMap->left_shift_key, fCurrentMap->right_shift_key);
	_MarkMenuItem(fControlMenu, fControlConflictView,
		fCurrentMap->left_control_key, fCurrentMap->right_control_key);
	_MarkMenuItem(fOptionMenu, fOptionConflictView,
		fCurrentMap->left_option_key, fCurrentMap->right_option_key);
	_MarkMenuItem(fCommandMenu, fCommandConflictView,
		fCurrentMap->left_command_key, fCurrentMap->right_command_key);
	_MarkMenuItem(fCapsMenu, fCapsConflictView,
		fCurrentMap->caps_key, fCurrentMap->caps_key);
}


void
ModifierKeysWindow::_MarkMenuItem(BPopUpMenu* menu, ConflictView* conflictView,
	uint32 leftKey, uint32 rightKey)
{
	for (int32 key = MENU_ITEM_FIRST; key <= MENU_ITEM_LAST; key++) {
		if (key == MENU_ITEM_SEPARATOR)
			continue;

		if (leftKey == _KeyToKeyCode(key) && rightKey == _KeyToKeyCode(key, true))
			menu->ItemAt(key)->SetMarked(true);
	}

	// Set the warning icon if not marked
	BBitmap* icon = conflictView->Icon();

	conflictView->SetWarnIcon(menu->FindMarked() == NULL);

	// if there was a change invalidate the view
	if (icon != conflictView->Icon())
		conflictView->Invalidate();
}


// get the string for a modifier key
const char*
ModifierKeysWindow::_KeyToString(int32 key)
{
	switch (key) {
		case MENU_ITEM_CAPS:
			return B_TRANSLATE_COMMENT("Caps key",
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

	return "";
}


// get the keycode for a modifier key
uint32
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
			return 0;
	}

	return 0;
}


// validate duplicate keys
void
ModifierKeysWindow::_ValidateDuplicateKeys()
{
	uint32 dupMask = _DuplicateKeys();
	_ValidateDuplicateKey(fCapsConflictView, CAPS_KEY & dupMask);
	_ValidateDuplicateKey(fShiftConflictView, SHIFT_KEY & dupMask);
	_ValidateDuplicateKey(fControlConflictView, CONTROL_KEY & dupMask);
	_ValidateDuplicateKey(fOptionConflictView, OPTION_KEY & dupMask);
	_ValidateDuplicateKey(fCommandConflictView, COMMAND_KEY & dupMask);
	fOkButton->SetEnabled(dupMask == 0);
}


void
ModifierKeysWindow::_ValidateDuplicateKey(ConflictView* view, uint32 mask)
{
	BBitmap* icon = view->Icon();
	view->SetStopIcon(mask != 0);
	// if there was a change invalidate the view
	if (icon != view->Icon())
		view->Invalidate();
}


// return a mask marking which keys are duplicates of each other for
// validation.
uint32
ModifierKeysWindow::_DuplicateKeys()
{
	uint32 duplicateMask = 0;

	for (int32 testKey = MENU_ITEM_FIRST; testKey <= MENU_ITEM_LAST; testKey++) {
		uint32 testLeft = 0;
		uint32 testRight = 0;

		switch (testKey) {
			case MENU_ITEM_CAPS:
				testLeft = fCurrentMap->caps_key;
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

		if (testLeft == 0 && testRight == 0)
			continue;

		for (int32 key = MENU_ITEM_FIRST; key <= MENU_ITEM_LAST; key++) {
			if (key == testKey) {
				// skip over yourself
				continue;
			}

			uint32 left = 0;
			uint32 right = 0;

			switch (key) {
				case MENU_ITEM_CAPS:
					left = fCurrentMap->caps_key;
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

			if (left == 0 && right == 0)
				continue;

			if (left == testLeft || right == testRight) {
				duplicateMask |= 1 << testKey;
				duplicateMask |= 1 << key;
			}
		}
	}

	return duplicateMask;
}
