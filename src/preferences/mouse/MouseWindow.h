/*
 * Copyright 2003-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval,
 *		Axel Dörfler (axeld@pinc-software.de)
 *		Andrew McCall (mccall@digitalparadise.co.uk)
 */


#ifndef MOUSE_WINDOW_H
#define MOUSE_WINDOW_H


#include <Window.h>
#include <Button.h>

#include "MouseSettings.h"

class SettingsView;


class MouseWindow : public BWindow {
public:
		MouseWindow(BRect rect);

		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage *message);

private:
		MouseSettings	fSettings;
		BButton			*fDefaultsButton;
		BButton			*fRevertButton;
		SettingsView	*fSettingsView;
};

#endif	/* MOUSE_WINDOW_H */
