
	#include "MenuApp.h"
	
	ColorPicker::ColorPicker()
		:BColorControl(BPoint(10,10), B_CELLS_32x8, 
			9, "COLOR", new BMessage(MENU_COLOR), true)
	{
		menu_info info;
		get_menu_info(&info);
		SetValue(info.background_color);
	}
	
	ColorPicker::~ColorPicker()
	{}
	
	void
	ColorPicker::MessageReceived(BMessage *msg){
		switch(msg->what) {
		}
	}