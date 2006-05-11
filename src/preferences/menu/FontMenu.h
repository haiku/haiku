#ifndef __FONTMENU_H
#define __FONTMENU_H

#include "AutoSettingsMenu.h"

class FontSizeMenu : public AutoSettingsMenu {
public:
			FontSizeMenu();

private:	
	menu_info 	info;
	BMenuItem	*fontSizeNine;
	BMenuItem	*fontSizeTen;
	BMenuItem	*fontSizeEleven;
	BMenuItem	*fontSizeTwelve;
	BMenuItem	*fontSizeFourteen;
	BMenuItem	*fontSizeEighteen;
};
	

class FontMenu : public AutoSettingsMenu {
public:
			FontMenu();
	virtual void	AttachedToWindow();
	void		GetFonts();
	
	status_t	PlaceCheckMarkOnFont(font_family family, font_style style);
	void		ClearAllMarkedItems();
private:
	menu_info	info;
};

#endif
