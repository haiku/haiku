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

#include "RefreshView.h"
#include "RefreshSlider.h"

class RefreshWindow : public BWindow {
	public:
		RefreshWindow(BRect frame, float current, float min, float max);

		virtual void MessageReceived(BMessage *message);
		virtual void WindowActivated(bool active);

	private:
		RefreshView			*fRefreshView;
		RefreshSlider		*fRefreshSlider;
		BButton				*fDoneButton;
		BButton				*fCancelButton;
};

#endif	// REFRESH_WINDOW_H
