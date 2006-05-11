#include "FontMenu.h"
#include "MenuSettings.h"
#include "msg.h"

#include <Message.h>
#include <MenuItem.h>
#include <Window.h>

FontSizeMenu::FontSizeMenu()
	:AutoSettingsMenu("Font Size", B_ITEMS_IN_COLUMN)
{		
	menu_info info;
	MenuSettings::GetInstance()->Get(info);
	
	BMessage *msg = new BMessage(MENU_FONT_SIZE);
	msg->AddFloat("size", 9);
	BMenuItem *item = new BMenuItem("9", msg, 0, 0);
	AddItem(item);
	if (info.font_size == 9)
		item->SetMarked(true);
	
	msg = new BMessage(MENU_FONT_SIZE);
	msg->AddFloat("size", 10);
	item = new BMenuItem("10", msg, 0, 0);
	AddItem(item);
	if (info.font_size == 10)
		item->SetMarked(true);
	
	msg = new BMessage(MENU_FONT_SIZE);
	msg->AddFloat("size", 11);
	item = new BMenuItem("11", msg, 0, 0);
	AddItem(item);
	if (info.font_size == 11)
		item->SetMarked(true);
	
	msg = new BMessage(MENU_FONT_SIZE);
	msg->AddFloat("size", 12);
	item = new BMenuItem("12", msg, 0, 0);
	AddItem(item);
	if (info.font_size == 12)
		item->SetMarked(true);
	
	msg = new BMessage(MENU_FONT_SIZE);
	msg->AddFloat("size", 14);
	item = new BMenuItem("14", msg, 0, 0);
	AddItem(item);
	if (info.font_size == 14)
		item->SetMarked(true);
	
	msg = new BMessage(MENU_FONT_SIZE);
	msg->AddFloat("size", 18);
	item = new BMenuItem("18", msg, 0, 0);
	AddItem(item);
	if (info.font_size == 18)
		item->SetMarked(true);
	
	SetRadioMode(true);
}
