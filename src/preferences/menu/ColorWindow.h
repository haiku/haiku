#ifndef __COLORWINDOW_H
#define __COLORWINDOW_H

#include <Menu.h>
#include <Window.h>

class BColorControl;
class BButton;
class ColorWindow : public BWindow {
	public:
			ColorWindow();
	virtual void	MessageReceived(BMessage *msg);
	
	BColorControl 	*colorPicker;
	BButton 	*DefaultButton;
	BButton 	*RevertButton;
	menu_info	revert_info;
	menu_info 	info;
};

#endif
