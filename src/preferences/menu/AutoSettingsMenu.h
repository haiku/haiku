#ifndef __AUTOSETTINGSMENU_H
#define __AUTOSETTINGSMENU_H

#include <Menu.h>

class AutoSettingsMenu : public BMenu {
public:
			AutoSettingsMenu(const char *name, menu_layout layout);
	virtual void	AttachedToWindow();
};


#endif
