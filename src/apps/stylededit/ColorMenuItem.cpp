#ifndef _MENU_ITEM_H
#include <MenuItem.h>
#endif

#ifndef COLOR_MENU_ITEM_H
#include "ColorMenuItem.h"
#endif

ColorMenuItem::ColorMenuItem(const char	*label, rgb_color color,BMessage *message)
	:BMenuItem(label, message,0,0){
	fItemColor= color;
}

void ColorMenuItem::DrawContent(){
	
		BMenu *menu= Menu();
		menu->SetHighColor(fItemColor);
		BMenuItem::DrawContent();
}
