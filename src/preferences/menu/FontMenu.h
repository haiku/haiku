#ifndef __FONTMENU_H
#define __FONTMENU_H

#include <Menu.h>

class FontSizeMenu : public BMenu {
public:
			FontSizeMenu();
	virtual void	Update();

private:	
	menu_info 	info;
	BMenuItem	*fontSizeNine;
	BMenuItem	*fontSizeTen;
	BMenuItem	*fontSizeEleven;
	BMenuItem	*fontSizeTwelve;
	BMenuItem	*fontSizeFourteen;
	BMenuItem	*fontSizeEighteen;
};
	

class FontMenu : public BMenu {
public:
			FontMenu();
	virtual void	GetFonts();
	virtual void	Update();
	virtual status_t PlaceCheckMarkOnFont(font_family family, font_style style);
	virtual void	ClearAllMarkedItems();

private:
	menu_info	info;
	BMenuItem	*fontFamily;
	BMenu		*fontStyleMenu;
	BMenuItem	*fontStyleItem;
};

#endif
