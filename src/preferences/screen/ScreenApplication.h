/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rafael Romo
 *		Stefano Ceccherini (burton666@libero.it)
 *		Andrew Bachmann
 *		Sergei Panteleev
 */
#ifndef SCREEN_APPLICATION_H
#define SCREEN_APPLICATION_H


#include <Application.h>


class ScreenWindow;

class ScreenApplication : public BApplication {
	public:
		ScreenApplication();

		virtual void MessageReceived(BMessage *message);
		virtual void AboutRequested();

	private:
		ScreenWindow *fScreenWindow;
};

#endif	/* SCREEN_APPLICATION_H */
