/*
 * Copyright 2002-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		<unknown, please fill in who knows>
 *		Vasilis Kaoutsis, kaoutsis@sch.gr
 */
#ifndef COLOR_WINDOW_H
#define COLOR_WINDOW_H


#include <Menu.h>
#include <Messenger.h>
#include <Window.h>

class BColorControl;
class BButton;


class ColorWindow : public BWindow {
	public:
		ColorWindow(BMessenger owner);
		~ColorWindow();

		virtual void Quit();
		virtual void MessageReceived(BMessage* message);

	private:	
		BColorControl*	fColorControl;
		BButton*		fDefaultButton;
		BButton*		fRevertButton;
		menu_info		fRevertInfo;
		menu_info		fInfo;
		BMessenger		fOwner;
};

#endif	// COLOR_WINDOW_H
