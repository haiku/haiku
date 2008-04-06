/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ACTIVITY_WINDOW_H
#define ACTIVITY_WINDOW_H


#include <Window.h>

class BFile;
class ActivityView;


class ActivityWindow : public BWindow {
public:
	ActivityWindow();
	virtual ~ActivityWindow();

	virtual void MessageReceived(BMessage* message);
	virtual bool QuitRequested();

private:
	status_t _OpenSettings(BFile& file, uint32 mode);
	status_t _LoadSettings(BMessage& settings);
	status_t _SaveSettings();

	void _MessageDropped(BMessage *message);

	ActivityView*	fActivityView;
};

#endif	// ACTIVITY_WINDOW_H
