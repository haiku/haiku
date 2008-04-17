/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "SystemInfoHandler.h"

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
	
	// running applications count
	BList teamList;
	be_roster->StartWatching(BMessenger(this), 
		B_REQUEST_LAUNCHED | B_REQUEST_QUIT);
	be_roster->GetAppList(&teamList);
	fRunningApps = teamList.CountItems();
	teamList.MakeEmpty();

	// 
}


void
SystemInfoHandler::StopWatchingStuff()
{
	be_roster->StopWatching(BMessenger(this));
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
	default:
		BHandler::MessageReceived(message);
	}
}


uint32
SystemInfoHandler::RunningApps() const
{
	return fRunningApps;
}

