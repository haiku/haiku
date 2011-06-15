/*
 * Copyright 2004-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Bachmann
 */


#include "AddOnMonitor.h"
#include "AddOnMonitorHandler.h"
#include <Message.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <stdio.h>


AddOnMonitor::AddOnMonitor()
	:
	BLooper("AddOnMonitor"),
	fInitCheck(B_NO_INIT),
	fPulseRunner(NULL)
{
}


AddOnMonitor::AddOnMonitor(AddOnMonitorHandler* handler)
	:
	BLooper("AddOnMonitor"),
	fInitCheck(B_NO_INIT),
	fPulseRunner(NULL)
{
	SetHandler(handler);

	thread_id id = Run();
	if (id < 0) {
		fInitCheck = (status_t)id;
		fprintf(stderr, "AddOnMonitor() : bad id returned by Run()\n");
		return;
	}
}


AddOnMonitor::~AddOnMonitor()
{
	delete fPulseRunner;
}


status_t
AddOnMonitor::InitCheck()
{
	return fInitCheck;
}


void
AddOnMonitor::SetHandler(AddOnMonitorHandler* handler)
{
	if (handler == NULL)
		return;

	AddHandler(handler);
	SetPreferredHandler(handler);

	delete fPulseRunner;
	fPulseRunner = NULL;

	status_t status;
	BMessenger messenger(handler, this, &status);
	if (status != B_OK) {
		fInitCheck = status;
		return;
	}

	BMessage pulseMessage(B_PULSE);
	fPulseRunner = new(std::nothrow) BMessageRunner(messenger, &pulseMessage,
		1000000);
	if (fPulseRunner == NULL) {
		fInitCheck = B_NO_MEMORY;
		return;
	}

	status = fPulseRunner->InitCheck();
	if (status != B_OK) {
		fInitCheck = status;
		fprintf(stderr, "AddOnMonitor() : bad status returned by "
			"fPulseRunner->InitCheck()\n");
		return;
	}

	// Send an initial message to process added directories immediately
	messenger.SendMessage(&pulseMessage);

	fInitCheck = B_OK;
}
