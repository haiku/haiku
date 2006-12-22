/*
 * Copyright 2002-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		<unknown, please fill in who knows>
 *		Vasilis Kaoutsis, kaoutsis@sch.gr
 */


#include "BitmapMenuItem.h"
#include "FontMenu.h"
#include "MenuBar.h"
#include "MenuSettings.h"
#include "msg.h"

#include <Application.h>
#include <Resources.h>
#include <TranslationUtils.h>	
#include <Window.h>

#include <stdio.h>
#include <stdlib.h>


MenuBar::MenuBar()
	: BMenuBar(BRect(40, 10, 10, 10), "menu", B_FOLLOW_TOP | B_FRAME_EVENTS,
		B_ITEMS_IN_COLUMN, true)
{
	_BuildMenu();
	UpdateMenu();
	SetItemMargins(14.0, 2.0, 20.0, 0.0);
}


void
MenuBar::AttachedToWindow()
{
	BMenuBar::AttachedToWindow();
	SetTargetForItems(Window());
}


void
MenuBar::_BuildMenu()
{
	// font and font size menus
	BMenu* fontMenu = new FontMenu();
	BMenu* fontSizeMenu = new FontSizeMenu();

	// create the menu items
	fAlwaysShowTriggersItem = new BMenuItem("Always Show Triggers",
		new BMessage(ALLWAYS_TRIGGERS_MSG), 0, 0);
	fControlAsShortcutItem = new BitmapMenuItem("as Shortcut Key",
		new BMessage(CTL_MARKED_MSG), BTranslationUtils::GetBitmap(B_RAW_TYPE, "CTL"));
	fAltAsShortcutItem = new BitmapMenuItem("as Shortcut Key",
		new BMessage(ALT_MARKED_MSG), BTranslationUtils::GetBitmap(B_RAW_TYPE, "ALT"));

	// color menu
	BMenuItem* colorSchemeItem = new BMenuItem("Color Scheme...",
		new BMessage(COLOR_SCHEME_OPEN_MSG), 0, 0);

	// Add items to menubar	
	AddItem(fontMenu, 0);
	AddItem(fontSizeMenu, 1);

	AddSeparatorItem();

	AddItem(colorSchemeItem);

	AddSeparatorItem();

	AddItem(fAlwaysShowTriggersItem);

	AddSeparatorItem();

	AddItem(fControlAsShortcutItem);
	AddItem(fAltAsShortcutItem);
}


void
MenuBar::UpdateMenu()
{
	menu_info info;
	MenuSettings::GetInstance()->Get(info);

	fAlwaysShowTriggersItem->SetMarked(info.triggers_always_shown);

	key_map *keys; 
	char* chars; 
	get_key_map(&keys, &chars); 

	bool altAsShortcut = (keys->left_command_key == 0x5d) 
		&& (keys->right_command_key == 0x5f); 

	free(chars); 
	free(keys);

	fAltAsShortcutItem->SetMarked(altAsShortcut); 
	fControlAsShortcutItem->SetMarked(!altAsShortcut); 
}


void
MenuBar::Update()
{
	UpdateMenu();

	BFont font;
	if (LockLooper()) {
		menu_info info;
		MenuSettings::GetInstance()->Get(info);

		font.SetFamilyAndStyle(info.f_family, info.f_style);
		font.SetSize(info.font_size);
		SetFont(&font);
		SetViewColor(info.background_color);
	
		// force the menu to redraw
		InvalidateLayout();
		Invalidate();
		UnlockLooper();
	}
}


void
MenuBar::FrameResized(float width, float height)
{
	Window()->ResizeTo(width + 80, height + 55);
	Window()->PostMessage(UPDATE_WINDOW);
	BMenuBar::FrameResized(width, height);
}
