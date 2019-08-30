/*
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2007, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Beep.h>

#include <stdio.h>

#include <DataExchange.h>
#include <MediaSounds.h>


status_t
system_beep(const char* eventName)
{
	BMessenger messenger("application/x-vnd.Be.addon-host");
	if (!messenger.IsValid())
		return B_ERROR;

	BMessage msg(MEDIA_ADD_ON_SERVER_PLAY_MEDIA), reply;
	msg.AddString(MEDIA_NAME_KEY, eventName ? eventName : MEDIA_SOUNDS_BEEP);
	msg.AddString(MEDIA_TYPE_KEY, MEDIA_TYPE_SOUNDS);

	status_t status = messenger.SendMessage(&msg, &reply);
	if (status != B_OK || reply.FindInt32("error", &status) != B_OK)
		status = B_BAD_REPLY;

	return status;
}


status_t
beep()
{
	return system_beep(NULL);
}


status_t
add_system_beep_event(const char* name, uint32 flags)
{
	BMessenger messenger("application/x-vnd.Be.media-server");
	if (!messenger.IsValid())
		return B_ERROR;

	BMessage msg(MEDIA_SERVER_ADD_SYSTEM_BEEP_EVENT), reply;
	msg.AddString(MEDIA_NAME_KEY, name);
	msg.AddString(MEDIA_TYPE_KEY, MEDIA_TYPE_SOUNDS);
	msg.AddInt32(MEDIA_FLAGS_KEY, flags);

	status_t status = messenger.SendMessage(&msg, &reply);
	if (status != B_OK || reply.FindInt32("error", &status) != B_OK)
		status = B_BAD_REPLY;

	return status;
}
