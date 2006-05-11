#include "FontMenu.h"
#include "MenuSettings.h"
#include "msg.h"

#include <stdlib.h>

#include <Message.h>
#include <MenuItem.h>

FontMenu::FontMenu()
	: AutoSettingsMenu("Font", B_ITEMS_IN_COLUMN)
{
	SetRadioMode(true);
	GetFonts();
}


void
FontMenu::AttachedToWindow()
{
	AutoSettingsMenu::AttachedToWindow();
	
	BFont font;
	GetFont(&font);

	// font style menus 		
	for (int i = 0; i < CountItems(); i++)
		ItemAt(i)->Submenu()->SetFont(&font);
	
	ClearAllMarkedItems();
	menu_info info;
	MenuSettings::GetInstance()->Get(info);
	PlaceCheckMarkOnFont(info.f_family, info.f_style);
}


void
FontMenu::GetFonts()
{
	int32 numFamilies = count_font_families();
	for ( int32 i = 0; i < numFamilies; i++ ) {
		font_family *family = (font_family*)malloc(sizeof(font_family));
		uint32 flags;
		if ( get_font_family(i, family, &flags) == B_OK ) {
			BMenu *fontStyleMenu = new BMenu(*family, B_ITEMS_IN_COLUMN);
			fontStyleMenu->SetRadioMode(true);
			int32 numStyles = count_font_styles(*family);
			for ( int32 j = 0; j < numStyles; j++ ) {
				font_style *style = (font_style*)malloc(sizeof(font_style));
				if ( get_font_style(*family, j, style, &flags) == B_OK ) {
					BMessage *msg = new BMessage(MENU_FONT_STYLE);
					msg->AddPointer("family", family);
					msg->AddPointer("style", style);
					BMenuItem *fontStyleItem = new BMenuItem(*style, msg, 0, 0);
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
			BMenuItem *fontFamily = new BMenuItem(fontStyleMenu, msg);
			AddItem(fontFamily);
		}
	}
}




status_t 
FontMenu::PlaceCheckMarkOnFont(font_family family, font_style style)
{
	BMenuItem *fontFamilyItem = FindItem(family);
	if (fontFamilyItem != NULL && family != NULL) {
		fontFamilyItem->SetMarked(true);
		BMenu *styleMenu = fontFamilyItem->Submenu();
	
		if (styleMenu != NULL && style != NULL) {
			BMenuItem *fontStyleItem = styleMenu->FindItem(style);
			if (fontStyleItem != NULL)
				fontStyleItem->SetMarked(true);
		
		} else
			return B_ERROR;
	} else
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
