/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ACTIVITY_MONITOR_H
#define ACTIVITY_MONITOR_H


#include <Application.h>
#include <Catalog.h>

class BMessage;
class ActivityWindow;

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "ActivityWindow"

static const char* kAppName = B_TRANSLATE_MARK("ActivityMonitor");


class ActivityMonitor : public BApplication {
public:
					ActivityMonitor();
	virtual			~ActivityMonitor();

	virtual	void	ReadyToRun();

	virtual	void	RefsReceived(BMessage* message);
	virtual	void	MessageReceived(BMessage* message);

	virtual	void	AboutRequested();

	static	void	ShowAbout();

private:
	ActivityWindow*	fWindow;
};

extern const char* kSignature;

#endif	// ACTIVITY_MONITOR_H
