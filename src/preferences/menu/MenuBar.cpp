/*
 * Copyright 2002-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		<unknown, please fill in who knows>
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
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


MenuBar::MenuBar()
	: BMenuBar(BRect(40, 10, 10, 10), "menu", 0,
		B_ITEMS_IN_COLUMN, true)
{
	_BuildMenu();	
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
	BMenuItem* colorSchemeItem = new BMenuItem("Color Scheme" B_UTF8_ELLIPSIS,
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
MenuBar::Update()
{
	menu_info info;
	MenuSettings::GetInstance()->Get(info);

	fAlwaysShowTriggersItem->SetMarked(info.triggers_always_shown);

	bool altAsShortcut = MenuSettings::GetInstance()->AltAsShortcut();
	fAltAsShortcutItem->SetMarked(altAsShortcut); 
	fControlAsShortcutItem->SetMarked(!altAsShortcut); 

	BFont font;
	if (LockLooper()) {
		font.SetFamilyAndStyle(info.f_family, info.f_style);
		font.SetSize(info.font_size);
		SetFont(&font);
		SetViewColor(info.background_color);
		SetLowColor(ViewColor());	

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
