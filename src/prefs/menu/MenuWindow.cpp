	#include "MenuApp.h"
	#include <stdio.h>
	#include <Roster.h>
	
	int ans;
	
	MenuWindow::MenuWindow()
		: BWindow(rect, "Menu", B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
	{
	 	get_menu_info(&revert_info);
	 	get_menu_info(&info);
	 	
	 	revert = false;
	 	
	 	MoveTo((rect.left += 100),(rect.top += 100));
	 	BRect r = Bounds();
	 	r.left -= 1;
	 	r.top -= 1;
	 	menuView = new BBox(r, "menuView",
			B_FOLLOW_ALL, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE);
		menuView->SetViewColor(219,219,219,255);
		menuBar = new MenuBar();
		menuView->AddChild(menuBar);
		AddChild(menuView);
		menuView->ResizeTo((Frame().right),(Frame().bottom));
		defaultButton = new BButton(BRect(10,0,85,20), "Default", "Defaults",
			new BMessage(MENU_DEFAULT), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_NAVIGABLE);
		revertButton = new BButton(BRect(95,0,175,20), "Revert", "Revert",
			new BMessage(MENU_REVERT), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_NAVIGABLE);
		revertButton->SetEnabled(false);
		
		menuView->AddChild(defaultButton);
		menuView->AddChild(revertButton);
		
		Update();
		
	}
	
	MenuWindow::~MenuWindow()
	{/*nothing to delete;*/}
	
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
			info.f_family = *family;
			info.f_style = *style;
			set_menu_info(&info);
			Update();
			break;
		}
		
		case MENU_FONT_SIZE:
			revert = true;
			float f;
			msg->FindFloat("size", &f);
			info.font_size = f;
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
			menuBar->ctlAsShortcutItem->SetMarked(true);
			menuBar->altAsShortcutItem->SetMarked(false);
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
			menuBar->altAsShortcutItem->SetMarked(true);
			menuBar->ctlAsShortcutItem->SetMarked(false);
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
			BMessage(msg);
			break;
		}
	}
	
	bool
	MenuWindow::QuitRequested()
	{
		be_app->PostMessage(B_QUIT_REQUESTED);
		return true;
	}
	
	void
	MenuWindow::Update()
	{
    	revertButton->SetEnabled(revert);
    
    	// alert the rest of the application to update	
		menuBar->Update();
		
		// resize the window according to the size of menuBar
		ResizeTo((menuBar->Frame().right + 35), (menuBar->Frame().bottom + 45));
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