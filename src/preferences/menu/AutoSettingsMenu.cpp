#include "AutoSettingsMenu.h"

AutoSettingsMenu::AutoSettingsMenu(const char *name, menu_layout layout)
	:
	BMenu(name, layout)
{
}


void
AutoSettingsMenu::AttachedToWindow()
{
	menu_info info;
	get_menu_info(&info);
	BFont font;
			
	font.SetFamilyAndStyle(info.f_family, info.f_style);
	font.SetSize(info.font_size);
	SetFont(&font);
	SetViewColor(info.background_color);
	
	BMenu::AttachedToWindow();
}
