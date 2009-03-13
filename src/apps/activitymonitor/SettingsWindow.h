/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H


#include <Messenger.h>

#include "ActivityWindow.h"

class IntervalSlider;


class SettingsWindow : public BWindow {
public:
						SettingsWindow(ActivityWindow* target);
	virtual				~SettingsWindow();

	virtual void		MessageReceived(BMessage* message);
	virtual bool		QuitRequested();

private:
	BRect				_RelativeTo(BWindow* window);

	BMessenger			fTarget;
	IntervalSlider*		fIntervalSlider;
};

static const uint32 kMsgTimeIntervalUpdated = 'tiup';

#endif	// SETTINGS_WINDOW_H
