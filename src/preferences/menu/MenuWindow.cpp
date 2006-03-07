#include "ColorWindow.h"
#include "MenuApp.h"
#include "MenuBar.h"
#include "MenuWindow.h"
#include "msg.h"

#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <MenuItem.h>
#include <Roster.h>
	
MenuWindow::MenuWindow(BRect rect)
	: BWindow(rect, "Menu", B_TITLED_WINDOW,
			B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE)
{
 	get_menu_info(&revert_info);
 	get_menu_info(&info);
 	
 	revert = false;
 	
	menuView = new BBox(Bounds(), "menuView", B_FOLLOW_ALL_SIDES,
				B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_PLAIN_BORDER);
	AddChild(menuView);
	
	menuBar = new MenuBar();
	menuView->AddChild(menuBar);
			
	// resize the window according to the size of menuBar
	ResizeTo((menuBar->Frame().right + 35), (menuBar->Frame().bottom + 45));
	
	BRect menuBarFrame = menuBar->Frame();
	BRect buttonFrame(menuBarFrame.left, menuBarFrame.bottom + 10, menuBarFrame.left + 75, menuBarFrame.bottom + 30);
	
	defaultButton = new BButton(buttonFrame, "Default", "Defaults", new BMessage(MENU_DEFAULT),
					B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_NAVIGABLE);
	menuView->AddChild(defaultButton);
	
	buttonFrame.OffsetBy(buttonFrame.Width() + 20, 0);
	revertButton = new BButton(buttonFrame, "Revert", "Revert", new BMessage(MENU_REVERT),
					B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_NAVIGABLE);
	revertButton->SetEnabled(false);
	menuView->AddChild(revertButton);
	
	menuView->MakeFocus();
	
	Update();	
}


void
MenuWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case MENU_REVERT:
			set_menu_info(&revert_info);
			revert = false;
			Update();
			break;
			
		case MENU_DEFAULT:
			Defaults();
			break;
			
		case UPDATE_WINDOW:
			Update();
			break;
		
		case MENU_FONT_FAMILY:
		case MENU_FONT_STYLE:
		{
			font_family *family;
			msg->FindPointer("family", (void**)&family);
			font_style *style;
			msg->FindPointer("style", (void**)&style);
			memcpy(info.f_family, family, sizeof(info.f_family));
			memcpy(info.f_style, style, sizeof(info.f_style));
			set_menu_info(&info);
			Update();
			break;
		}
		
		case MENU_FONT_SIZE:
			revert = true;
			float fontSize;
			msg->FindFloat("size", &fontSize);
			info.font_size = fontSize;
			set_menu_info(&info);
			Update();
			break;
				
		case MENU_SEP_TYPE:
			revert = true;
			int32 i;
			msg->FindInt32("sep", &i);
			info.separator = i;
			set_menu_info(&info);
			Update();
			break;
		
		case CLICK_OPEN_MSG:
			revert = true;
			if (info.click_to_open != true)
				info.click_to_open = true;
			else
				info.click_to_open = false;
			set_menu_info(&info);
			menuBar->set_menu();
			Update();
			break;
		
		case ALLWAYS_TRIGGERS_MSG:
			revert = true;
			if (info.triggers_always_shown != true)
				info.triggers_always_shown = true;
			else
				info.triggers_always_shown = false;
			set_menu_info(&info);
			menuBar->set_menu();
			Update();
			break;
		
		case CTL_MARKED_MSG:
			revert = true;
			// This might not be the same for all keyboards
			set_modifier_key(B_LEFT_COMMAND_KEY, 0x5c);
			set_modifier_key(B_RIGHT_COMMAND_KEY, 0x60);
			set_modifier_key(B_LEFT_CONTROL_KEY, 0x5d);
			set_modifier_key(B_RIGHT_OPTION_KEY, 0x5f);
			be_roster->Broadcast(new BMessage(B_MODIFIERS_CHANGED));
			Update();
			break;
		
		case ALT_MARKED_MSG:
			revert = true;
			// This might not be the same for all keyboards
			set_modifier_key(B_LEFT_COMMAND_KEY, 0x5d);
			set_modifier_key(B_RIGHT_COMMAND_KEY, 0x5f);
			set_modifier_key(B_LEFT_CONTROL_KEY, 0x5c);
			set_modifier_key(B_RIGHT_OPTION_KEY, 0x60);
			
			be_roster->Broadcast(new BMessage(B_MODIFIERS_CHANGED));
			Update();
			break;
		
		case COLOR_SCHEME_MSG:
			colorWindow = new ColorWindow();
			colorWindow->Show();
			break;
		
		case MENU_COLOR:
			set_menu_info(&info);
			(new BAlert("test","we made it","cool"))->Go();
			break;
		
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


void
MenuWindow::Update()
{
	revertButton->SetEnabled(revert);
   
	// alert the rest of the application to update	
	menuBar->Update();
}


void
MenuWindow::Defaults()
{
	// to set the default color. this should be changed 
	// to the system color for system wide compatability.
	rgb_color color;
	color.red = 219;
	color.blue = 219;
	color.green = 219;
	color.alpha = 255;

	// the default settings. possibly a call to the app_server
	// would provide and execute this information, as it does
	// for get_menu_info and set_menu_info (or is this information
	// coming from libbe.so? or else where?). 
	info.font_size = 12;
	//info.f_family = "test";
	//info.f_style = "test";
	info.background_color = color;
	info.separator = 0;
	info.click_to_open = true;
	info.triggers_always_shown = false;
	set_menu_info(&info);
	Update();
}
