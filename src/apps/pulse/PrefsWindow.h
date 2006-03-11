/*
 * Copyright 2002-2006 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Copyright 1999, Be Incorporated. All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 *
 * Written by:	Daniel Switkin
 */
#ifndef PREFS_WINDOW_H
#define PREFS_WINDOW_H


#include "Prefs.h"

#include <Messenger.h>
#include <Window.h>

class BTabView;

class PrefsWindow : public BWindow {
	public:
		PrefsWindow(BRect rect, const char *name, BMessenger *messenger, Prefs *prefs);
		virtual ~PrefsWindow();

		virtual void MessageReceived(BMessage *message);
		virtual bool QuitRequested();

	private:
		BTabView*	fTabView;
		BMessenger	fTarget;
		Prefs*		fPrefs;
};

#endif	// PREFS_WINDOW_H
