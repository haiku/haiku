// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        MouseWindow.h
//  Author:      Jérôme Duval, Andrew McCall (mccall@digitalparadise.co.uk)
//  Description: Media Preferences
//  Created :   December 10, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
#ifndef MOUSE_WINDOW_H
#define MOUSE_WINDOW_H


#include <Window.h>
#include <Button.h>

#include "MouseSettings.h"

class MouseView;


class MouseWindow : public BWindow {
	public:
		MouseWindow(BRect rect);

		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage *message);

		void SetRevertable(bool revertable);

	private:
		MouseSettings	fSettings;
		BButton			*fRevertButton;
		MouseView		*fView;
};

#define kBorderSpace	10
#define	kItemSpace		7

#endif	/* MOUSE_WINDOW_H */
