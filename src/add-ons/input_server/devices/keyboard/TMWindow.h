// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        TMWindow.h
//  Author:      Jérôme Duval
//  Description: Keyboard input server addon
//  Created :    October 13, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#ifndef TMWINDOW_H
#define TMWINDOW_H

#include <Box.h>
#include <Button.h>
#include <ListView.h>
#include <Window.h>

#include "InputServerTypes.h"

class TMBox : public BBox {
public:
	TMBox(BRect bounds, const char* name=NULL, uint32 resizeFlags = B_FOLLOW_LEFT|B_FOLLOW_TOP,
		uint32 flags = B_WILL_DRAW|B_FRAME_EVENTS|B_NAVIGABLE_JUMP,
		border_style border = B_FANCY_BORDER);
	void Pulse();
	
	BListView *fListView;
};

class TMWindow : public BWindow 
{
public:
	TMWindow();
	~TMWindow();
	
	void MessageReceived(BMessage *msg);
	virtual bool QuitRequested();
	void Enable();
	void Disable();
private:
	bool fQuitting;
	
	BButton *fKillApp;
	TMBox *fBackground;
};


#endif //TMWINDOW_H
