#include "FontMenu.h"
#include "msg.h"

#include <Message.h>
#include <MenuItem.h>
#include <Window.h>

FontSizeMenu::FontSizeMenu()
	:BMenu("Font Size", B_ITEMS_IN_COLUMN)
{		
	get_menu_info(&info);
	
	BMessage *msg = new BMessage(MENU_FONT_SIZE);
	msg->AddFloat("size", 9);
	fontSizeNine = new BMenuItem("9", msg, 0, 0);
	AddItem(fontSizeNine);
	if(info.font_size == 9){fontSizeNine->SetMarked(true);}
	
	msg = new BMessage(MENU_FONT_SIZE);
	msg->AddFloat("size", 10);
	fontSizeTen = new BMenuItem("10", msg, 0, 0);
	AddItem(fontSizeTen);
	if(info.font_size == 10){fontSizeTen->SetMarked(true);}
	
	msg = new BMessage(MENU_FONT_SIZE);
	msg->AddFloat("size", 11);
	fontSizeEleven = new BMenuItem("11", msg, 0, 0);
	AddItem(fontSizeEleven);
	if(info.font_size == 11){fontSizeEleven->SetMarked(true);}
	
	msg = new BMessage(MENU_FONT_SIZE);
	msg->AddFloat("size", 12);
	fontSizeTwelve = new BMenuItem("12", msg, 0, 0);
	AddItem(fontSizeTwelve);
	if(info.font_size == 12){fontSizeTwelve->SetMarked(true);}
	
	msg = new BMessage(MENU_FONT_SIZE);
	msg->AddFloat("size", 14);
	fontSizeFourteen = new BMenuItem("14", msg, 0, 0);
	AddItem(fontSizeFourteen);
	if(info.font_size == 14){fontSizeFourteen->SetMarked(true);}
	
	msg = new BMessage(MENU_FONT_SIZE);
	msg->AddFloat("size", 18);
	fontSizeEighteen = new BMenuItem("18", msg, 0, 0);
	AddItem(fontSizeEighteen);
	if(info.font_size == 18){fontSizeEighteen->SetMarked(true);}
	
	SetTargetForItems(Window());
	SetRadioMode(true);
}


void
FontSizeMenu::Update()
{
	get_menu_info(&info);
	BFont font;
		
	if (LockLooper()) {
		font.SetFamilyAndStyle(info.f_family, info.f_style);
		font.SetSize(info.font_size);
		SetFont(&font);
		SetViewColor(info.background_color);
		InvalidateLayout();
		Invalidate();
		UnlockLooper();
	}
	
	SetEnabled(true);
}
