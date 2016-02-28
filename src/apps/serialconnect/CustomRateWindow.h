/*
 * Copyright 2016, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under terms of the MIT license.
 */


#ifndef CUSTOMRATEWINDOW_H
#define CUSTOMRATEWINDOW_H


#include <Window.h>


class BSpinner;


class CustomRateWindow: public BWindow
{
	public:
					CustomRateWindow(int baudrate);
		void		MessageReceived(BMessage* message);

	private:
		BSpinner*	fSpinner;
};


#endif /* !CUSTOMRATEWINDOW_H */
