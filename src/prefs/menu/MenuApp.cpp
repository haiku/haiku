
	#include "MenuApp.h"
	
	MenuApp::MenuApp()
		: BApplication("application/x-vnd.Be-GGUI")
	{
		get_menu_info(&info);
		
		menuWindow = new MenuWindow();
		Update();
		menuWindow->Show();
	}
	
	MenuApp::~MenuApp()
	{/*nothing to clean up*/}
	
	bool
	MenuApp::QuitRequested() 
	{
		return true;
	}

	void
	MenuApp::Update()
	{
		menuWindow->Update();
 	}
	
	int	main()
	{
		new MenuApp();
		be_app -> Run();
		delete be_app;
	}
	
	void
	MenuApp::MessageReceived(BMessage *msg){
		switch(msg->what) {
		
		//others
		case UPDATE_WINDOW:
		case CLICK_OPEN_MSG:
		case ALLWAYS_TRIGGERS_MSG:
		case CTL_MARKED_MSG:
		case ALT_MARKED_MSG:
		case COLOR_SCHEME_MSG:
		case MENU_COLOR:
			menuWindow->PostMessage(msg);
			break;
						
		default:
			BMessage(msg);
			break;
		}
	}