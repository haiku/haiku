// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2005, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        TMWindow.h
//  Author:      Jérôme Duval
//  Description: Input server bottomline window
//  Created :    January 24, 2005
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#ifndef BOTTOMLINEWINDOW_H
#define BOTTOMLINEWINDOW_H

#include <Message.h>
#include <TextView.h>
#include <Window.h>

class BottomlineWindow : public BWindow 
{
public:
	BottomlineWindow(const BFont *font);
	~BottomlineWindow();
	
	void MessageReceived(BMessage *msg);
	virtual bool QuitRequested();

	void HandleInputMethodEvent(BMessage *msg, BList *list);
//private:
	BTextView *fTextView;
};


#endif //BOTTOMLINEWINDOW_H
