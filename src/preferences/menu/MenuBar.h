#ifndef __BARMENU_H
#define __BARMENU_H

#include <MenuBar.h>

class BMenuItem;
class FontMenu;
class FontSizeMenu;

class MenuBar : public BMenuBar {
public:
			MenuBar();
	virtual	void	AttachedToWindow();
		void	set_menu();
		void	build_menu();
	virtual void	Update();
	virtual void 	FrameResized(float width, float height);

private:	
	menu_info 		info;
	
	//seperator submenu
	BMenu			*separatorStyleMenu;
	BMenuItem		*separatorStyleZero;
	BMenuItem		*separatorStyleOne;
	BMenuItem		*separatorStyleTwo;
	
	//others
	FontMenu		*fontMenu;
	FontSizeMenu		*fontSizeMenu;
	BMenuItem		*clickToOpenItem;
	BMenuItem		*alwaysShowTriggersItem;
	BMenuItem		*colorSchemeItem;
	BMenuItem		*separatorStyleItem;
	BMenuItem		*ctlAsShortcutItem;
	BMenuItem		*altAsShortcutItem;
};

#endif
