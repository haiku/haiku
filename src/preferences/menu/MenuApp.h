#ifndef __MENU_APP_H 
#define __MENU_APP_H

#include <Application.h>
#include <Menu.h>

class MenuWindow;	
class MenuApp : public BApplication {
public:
			MenuApp();
	virtual void	Update();
	virtual void	MessageReceived(BMessage *msg);
	
private:
	MenuWindow	*menuWindow;
	BRect		rect;
	menu_info	info;
	BMenu 		*menu;
};
	
#endif
