/*
 * Copyright 2008 Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fredrik Modéen 			fredrik@modeen.se 
 */
#ifndef PORT_ITEM_H
#define PORT_ITEM_H

#include <ListItem.h>
#include <String.h>

class PortItem : public BStringItem {
	public:
		PortItem(const char* label);
		~PortItem();
		virtual void DrawItem(BView *owner, BRect frame, bool complete = false);
		BString 	GetOldJoystickName();
		BString 	GetJoystickName();
		void		SetJoystickName(BString str);
	protected:
		BString 	fOldSelectedJoystick;
		BString 	fSelectedJoystick;
};


#endif	/* MESSAGED_ITEM_H */

