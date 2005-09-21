/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Reworked from DarkWyrm version in CDPlayer
 */

#ifndef _DRAW_BUTTON_H
#define _DRAW_BUTTON_H

#include "DialogPane.h"

class DrawButton : public PaneSwitch
{
public:
	DrawButton(BRect frame, const char *name, const char *labelOn, const char* labelOff, 
					BMessage *msg, int32 resize = B_FOLLOW_LEFT|B_FOLLOW_TOP,
					int32 flags = B_WILL_DRAW | B_NAVIGABLE);
	~DrawButton(void);
	
	void	Draw(BRect update);
private:
	char* fLabelOn, *fLabelOff;
};

#endif
