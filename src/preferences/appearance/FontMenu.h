#ifndef FONTMENU_H_
#define FONTMENU_H_

#include <Menu.h>

#define MENU_FONT_STYLE 'mnfs'
#define MENU_FONT_FAMILY 'mnff'

class FontMenu : public BMenu {
public:
	FontMenu();
	virtual ~FontMenu();
	virtual void GetFonts();
	virtual void Update();
	virtual status_t PlaceCheckMarkOnFont(font_family family, font_style style);
	virtual void ClearAllMarkedItems();

	menu_info		info;
	BMenuItem		*fontFamily;
	BMenu			*fontStyleMenu;
	BMenuItem		*fontStyleItem;
};
#endif
