/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */
#ifndef __MENU_ITEM_PRIVATE_H
#define __MENU_ITEM_PRIVATE_H


#include <MenuItem.h>


class BMenu;

namespace BPrivate {

class MenuItemPrivate {
public:
								MenuItemPrivate(BMenuItem* menuItem);

			void				SetSubmenu(BMenu* submenu);

			void				Install(BWindow* window);
			void				Uninstall();

private:
			BMenuItem*			fMenuItem;
};

};	// namespace BPrivate


#endif	// __MENU_ITEM_PRIVATE_H
