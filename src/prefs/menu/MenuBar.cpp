	#include "MenuApp.h"
	#include <stdlib.h>
	#include <Resources.h>
	#include <Application.h>
	#include <stdio.h>
	
	MenuBar::MenuBar()
		:BMenuBar(BRect(40,10,10,10), "menu", B_FOLLOW_TOP|B_FRAME_EVENTS, B_ITEMS_IN_COLUMN, true)
	{
		fCtlBmp = BTranslationUtils::GetBitmap(B_RAW_TYPE, "CTL");
		fAltBmp = BTranslationUtils::GetBitmap(B_RAW_TYPE, "ALT");
		fSep0Bmp = BTranslationUtils::GetBitmap(B_RAW_TYPE, "SEP0");
		fSep1Bmp = BTranslationUtils::GetBitmap(B_RAW_TYPE, "SEP1");
		fSep2Bmp = BTranslationUtils::GetBitmap(B_RAW_TYPE, "SEP2");
		
		
		get_menu_info(&info);
		build_menu();
		set_menu();
	}
	
	MenuBar::~MenuBar()
	{ /*nothing to clean up*/}
	
	void
	MenuBar::build_menu()
	{
		// font and font size menus
		fontMenu = new FontMenu();
		fontSizeMenu = new FontSizeMenu();
		
		// create the menu items
		clickToOpenItem = new BMenuItem("Click To Open", new BMessage(CLICK_OPEN_MSG), 0, 0);
		alwaysShowTriggersItem = new BMenuItem("Always Show Triggers", new BMessage(ALLWAYS_TRIGGERS_MSG), 0, 0);
		separatorStyleItem = new BMenuItem("Separator Style", new BMessage(DEFAULT_MSG), 0, 0);
		ctlAsShortcutItem = new BitmapMenuItem("as Shortcut Key", new BMessage(CTL_MARKED_MSG), fCtlBmp);
		altAsShortcutItem = new BitmapMenuItem("as Shortcut Key", new BMessage(ALT_MARKED_MSG), fAltBmp);
		
		// color menu
		colorSchemeItem = new BMenuItem("Color Scheme...", new BMessage(COLOR_SCHEME_MSG), 0, 0);
	
		// create the separator menu
		separatorStyleMenu = new BMenu("Separator Style", B_ITEMS_IN_COLUMN);
		separatorStyleMenu->SetRadioMode(true);
		BMessage *msg = new BMessage(MENU_SEP_TYPE);
		msg->AddInt32("sep", 0);
		separatorStyleZero = new BitmapMenuItem("                 ", msg, fSep0Bmp);
		msg = new BMessage(MENU_SEP_TYPE);
		msg->AddInt32("sep", 1);
		separatorStyleOne = new BitmapMenuItem("", msg, fSep1Bmp);
		msg = new BMessage(MENU_SEP_TYPE);
		msg->AddInt32("sep", 2);
		separatorStyleTwo = new BitmapMenuItem("", msg, fSep2Bmp);
		if (info.separator == 0)
			separatorStyleZero->SetMarked(true);
		if (info.separator == 1)
			separatorStyleOne->SetMarked(true);
		if (info.separator == 2)
			separatorStyleTwo->SetMarked(true);
		separatorStyleMenu->AddItem(separatorStyleZero);
		separatorStyleMenu->AddItem(separatorStyleOne);
		separatorStyleMenu->AddItem(separatorStyleTwo);
		separatorStyleMenu->SetTargetForItems(Window());
	
		// Add items to menubar	
		AddItem(fontMenu, 0);
		AddItem(fontSizeMenu, 1);
		AddSeparatorItem();
		AddItem(clickToOpenItem);
		AddItem(alwaysShowTriggersItem);
		AddSeparatorItem();
		AddItem(colorSchemeItem);
		AddItem(separatorStyleMenu);
		AddSeparatorItem();
		AddItem(ctlAsShortcutItem);
		AddItem(altAsShortcutItem);
		SetTargetForItems(Window());
	}
	
	void
	MenuBar::set_menu()
	{
		key_map *keys; 
        char *chars; 
        bool altAsShortcut;
		
		// get up-to-date menu info
		get_menu_info(&info);
		
		alwaysShowTriggersItem->SetMarked(info.triggers_always_shown);

		clickToOpenItem->SetMarked(info.click_to_open);
		alwaysShowTriggersItem->SetEnabled(!info.click_to_open);
		
		get_key_map(&keys, &chars); 
        
        altAsShortcut = (keys->left_command_key == 0x5d) && (keys->right_command_key == 0x5f); 
        altAsShortcutItem->SetMarked(altAsShortcut); 
        ctlAsShortcutItem->SetMarked(!altAsShortcut); 
                
        free(chars); 
        free(keys);
	}
	
	void
	MenuBar::Update()
	{
		// get up-to-date menu info
		get_menu_info(&info);

		// update submenus
		fontMenu->Update();
		fontSizeMenu->Update();

		// this needs to be updated in case the Defaults
		// were requested.
		if (info.separator == 0)
			separatorStyleZero->SetMarked(true);
		else if (info.separator == 1)
			separatorStyleOne->SetMarked(true);
		else if (info.separator == 2)
			separatorStyleTwo->SetMarked(true);

		set_menu();
		
		BFont font;
		Window()->Lock();
 		font.SetFamilyAndStyle(info.f_family, info.f_style);
 		font.SetSize(info.font_size);
 		SetFont(&font);
 		SetViewColor(info.background_color);
		Window()->Unlock();
	
		// force the menu to redraw
		InvalidateLayout();
	}
	
	void MenuBar::FrameResized(float width, float height)
	{
		Window()->PostMessage(UPDATE_WINDOW);	
	}