/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors in chronological order:
 *		<unknown, please fill in who knows>
 *		Vasilis Kaoutsis, kaoutsis@sch.gr
 */
#ifndef __COLORWINDOW_H
#define __COLORWINDOW_H


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
		BColorControl 	*colorPicker;
		BButton 	*DefaultButton;
		BButton 	*RevertButton;
		menu_info	revert_info;
		menu_info 	info;
		BMessenger	fOwner;
};

#endif	// __COLORWINDOW_H
