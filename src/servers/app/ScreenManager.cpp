/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */

/**	Manages all available physical screens */


#include "ScreenManager.h"
#include "ServerScreen.h"

#include <Autolock.h>
#include <Entry.h>
#include <NodeMonitor.h>

#include <new>

using std::nothrow;


#ifdef __HAIKU__
#	define USE_ACCELERANT 1
#else
#	define USE_ACCELERANT 0
#endif

#if USE_ACCELERANT
#	include "AccelerantHWInterface.h"
#else
#	include "ViewHWInterface.h"
#	include "DWindowHWInterface.h"
#endif


ScreenManager* gScreenManager;


ScreenManager::ScreenManager()
	: BLooper("screen manager"),
	fScreenList(4)
{
	_ScanDrivers();

	// turn on node monitoring the graphics driver directory
	BEntry entry("/dev/graphics");
	node_ref nodeRef;
	if (entry.InitCheck() == B_OK && entry.GetNodeRef(&nodeRef) == B_OK)
		watch_node(&nodeRef, B_WATCH_DIRECTORY, this);
}


ScreenManager::~ScreenManager()
{
	for (int32 i = 0; i < fScreenList.CountItems(); i++) {
		screen_item* item = fScreenList.ItemAt(i);

		delete item->screen;
		delete item;
	}
}


Screen*
ScreenManager::ScreenAt(int32 index) const
{
	if (!IsLocked())
		debugger("Called ScreenManager::ScreenAt() without lock!");

	screen_item* item = fScreenList.ItemAt(index);
	if (item != NULL)
		return item->screen;

	return NULL;
}


int32
ScreenManager::CountScreens() const
{
	if (!IsLocked())
		debugger("Called ScreenManager::CountScreens() without lock!");

	return fScreenList.CountItems();
}


status_t
ScreenManager::AcquireScreens(ScreenOwner* owner, int32* wishList,
	int32 wishCount, bool force, ScreenList& list)
{
	BAutolock locker(this);
	int32 added = 0;

	// ToDo: don't ignore the wish list

	for (int32 i = 0; i < fScreenList.CountItems(); i++) {
		screen_item *item = fScreenList.ItemAt(i);

		if (item->owner == NULL && list.AddItem(item->screen)) {
			item->owner = owner;
			added++;
		}
	}

	return added > 0 ? B_OK : B_ENTRY_NOT_FOUND;
}


void
ScreenManager::ReleaseScreens(ScreenList& list)
{
	BAutolock locker(this);

	for (int32 i = 0; i < fScreenList.CountItems(); i++) {
		screen_item *item = fScreenList.ItemAt(i);

		for (int32 j = 0; j < list.CountItems(); j++) {
			Screen* screen = list.ItemAt(j);

			if (item->screen == screen)
				item->owner = NULL;
		}
	}
}


void
ScreenManager::_ScanDrivers()
{
	HWInterface *interface = NULL;

	// Eventually we will loop through drivers until
	// one can't initialize in order to support multiple monitors.
	// For now, we'll just load one and be done with it.

	// ToDo: to make monitoring the driver directory useful, we need more
	//	power and data here, and should do the scanning on our own

	bool initDrivers = true;
	while (initDrivers) {

#if USE_ACCELERANT
		  interface = new AccelerantHWInterface();
#else
		  interface = new DWindowHWInterface();
//		  interface = new ViewHWInterface();
#endif

		_AddHWInterface(interface);
		initDrivers = false;
	}
}


void
ScreenManager::_AddHWInterface(HWInterface* interface)
{
	Screen* screen = new(nothrow) Screen(interface, fScreenList.CountItems() + 1);
	if (screen == NULL) {
		delete interface;
		return;
	}

	// The interface is now owned by the screen

	if (screen->Initialize() >= B_OK) {
		screen_item* item = new(nothrow) screen_item;
		if (item != NULL) {
			item->screen = screen;
			item->owner = NULL;
			if (fScreenList.AddItem(item))
				return;

			delete item;
		}
	}

	delete screen;
}


void
ScreenManager::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
			// ToDo: handle notification
			break;

		default:
			BHandler::MessageReceived(message);
	}
}

