/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ACTION_MENU_ITEM_H
#define ACTION_MENU_ITEM_H


#include <MenuItem.h>


class ActionMenuItem : public BMenuItem {
public:
								ActionMenuItem(const char* label,
									BMessage* message, char shortcut = 0,
									uint32 modifiers = 0);
								ActionMenuItem(BMenu* menu,
									BMessage* message = NULL);
	virtual						~ActionMenuItem();

	virtual void				PrepareToShow(BLooper* parentLooper,
									BHandler* targetHandler);
	virtual bool				Finish(BLooper* parentLooper,
									BHandler* targetHandler, bool force);

	virtual void				ItemSelected();
};


#endif // ACTION_MENU_ITEM_H
