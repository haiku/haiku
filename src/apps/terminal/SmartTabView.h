/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 *	Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 */

#ifndef __SMARTTABVIEW_H
#define __SMARTTABVIEW_H

#include <TabView.h>

class SmartTabView : public BTabView {
public:
	SmartTabView(BRect frame, const char *name,
			button_width width = B_WIDTH_AS_USUAL, 
			uint32 resizingMode = B_FOLLOW_ALL,
			uint32 flags = B_FULL_UPDATE_ON_RESIZE |
					B_WILL_DRAW | B_NAVIGABLE_JUMP |
					B_FRAME_EVENTS | B_NAVIGABLE);
	virtual ~SmartTabView();
	virtual	void Select(int32 tab);	

};

#endif // __SMARTTABVIEW_H

