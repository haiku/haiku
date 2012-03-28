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
#include <FindDirectory.h>
#include <GroupLayout.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <IconUtils.h>
#include <Locale.h>
#include <LayoutBuilder.h>
#include <MenuItem.h>
#include <Message.h>
#include <Path.h>
#include <Resources.h>
#include <Size.h>

#include "KeymapApplication.h"


#ifdef DEBUG_ALERT
#	define FTRACE(x) fprintf(x)
#else
#	define FTRACE(x) /* nothing */
#endif


const rgb_color disabledColor = (rgb_color){128, 128, 128, 255};
const rgb_color normalColor = (rgb_color){0, 0, 0, 255};

enum {
	SHIFT_KEY = 0x00000001,
	CONTROL_KEY = 0x00000002,
	OPTION_KEY = 0x00000004,
	COMMAND_KEY = 0x00000008
};

enum {
	MENU_ITEM_SHIFT = 0,
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
#define B_TRANSLATE_CONTEXT "Modifier keys window"


ModifierKeysWindow::ModifierKeysWindow()
	:
	BWindow(BRect(0, 0, 360, 220), B_TRANSLATE("Modifier keys"),
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE
		| B_AUTO_UPDATE_SIZE_LIMITS)
{
	get_key_map(&fCurrentMap, &fCurrentBuffer);
	get_key_map(&fSavedMap, &fSavedBuffer);

	BStringView* keyRole = new BStringView("key role",
		B_TRANSLATE_COMMENT("Key role",
			"key roles, e.g. Control, Option, Command"));
	keyRole->SetAlignment(B_ALIGN_RIGHT);
	keyRole->SetFont(be_bold_font);

	BStringView* keyLabel = new BStringView("key label",
		B_TRANSLATE_COMMENT("Key", "A computer keyboard key"));
	keyLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	keyLabel->SetFont(be_bold_font);

	float width = 0.0;
	float widest = 0.0;

	fShiftStringView = new BStringView("shift",
		B_TRANSLATE_COMMENT("Shift:", "Shift key role name"));
	fShiftStringView->SetAlignment(B_ALIGN_RIGHT);
	width = fShiftStringView->StringWidth(fShiftStringView->Text());
	if (width > widest)
		widest = width;

	fControlStringView = new BStringView("control",
		B_TRANSLATE_COMMENT("Control:", "Control key role name"));
	fControlStringView->SetAlignment(B_ALIGN_RIGHT);
	width = fControlStringView->StringWidth(fControlStringView->Text());
	if (width > widest)
		widest = width;

	fOptionStringView = new BStringView("option",
		B_TRANSLATE_COMMENT("Option:", "Option key role name"));
	fOptionStringView->SetAlignment(B_ALIGN_RIGHT);
	width = fOptionStringView->StringWidth(fOptionStringView->Text());
	if (width > widest)
		widest = width;

	fCommandStringView = new BStringView("command",
		B_TRANSLATE_COMMENT("Command:", "Command key role name"));
	fCommandStringView->SetAlignment(B_ALIGN_RIGHT);
	width = fCommandStringView->StringWidth(fCommandStringView->Text());
	if (width > widest)
		widest = width;

	// set the width of each of the string view's to the widest
	fShiftStringView->SetExplicitMaxSize(BSize(widest, B_SIZE_UNSET));
	fControlStringView->SetExplicitMaxSize(BSize(widest, B_SIZE_UNSET));
	fOptionStringView->SetExplicitMaxSize(BSize(widest, B_SIZE_UNSET));
	fCommandStringView->SetExplicitMaxSize(BSize(widest, B_SIZE_UNSET));

	fShiftConflictView = new ConflictView("shift warning view");
	fShiftConflictView->SetExplicitMaxSize(BSize(15, 15));

	fControlConflictView = new ConflictView("control warning view");
	fControlConflictView->SetExplicitMaxSize(BSize(15, 15));

	fOptionConflictView = new ConflictView("option warning view");
	fOptionConflictView->SetExplicitMaxSize(BSize(15, 15));

	fCommandConflictView = new ConflictView("command warning view");
	fCommandConflictView->SetExplicitMaxSize(BSize(15, 15));

	fCancelButton = new BButton("cancelButton", B_TRANSLATE("Cancel"),
		new BMessage(B_QUIT_REQUESTED));

	fRevertButton = new BButton("revertButton", B_TRANSLATE("Revert"),
		new BMessage(kMsgRevertModifiers));
	fRevertButton->SetEnabled(false);

	fOkButton = new BButton("okButton", B_TRANSLATE("Set modifier keys"),
		new BMessage(kMsgApplyModifiers));
	fOkButton->MakeDefault(true);

	// Build the layout
	SetLayout(new BGroupLayout(B_VERTICAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(BGridLayoutBuilder(10, 10)
			.Add(keyRole, 0, 0)
			.Add(keyLabel, 1, 0)

			.Add(fShiftStringView, 0, 1)
			.Add(_CreateShiftMenuField(), 1, 1)
			.Add(fShiftConflictView, 2, 1)

			.Add(fControlStringView, 0, 2)
			.Add(_CreateControlMenuField(), 1, 2)
			.Add(fControlConflictView, 2, 2)

			.Add(fOptionStringView, 0, 3)
			.Add(_CreateOptionMenuField(), 1, 3)
			.Add(fOptionConflictView, 2, 3)

			.Add(fCommandStringView, 0, 4)
			.Add(_CreateCommandMenuField(), 1, 4)
			.Add(fCommandConflictView, 2, 4)
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
	_ValidateDuplicateKeys();

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
			int32 menuitem = MENU_ITEM_SHIFT;
			int32 key = -1;

			for (; menuitem <= MENU_ITEM_COMMAND; menuitem++) {
				if (message->FindInt32(_KeyToString(menuitem), &key) == B_OK)
					break;
			}

			if (key == -1)
				return;

			// menuitem contains the item we want to set
			// key contains the item we want to set it to.

			switch (menuitem) {
				case MENU_ITEM_SHIFT:
					fCurrentMap->left_shift_key = _KeyToKeyCode(key);
					fCurrentMap->right_shift_key = _KeyToKeyCode(key, true);
					break;

				case MENU_ITEM_CONTROL:
					fCurrentMap->left_control_key = _KeyToKeyCode(key);
					fCurrentMap->right_control_key = _KeyToKeyCode(key, true);
					break;

				case MENU_ITEM_OPTION:
					fCurrentMap->left_option_key = _KeyToKeyCode(key);
					fCurrentMap->right_option_key = _KeyToKeyCode(key, true);
					break;

				case MENU_ITEM_COMMAND:
					fCurrentMap->left_command_key = _KeyToKeyCode(key);
					fCurrentMap->right_command_key = _KeyToKeyCode(key, true);
					break;
			}

			_MarkMenuItems();
			_ValidateDuplicateKeys();

			// enable/disable revert button
			fRevertButton->SetEnabled(
				memcmp(fCurrentMap, fSavedMap, sizeof(key_map)));
			break;
		}

		// OK button
		case kMsgApplyModifiers:
		{
			// if duplicate modifiers are found, don't update
			if (_DuplicateKeys() != 0)
				break;

			BMessage* updateModifiers = new BMessage(kMsgUpdateModifierKeys);

			if (fCurrentMap->left_shift_key != fSavedMap->left_shift_key) {
				updateModifiers->AddUInt32("left_shift_key",
					fCurrentMap->left_shift_key);
			}

			if (fCurrentMap->right_shift_key != fSavedMap->right_shift_key) {
				updateModifiers->AddUInt32("right_shift_key",
					fCurrentMap->right_shift_key);
			}

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


//	#pragma mark -


BMenuField*
ModifierKeysWindow::_CreateShiftMenuField()
{
	fShiftMenu = new BPopUpMenu(
		B_TRANSLATE_NOCOLLECT(_KeyToString(MENU_ITEM_SHIFT)), true, true);

	for (int32 key = MENU_ITEM_SHIFT; key <= MENU_ITEM_DISABLED; key++) {
		if (key == MENU_ITEM_SEPERATOR) {
			// add separator item
			BSeparatorItem* separator = new BSeparatorItem;
			fShiftMenu->AddItem(separator, MENU_ITEM_SEPERATOR);
			continue;
		}

		BMessage* message = new BMessage(kMsgUpdateModifier);
		message->AddInt32(_KeyToString(MENU_ITEM_SHIFT), key);
		BMenuItem* item = new BMenuItem(
			B_TRANSLATE_NOCOLLECT(_KeyToString(key)), message);

		fShiftMenu->AddItem(item, key);
	}
	fShiftMenu->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
		B_ALIGN_VERTICAL_UNSET));

	return new BMenuField(NULL, fShiftMenu);
}


BMenuField*
ModifierKeysWindow::_CreateControlMenuField()
{
	fControlMenu = new BPopUpMenu(
		B_TRANSLATE_NOCOLLECT(_KeyToString(MENU_ITEM_CONTROL)), true, true);

	for (int32 key = MENU_ITEM_SHIFT; key <= MENU_ITEM_DISABLED; key++) {
		if (key == MENU_ITEM_SEPERATOR) {
			// add separator item
			BSeparatorItem* separator = new BSeparatorItem;
			fControlMenu->AddItem(separator, MENU_ITEM_SEPERATOR);
			continue;
		}

		BMessage* message = new BMessage(kMsgUpdateModifier);
		message->AddInt32(_KeyToString(MENU_ITEM_CONTROL), key);
		BMenuItem* item = new BMenuItem(
			B_TRANSLATE_NOCOLLECT(_KeyToString(key)), message);

		fControlMenu->AddItem(item, key);
	}
	fControlMenu->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
		B_ALIGN_VERTICAL_UNSET));

	return new BMenuField(NULL, fControlMenu);
}


BMenuField*
ModifierKeysWindow::_CreateOptionMenuField()
{
	fOptionMenu = new BPopUpMenu(
		B_TRANSLATE_NOCOLLECT(_KeyToString(MENU_ITEM_OPTION)), true, true);

	for (int32 key = MENU_ITEM_SHIFT; key <= MENU_ITEM_DISABLED; key++) {
		if (key == MENU_ITEM_SEPERATOR) {
			// add separator item
			BSeparatorItem* separator = new BSeparatorItem;
			fOptionMenu->AddItem(separator, MENU_ITEM_SEPERATOR);
			continue;
		}

		BMessage* message = new BMessage(kMsgUpdateModifier);
		message->AddInt32(_KeyToString(MENU_ITEM_OPTION), key);
		BMenuItem* item = new BMenuItem(
			B_TRANSLATE_NOCOLLECT(_KeyToString(key)), message);

		fOptionMenu->AddItem(item, key);
	}
	fOptionMenu->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
		B_ALIGN_VERTICAL_UNSET));

	return new BMenuField(NULL, fOptionMenu);
}


