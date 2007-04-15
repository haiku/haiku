/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <Beep.h>
#include <stdio.h>

#include "DataExchange.h"


status_t
system_beep(const char *eventName)
{
	BMessenger messenger("application/x-vnd.Be.media-server");
	if (!messenger.IsValid())
		return B_ERROR;
	BMessage msg(MEDIA_SERVER_SYSTEM_BEEP_EVENT), reply;
	msg.AddInt32("action", SYSTEM_BEEP_EVENT_INVOKE);
	msg.AddString("event", eventName);
	
	status_t err = messenger.SendMessage(&msg, &reply);
	if ((err != B_OK)
		|| (reply.FindInt32("error", &err) != B_OK))
		err = B_BAD_REPLY;
	return err;
}


status_t
beep()
{
	return system_beep(NULL);
}


status_t
add_system_beep_event(const char *eventName, uint32 flags)
{
	BMessenger messenger("application/x-vnd.Be.media-server");
	if (!messenger.IsValid())
		return B_ERROR;
	BMessage msg(MEDIA_SERVER_SYSTEM_BEEP_EVENT), reply;
	msg.AddInt32("action", SYSTEM_BEEP_EVENT_ADD);
	msg.AddString("event", eventName);
	msg.AddInt32("flags", flags);
	
	status_t err = messenger.SendMessage(&msg, &reply);
	if ((err != B_OK)
		|| (reply.FindInt32("error", &err) != B_OK))
		err = B_BAD_REPLY;
	return err;
}

