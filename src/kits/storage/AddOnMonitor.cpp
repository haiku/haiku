#include "AddOnMonitor.h"
#include "AddOnMonitorHandler.h"
#include <Message.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <stdio.h>

AddOnMonitor::AddOnMonitor(AddOnMonitorHandler * handler)
	: BLooper("AddOnMonitor")
{
	fInitCheck = B_NO_INIT;
	AddHandler(handler);
	SetPreferredHandler(handler);
	status_t status;
	BMessenger messenger(handler, this, &status);
	if (status != B_OK) {
		fInitCheck = status;
		return;
	}
	if (!messenger.IsValid()) {
		fInitCheck = B_ERROR;
		return;
	}
	fPulseMessage = new BMessage(B_PULSE);
	fPulseRunner = new BMessageRunner(messenger, fPulseMessage, 1000000);
	status = fPulseRunner->InitCheck();
	if (status != B_OK) {
		fInitCheck = status;
		return;
	}
	thread_id id = Run();
	if (id < 0) {
		fInitCheck = (status_t)id;
		return;
	}
	fInitCheck = B_OK;
	return;
}


/* virtual */
AddOnMonitor::~AddOnMonitor()
{
	delete fPulseMessage;
	delete fPulseRunner;
}


/* virtual */ status_t
AddOnMonitor::InitCheck()
{
	return fInitCheck;
}