BMenuField*
ModifierKeysWindow::_CreateCommandMenuField()
{
	fCommandMenu = new BPopUpMenu(
		B_TRANSLATE_NOCOLLECT(_KeyToString(MENU_ITEM_COMMAND)), true, true);

	for (int32 key = MENU_ITEM_SHIFT; key <= MENU_ITEM_DISABLED; key++) {
		if (key == MENU_ITEM_SEPERATOR) {
			// add separator item
			BSeparatorItem* separator = new BSeparatorItem;
			fCommandMenu->AddItem(separator, MENU_ITEM_SEPERATOR);
			continue;
		}

		BMessage* message = new BMessage(kMsgUpdateModifier);
		message->AddInt32(_KeyToString(MENU_ITEM_COMMAND), key);
		BMenuItem* item = new BMenuItem(
			B_TRANSLATE_NOCOLLECT(_KeyToString(key)), message);
		fCommandMenu->AddItem(item, key);
	}
	fCommandMenu->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
		B_ALIGN_VERTICAL_UNSET));

	return new BMenuField(NULL, fCommandMenu);
}


void
ModifierKeysWindow::_MarkMenuItems()
{
	for (int32 key = MENU_ITEM_SHIFT; key <= MENU_ITEM_DISABLED; key++) {
		if (key == MENU_ITEM_SEPERATOR)
			continue;

		if (fCurrentMap->left_shift_key == _KeyToKeyCode(key)
			&& fCurrentMap->right_shift_key == _KeyToKeyCode(key, true)) {
			fShiftMenu->ItemAt(key)->SetMarked(true);

			if (key == MENU_ITEM_DISABLED)
				fShiftStringView->SetHighColor(disabledColor);
			else
				fShiftStringView->SetHighColor(normalColor);

			fShiftStringView->Invalidate();
		}

		if (fCurrentMap->left_control_key == _KeyToKeyCode(key)
			&& fCurrentMap->right_control_key == _KeyToKeyCode(key, true)) {
			fControlMenu->ItemAt(key)->SetMarked(true);

			if (key == MENU_ITEM_DISABLED)
				fControlStringView->SetHighColor(disabledColor);
			else
				fControlStringView->SetHighColor(normalColor);

			fControlStringView->Invalidate();
		}

		if (fCurrentMap->left_option_key == _KeyToKeyCode(key)
			&& fCurrentMap->right_option_key == _KeyToKeyCode(key, true)) {
			fOptionMenu->ItemAt(key)->SetMarked(true);

			if (key == MENU_ITEM_DISABLED)
				fOptionStringView->SetHighColor(disabledColor);
			else
				fOptionStringView->SetHighColor(normalColor);

			fOptionStringView->Invalidate();
		}

		if (fCurrentMap->left_command_key == _KeyToKeyCode(key)
			&& fCurrentMap->right_command_key == _KeyToKeyCode(key, true)) {
			fCommandMenu->ItemAt(key)->SetMarked(true);

			if (key == MENU_ITEM_DISABLED)
				fCommandStringView->SetHighColor(disabledColor);
			else
				fCommandStringView->SetHighColor(normalColor);

			fCommandStringView->Invalidate();
		}
	}
}


