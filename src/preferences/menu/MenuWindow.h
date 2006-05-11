#ifndef __MENU_WINDOW_H
#define __MENU_WINDOW_H

#include <Menu.h>
#include <Window.h>

class ColorWindow;
class BMenuItem;
class BBox;
class BButton;
class MenuBar;
class MenuWindow : public BWindow {
public:
			MenuWindow(BRect frame);
	virtual void	MessageReceived(BMessage *msg);
	virtual	void	Update();
		void	Defaults();

private:	
	bool		revert;
	ColorWindow	*colorWindow;
	BMenuItem 	*toggleItem;
	BMenu		*menu;
	MenuBar		*menuBar;
	BBox		*menuView;
	BButton 	*revertButton;
	BButton 	*defaultButton;
};

#endif
