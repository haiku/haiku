/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "listener_output.h"

#include <stdio.h>


static BLocker sLocker;
static BList sListeners;


static void
listener_output(syslog_message &message)
{
	// compose the message to be sent to all listeners; just convert
	// the syslog_message into a BMessage
	BMessage output(SYSLOG_MESSAGE);

	output.AddInt32("from", message.from);
	output.AddInt32("when", message.when);
	output.AddString("ident", message.ident);
	output.AddString("message", message.message);
	output.AddInt32("options", message.options);
	output.AddInt32("priority", message.priority);

	sLocker.Lock();

	for (int32 i = sListeners.CountItems(); i-- > 0;) {
		BMessenger *target = (BMessenger *)sListeners.ItemAt(i);

		status_t status = target->SendMessage(&output);
		if (status < B_OK) {
			// remove targets once they can't be reached anymore
			sListeners.RemoveItem(target);
		}
	}

	sLocker.Unlock();
}


void 
remove_listener(BMessenger *messenger)
{
	if (sLocker.Lock()) {
		sListeners.RemoveItem(messenger);

		sLocker.Unlock();
	}
}


void 
add_listener(BMessenger *messenger)
{
	if (sLocker.Lock()) {
		sListeners.AddItem(messenger);

		sLocker.Unlock();
	}
}


void
init_listener_output(SyslogDaemon *daemon)
{
	daemon->AddHandler(listener_output);
}