// get the string for a modifier key
const char*
ModifierKeysWindow::_KeyToString(int32 key)
{
	switch (key) {
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

	BBitmap* shiftIcon = fShiftConflictView->Icon();
	BBitmap* controlIcon = fControlConflictView->Icon();
	BBitmap* optionIcon = fOptionConflictView->Icon();
	BBitmap* commandIcon = fCommandConflictView->Icon();

	if (dupMask != 0) {
		fShiftConflictView->ShowIcon((dupMask & SHIFT_KEY) != 0);
		fControlConflictView->ShowIcon((dupMask & CONTROL_KEY) != 0);
		fOptionConflictView->ShowIcon((dupMask & OPTION_KEY) != 0);
		fCommandConflictView->ShowIcon((dupMask & COMMAND_KEY) != 0);

		fOkButton->SetEnabled(false);
	} else {
		fShiftConflictView->ShowIcon(false);
		fControlConflictView->ShowIcon(false);
		fOptionConflictView->ShowIcon(false);
		fCommandConflictView->ShowIcon(false);

		fOkButton->SetEnabled(true);
	}

	// if there was a change invalidate the view
	if (shiftIcon != fShiftConflictView->Icon())
		fShiftConflictView->Invalidate();

	if (controlIcon != fControlConflictView->Icon())
		fControlConflictView->Invalidate();

	if (optionIcon != fOptionConflictView->Icon())
		fOptionConflictView->Invalidate();

	if (commandIcon != fCommandConflictView->Icon())
		fCommandConflictView->Invalidate();
}


// return a mask marking which keys are duplicates of each other for
// validation. Control = 1, Option = 2, Command = 3
uint32
ModifierKeysWindow::_DuplicateKeys()
{
	uint32 duplicateMask = 0;

	for (int32 testKey = MENU_ITEM_SHIFT; testKey <= MENU_ITEM_COMMAND;
		testKey++) {
		uint32 testLeft = 0;
		uint32 testRight = 0;

		switch (testKey) {
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

		for (int32 key = MENU_ITEM_SHIFT; key <= MENU_ITEM_COMMAND; key++) {
			if (key == testKey) {
				// skip over yourself
				continue;
			}

			uint32 left = 0;
			uint32 right = 0;

			switch(key) {
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

			if (left == testLeft && right == testRight) {
				duplicateMask |= 1 << testKey;
				duplicateMask |= 1 << key;
			}
		}
	}

	return duplicateMask;
}


//	#pragma mark - ConflictView


ConflictView::ConflictView(const char* name)
	:
	BView(BRect(0, 0, 15, 15), name, B_FOLLOW_NONE, B_WILL_DRAW),
	fIcon(NULL),
	fSavedIcon(NULL)
{
	_FillSavedIcon();
}


ConflictView::~ConflictView()
{
	delete fSavedIcon;
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

	// Draw icon
	if (fIcon == NULL)
		return;

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


// show or hide the icon
void
ConflictView::ShowIcon(bool show)
{
	if (show)
		fIcon = fSavedIcon;
	else
		fIcon = NULL;
}


//	#pragma mark - 


// fill out the icon with the stop symbol from app_server
void
ConflictView::_FillSavedIcon()
{
	// return if the fSavedIcon has already been filled out
	if (fSavedIcon != NULL && fSavedIcon->InitCheck() == B_OK)
		return;

	BPath path;
	status_t status = find_directory(B_BEOS_SERVERS_DIRECTORY, &path);
	if (status < B_OK) {
		FTRACE((stderr,
			"_FillWarningIcon() - find_directory failed: %s\n",
			strerror(status)));
		delete fSavedIcon;
		fSavedIcon = NULL;
		return;
	}

	path.Append("app_server");
	BFile file;
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status < B_OK) {
		FTRACE((stderr,
			"_FillWarningIcon() - BFile init failed: %s\n",
			strerror(status)));
		delete fSavedIcon;
		fSavedIcon = NULL;
		return;
	}

	BResources resources;
	status = resources.SetTo(&file);
	if (status < B_OK) {
		FTRACE((stderr,
			"_WarningIcon() - BResources init failed: %s\n",
			strerror(status)));
		delete fSavedIcon;
		fSavedIcon = NULL;
		return;
	}

	// Allocate the fSavedIcon bitmap
	fSavedIcon = new(std::nothrow) BBitmap(BRect(0, 0, 15, 15), 0, B_RGBA32);
	if (fSavedIcon->InitCheck() < B_OK) {
		FTRACE((stderr, "_WarningIcon() - No memory for warning bitmap\n"));
		delete fSavedIcon;
		fSavedIcon = NULL;
		return;
	}

	// Load the raw stop icon data
	size_t size = 0;
	const uint8* rawIcon;
	rawIcon = (const uint8*)resources.LoadResource(B_VECTOR_ICON_TYPE,
		"stop", &size);

	// load vector warning icon into fSavedIcon
	if (rawIcon == NULL
		|| BIconUtils::GetVectorIcon(rawIcon, size, fSavedIcon) < B_OK) {
			delete fSavedIcon;
			fSavedIcon = NULL;
	}
}
