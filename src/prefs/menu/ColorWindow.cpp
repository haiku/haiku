
	#include "MenuApp.h"
	
	ColorWindow::ColorWindow()
		: BWindow(BRect(150,150,350,200), "Menu Color Schene", B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
	{
		// Set and collect the variables for revert
		get_menu_info(&info);
		get_menu_info(&revert_info);
			
		BView *colView = new BView(BRect(0,0,1000,100), "menuView",
			B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE);
		
		colorPicker = new ColorPicker();
		colView->SetViewColor(219,219,219,255);
		colView->AddChild(colorPicker);
		AddChild(colView);
		
		ResizeTo(383,130);
		
		// Create the buttons and add them to the view
		DefaultButton = new BButton(BRect(10,100,85,110), "Default", "Default",
			new BMessage(MENU_COLOR_DEFAULT), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
		RevertButton = new BButton(BRect(95,100,175,20), "REVERT", "Revert",
			new BMessage(MENU_REVERT), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_NAVIGABLE);
		
		colView->AddChild(DefaultButton);
		colView->AddChild(RevertButton);
	}
	
	ColorWindow::~ColorWindow()
	{}
	
	void
	ColorWindow::MessageReceived(BMessage *msg)
	{
		switch(msg->what) {
		
		case MENU_REVERT:
			colorPicker->SetValue(revert_info.background_color);
			info.background_color = colorPicker->ValueAsColor();
			set_menu_info(&info);
			be_app->PostMessage(UPDATE_WINDOW);
			break;
					
		case MENU_COLOR_DEFAULT:
		//	change to system color for system wide
		//	compatability
			rgb_color color;
			color.red = 219;
			color.blue = 219;
			color.green = 219;
			color.alpha = 255;
			colorPicker->SetValue(color);
		
		case MENU_COLOR:{
			get_menu_info(&info);
			info.background_color = colorPicker->ValueAsColor();
			set_menu_info(&info);
			be_app->PostMessage(UPDATE_WINDOW);
			break;}
		
		default:
			be_app->PostMessage(UPDATE_WINDOW);
			BMessage(msg);
			break;
		}
	}
	