#ifndef __MENU_APP_H 
#define __MENU_APP_H

#include <Application.h>

class MenuWindow;	
class MenuApp : public BApplication {
public:
			MenuApp();
	virtual void	MessageReceived(BMessage *msg);
	
private:
	MenuWindow	*fMenuWindow;
};
	
#endif
