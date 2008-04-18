/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "SystemInfoHandler.h"

#include <Clipboard.h>
#include <Handler.h>
#include <List.h>
#include <Messenger.h>
#include <Roster.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


SystemInfoHandler::SystemInfoHandler()
	: BHandler("SystemInfoHandler")
{
}


SystemInfoHandler::~SystemInfoHandler()
{
}


status_t
SystemInfoHandler::Archive(BMessage *data, bool deep) const
{
	// we don't want ourselves to be archived at all...
	// return BHandler::Archive(data, deep);
	return B_OK;
}


void
SystemInfoHandler::StartWatchingStuff()
{
	fRunningApps = 0;
	fClipboardSize = 0;
	fClipboardTextSize = 0;
	
	// running applications count
	BList teamList;
	if (be_roster) {
		be_roster->StartWatching(BMessenger(this), 
			B_REQUEST_LAUNCHED | B_REQUEST_QUIT);
		be_roster->GetAppList(&teamList);
		fRunningApps = teamList.CountItems();
		teamList.MakeEmpty();
	}

	// useless clipboard size
	if (be_clipboard) {
		be_clipboard->StartWatching(BMessenger(this));
		_UpdateClipboardData();
	}
}


void
SystemInfoHandler::StopWatchingStuff()
{
	if (be_roster)
		be_roster->StopWatching(BMessenger(this));
	if (be_clipboard)
		be_clipboard->StopWatching(BMessenger(this));
}


void
SystemInfoHandler::MessageReceived(BMessage *message)
{
	switch (message->what) {
	case B_SOME_APP_LAUNCHED:
		fRunningApps++;
		// XXX: maybe resync periodically in case we miss one
		break;
	case B_SOME_APP_QUIT:
		fRunningApps--;
		// XXX: maybe resync periodically in case we miss one
		break;
	case B_CLIPBOARD_CHANGED:
		_UpdateClipboardData();
		break;
	default:
		BHandler::MessageReceived(message);
	}
}


uint32
SystemInfoHandler::RunningApps() const
{
	return fRunningApps;
}


uint32
SystemInfoHandler::ClipboardSize() const
{
	return fClipboardSize;
}


uint32
SystemInfoHandler::ClipboardTextSize() const
{
	return fClipboardTextSize;
}


void
SystemInfoHandler::_UpdateClipboardData()
{
	fClipboardSize = 0;
	fClipboardTextSize = 0;
	if (be_clipboard && be_clipboard->Lock()) {
		BMessage *data = be_clipboard->Data();
		if (data) {
			ssize_t size = data->FlattenedSize();
			const void *text;
			ssize_t textSize;
			fClipboardSize = (size < 0) ? 0 : (uint32)size;
			if (data->FindData("text/plain", B_MIME_TYPE, &text, &textSize) 
				>= B_OK)
				fClipboardTextSize = textSize;
		}
		be_clipboard->Unlock();
	}
}

