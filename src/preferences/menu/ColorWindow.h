/*
 * Copyright 2002-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		<unknown, please fill in who knows>
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
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

		virtual void Quit();
		virtual void MessageReceived(BMessage* message);

	private:
		void		_UpdateAndPost();

	private:	
		BColorControl*	fColorControl;
		BButton*		fDefaultButton;
		BButton*		fRevertButton;		
		rgb_color		fCurrentColor;
		rgb_color		fPreviousColor;
		rgb_color		fDefaultColor;	
		BMessenger		fOwner;
};

#endif	// COLOR_WINDOW_H
