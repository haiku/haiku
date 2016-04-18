/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */


#include <MenuItemPrivate.h>

#include <Menu.h>


namespace BPrivate {

MenuItemPrivate::MenuItemPrivate(BMenuItem* menuItem)
	:
	fMenuItem(menuItem)
{
}


void
MenuItemPrivate::SetSubmenu(BMenu* submenu)
{
	delete fMenuItem->fSubmenu;

	fMenuItem->_InitMenuData(submenu);

	if (fMenuItem->fSuper != NULL) {
		fMenuItem->fSuper->InvalidateLayout();

		if (fMenuItem->fSuper->LockLooper()) {
			fMenuItem->fSuper->Invalidate();
			fMenuItem->fSuper->UnlockLooper();
		}
	}
}


void
MenuItemPrivate::Install(BWindow* window)
{
	fMenuItem->Install(window);
}


void
MenuItemPrivate::Uninstall()
{
	fMenuItem->Uninstall();
}


}	// namespace BPrivate
