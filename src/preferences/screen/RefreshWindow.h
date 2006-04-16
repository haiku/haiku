/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rafael Romo
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef REFRESH_WINDOW_H
#define REFRESH_WINDOW_H


#include <Window.h>

class BSlider;


class RefreshWindow : public BWindow {
	public:
		RefreshWindow(BPoint position, float current, float min, float max);

		virtual void MessageReceived(BMessage* message);
		virtual void WindowActivated(bool active);

	private:
		BSlider* fRefreshSlider;
};

#endif	// REFRESH_WINDOW_H
