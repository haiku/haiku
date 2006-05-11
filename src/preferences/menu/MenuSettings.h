#ifndef __MENUSETTINGS_H
#define __MENUSETTINGS_H

#include <Menu.h>

class MenuSettings {
public:
	static MenuSettings *GetInstance();

	void Get(menu_info &info) const;
	void Set(menu_info &info);

	void Revert();
	void ResetToDefaults();

private:
	MenuSettings();
	~MenuSettings();

	menu_info fPreviousSettings;
	menu_info fDefaultSettings;	
};


#endif
