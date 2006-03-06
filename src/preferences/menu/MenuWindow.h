#ifndef __MENU_WINDOW_H
#define __MENU_WINDOW_H

#include <Window.h>

class ColorWindow;
class BMenuItem;
class BMenu;
class BBox;
class BButton;
class MenuBar;
class MenuWindow : public BWindow {
public:
			MenuWindow();
	virtual void	MessageReceived(BMessage *msg);
	virtual bool	QuitRequested();
	virtual	void	Update();
		void	Defaults();

private:	
	bool		revert;
	ColorWindow	*colorWindow;
	BMenuItem 	*toggleItem;
	menu_info	info;
	menu_info	revert_info;
	BRect		rect;	
	BMenu		*menu;
	MenuBar		*menuBar;
	BBox		*menuView;
	BButton 	*revertButton;
	BButton 	*defaultButton;
};

#endif
