#include "FontMenu.h"
#include <Message.h>
#include <MenuItem.h>
#include <stdlib.h>
	
	FontMenu::FontMenu()
		: BMenu("", B_ITEMS_IN_COLUMN)
	{
		get_menu_info(&info);
		SetRadioMode(true);
		GetFonts();
	}
	
	FontMenu::~FontMenu()
	{ /*nothing to clean up*/}
	
	void
	FontMenu::GetFonts()
	{
		int32 numFamilies = count_font_families();
		for ( int32 i = 0; i < numFamilies; i++ ) {
			font_family *family = (font_family*)malloc(sizeof(font_family));
			uint32 flags;
			if ( get_font_family(i, family, &flags) == B_OK ) {
				fontStyleMenu = new BMenu(*family, B_ITEMS_IN_COLUMN);
				fontStyleMenu->SetRadioMode(true);
				int32 numStyles = count_font_styles(*family);
				for ( int32 j = 0; j < numStyles; j++ ) {
					font_style *style = (font_style*)malloc(sizeof(font_style));
					if ( get_font_style(*family, j, style, &flags) == B_OK ) {
						BMessage *msg = new BMessage(MENU_FONT_STYLE);
						msg->AddPointer("family", family);
						msg->AddPointer("style", style);
						fontStyleItem = new BMenuItem(*style, msg, 0, 0);
						fontStyleMenu->AddItem(fontStyleItem);
					}
				}
				BMessage *msg = new BMessage(MENU_FONT_FAMILY);
				msg->AddPointer("family", family);
				font_style *style = (font_style*)malloc(sizeof(font_style));
				// if font family selected, we need to change style to
				// first style
				if ( get_font_style(*family, 0, style, &flags) == B_OK )
					msg->AddPointer("style", style);
				fontFamily = new BMenuItem(fontStyleMenu, msg);
				AddItem(fontFamily);
			}
		}
	}
	
	void
	FontMenu::Update()
	{
		// it may be better to pull all menu prefs
		// related stuff out of the FontMenu class
		// so it can be easily reused in other apps
		get_menu_info(&info);
 		
		// font menu
		BFont font;
 		font.SetFamilyAndStyle(info.f_family, info.f_style);
 		font.SetSize(info.font_size);
 		SetFont(&font);
 		SetViewColor(info.background_color);
		InvalidateLayout();

		// font style menus 		
 		for (int i = 0; i < CountItems(); i++) {
 			ItemAt(i)->Submenu()->SetFont(&font);
		}	
		 
		ClearAllMarkedItems();
		PlaceCheckMarkOnFont(info.f_family, info.f_style);
	}
	
	status_t 
	FontMenu::PlaceCheckMarkOnFont(font_family family, font_style style)
	{
		BMenuItem *fontFamilyItem;
		BMenuItem *fontStyleItem;
		BMenu *styleMenu;
	
		fontFamilyItem = FindItem(family);

		if ((fontFamilyItem != NULL) && (family != NULL))
		{
			fontFamilyItem->SetMarked(true);
			styleMenu = fontFamilyItem->Submenu();
		
			if ((styleMenu != NULL) && (style != NULL))
			{
				fontStyleItem = styleMenu->FindItem(style);

				if (fontStyleItem != NULL)
				{
					fontStyleItem->SetMarked(true);
				}
			
			}
			else
				return B_ERROR;
		}
		else
			return B_ERROR;
			

		return B_OK;
	}

	void
	FontMenu::ClearAllMarkedItems()
	{
		// we need to clear all menuitems and submenuitems		
		for (int i = 0; i < CountItems(); i++) { 
			ItemAt(i)->SetMarked(false);
			
			BMenu *submenu = ItemAt(i)->Submenu(); 
			for (int j = 0; j < submenu->CountItems(); j++) { 
				submenu->ItemAt(j)->SetMarked(false);                                        
			} 
		} 
		
	}
