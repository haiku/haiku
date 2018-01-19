/*
 * Copyright 2018, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TYPE_HANDLER_MENU_ITEM_H
#define TYPE_HANDLER_MENU_ITEM_H


#include "ActionMenuItem.h"


class TypeHandler;


class TypeHandlerMenuItem : public ActionMenuItem {
public:
								TypeHandlerMenuItem(const char* label,
									BMessage* message, char shortcut = 0,
									uint32 modifiers = 0);
	virtual						~TypeHandlerMenuItem();

	virtual	void				ItemSelected();

			status_t			SetTypeHandler(TypeHandler* handler);
									// takes over caller's reference

private:
	TypeHandler*				fTypeHandler;
};


#endif // TYPE_HANDLER_MENU_ITEM_H
