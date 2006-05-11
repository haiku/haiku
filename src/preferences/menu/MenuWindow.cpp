#include "ColorWindow.h"
#include "MenuApp.h"
#include "MenuBar.h"
#include "MenuSettings.h"
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
 	revert = false;
 	
	menuView = new BBox(Bounds(), "menuView", B_FOLLOW_ALL_SIDES,
				B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_PLAIN_BORDER);
	AddChild(menuView);
	
	menuBar = new MenuBar();
	menuView->AddChild(menuBar);
			
	// resize the window according to the size of menuBar
	ResizeTo((menuBar->Frame().right + 40), (menuBar->Frame().bottom + 45));
	
	BRect menuBarFrame = menuBar->Frame();
	BRect buttonFrame(menuBarFrame.left, menuBarFrame.bottom + 10, menuBarFrame.left + 75, menuBarFrame.bottom + 30);
	
	defaultButton = new BButton(buttonFrame, "Default", "Defaults", new BMessage(MENU_DEFAULT),
					B_FOLLOW_H_CENTER | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_NAVIGABLE);
	menuView->AddChild(defaultButton);
	
	buttonFrame.OffsetBy(buttonFrame.Width() + 20, 0);
	revertButton = new BButton(buttonFrame, "Revert", "Revert", new BMessage(MENU_REVERT),
					B_FOLLOW_H_CENTER | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_NAVIGABLE);
	revertButton->SetEnabled(false);
	menuView->AddChild(revertButton);
	
	menuView->MakeFocus();
	
	Update();	
}


void
MenuWindow::MessageReceived(BMessage *msg)
{
	MenuSettings *settings = MenuSettings::GetInstance();
	menu_info info;
	switch(msg->what) {
		case MENU_REVERT:
			revert = false;
			settings->Revert();
			Update();
			break;
			
		case MENU_DEFAULT:
			revert = true;
			settings->ResetToDefaults();
			Update();
			break;
			
		case UPDATE_WINDOW:
			Update();
			break;
		
		case MENU_FONT_FAMILY:
		case MENU_FONT_STYLE:
		{
			revert = true;
			font_family *family;
			msg->FindPointer("family", (void**)&family);
			font_style *style;
			msg->FindPointer("style", (void**)&style);
			settings->Get(info);
			memcpy(info.f_family, family, sizeof(info.f_family));
			memcpy(info.f_style, style, sizeof(info.f_style));
			settings->Set(info);
			Update();
			break;
		}
		
		case MENU_FONT_SIZE:
			revert = true;
			settings->Get(info);
			msg->FindFloat("size", &info.font_size);
			settings->Set(info);
			Update();
			break;
				
		case MENU_SEP_TYPE:
			revert = true;
			settings->Get(info);
			msg->FindInt32("sep", &info.separator);
			settings->Set(info);
			Update();
			break;
		
		case ALLWAYS_TRIGGERS_MSG:
			revert = true;
			settings->Get(info);
			info.triggers_always_shown = !info.triggers_always_shown;
			settings->Set(info);
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
			revert = true;
			Update();
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
