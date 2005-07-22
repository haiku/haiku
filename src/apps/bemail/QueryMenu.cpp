/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2001, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

#include "QueryMenu.h"
#include <Query.h>
#include <Autolock.h>
#include <MenuItem.h>
#include <NodeMonitor.h>
#include <VolumeRoster.h>
#include <Looper.h>
#include <Node.h>
#include <stdio.h>
#include <string.h>
#include <PopUpMenu.h>


BLooper *QueryMenu::fQueryLooper = NULL;
int32 QueryMenu::fMenuCount = 0;


// ***
// QHandler
// ***


class QHandler : public BHandler
{
	public:
		QHandler(QueryMenu *queryMenu);
		virtual void MessageReceived(BMessage *msg);

		QueryMenu *fQueryMenu;
};


QHandler::QHandler(QueryMenu *queryMenu)
	:	BHandler((const char *)NULL),
	fQueryMenu(queryMenu)
{
}


void QHandler::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case B_QUERY_UPDATE:
			fQueryMenu->DoQueryMessage(msg);
			break;
		
		default:
			BHandler::MessageReceived(msg);
			break;
	}
}


// ***
// QueryMenu
// ***


QueryMenu::QueryMenu(const char *title, bool popUp, bool radioMode, bool autoRename)
	:	BPopUpMenu(title, radioMode, autoRename),
	fTargetHandler(NULL),
	fPopUp(popUp)
{
	if (atomic_add(&fMenuCount, 1) == 0)
	{
		fQueryLooper = new BLooper("Query Watcher");
		fQueryLooper->Run();
	}
	fQueryHandler = new QHandler(this);
	fQueryLooper->Lock();
	fQueryLooper->AddHandler(fQueryHandler);
	fQueryLooper->Unlock();

//	BMessenger mercury(fQueryHandler, fQueryLooper);
//	fQuery = new BQuery();
//	fQuery->SetTarget(mercury);
}


QueryMenu::~QueryMenu(void)
{
	fCancelQuery = true;
	fQueryLock.Lock();

	int32 queries = fQueries.size();
	for (int i = 0; i < queries; i++) {
		fQueries[i]->Clear();
		delete fQueries[i];
	};
	fQueryLock.Unlock();
	
	fQueryLooper->Lock();
	fQueryLooper->RemoveHandler(fQueryHandler);
	delete fQueryHandler;
	if (atomic_add(&fMenuCount, -1) == 1)
		fQueryLooper->Quit();
	else
		fQueryLooper->Unlock();
}


void QueryMenu::DoQueryMessage(BMessage *msg)
{
	int32 opcode;
	int64 directory;
	int32 device;
	int64 node;
	if (msg->FindInt32("opcode", &opcode) == B_OK
		&& msg->FindInt64("directory", &directory) == B_OK
		&& msg->FindInt32("device", &device) == B_OK
		&& msg->FindInt64("node", &node) == B_OK)
	{
		const char *name;
		if (opcode == B_ENTRY_CREATED && msg->FindString("name", &name) == B_OK)
		{
			entry_ref ref(device, directory, name);
			EntryCreated(ref, node);
			return;
		}
		else if (opcode == B_ENTRY_REMOVED)
		{
			BAutolock lock(fQueryLock);
			if (!lock.IsLocked())
				return;
			EntryRemoved(node);
		}
	}
}


status_t QueryMenu::SetPredicate(const char *expr, BVolume *volume)
{
//	status_t status;

	if (volume == NULL) {
		BVolumeRoster roster;
		BVolume volume;
		BMessenger mercury(fQueryHandler, fQueryLooper);

		roster.Rewind();
		while (roster.GetNextVolume(&volume) == B_NO_ERROR) {
			if ((volume.KnowsQuery() == true) && (volume.KnowsAttr() == true) &&
				(volume.KnowsMime() == true)) {
				
				BQuery *query = new BQuery();
				if (query->SetVolume(&volume) != B_OK) {
					delete query;
					continue;
				};
				if (query->SetPredicate(expr) != B_OK) {
					delete query;
					continue;
				};
				if (query->SetTarget(mercury) != B_OK) {
					delete query;
					continue;
				};
				
				fQueries.push_back(query);
			};
		};
	} else {
	};

	// Set the volume
//	if (volume == NULL)
//	{
//		BVolume bootVolume;
//		BVolumeRoster().GetBootVolume(&bootVolume);
//
//		if ( (status = fQuery->SetVolume(&bootVolume)) != B_OK)
//			return status;
//	}
//	else if ((status = fQuery->SetVolume(volume)) != B_OK)
//		return status;
//	
//	if ((status = fQuery->SetPredicate(expr)) < B_OK)
//		return status;

	// Force query thread to exit if still running
	fCancelQuery = true;
	fQueryLock.Lock();

	// Remove all existing menu items (if any... )
	RemoveEntries();
	fQueryLock.Unlock();
	
	// Resolve Query/Build Menu in seperate thread
	thread_id thread;
	thread = spawn_thread(query_thread, "query menu thread", B_NORMAL_PRIORITY, this);

	return resume_thread(thread);
}


void QueryMenu::RemoveEntries()
{
	int64 node;
	for (int32 i = CountItems() - 1;i >= 0;i--)
	{
		if (ItemAt(i)->Message()->FindInt64("node", &node) == B_OK)
			RemoveItem(i);
	}
}


int32 QueryMenu::query_thread(void *data)
{
	return ((QueryMenu *)(data))->QueryThread();
}


int32 QueryMenu::QueryThread()
{
	BAutolock lock(fQueryLock);

	if (!lock.IsLocked())
		return B_ERROR;

	// Begin resolving query
	fCancelQuery = false;
	int32 queries = fQueries.size();
	for (int i = 0; i < queries; i++) {
		BQuery *query = fQueries[i];
	
		query->Fetch();
	
		// Build Menu
		entry_ref ref;
		node_ref node;
		while (query->GetNextRef(&ref) == B_OK && !fCancelQuery)
		{
			BEntry entry(&ref);
			entry.GetNodeRef(&node);
			EntryCreated(ref, node.node);
		}
	};

	// Remove the group separator if there are no groups or no items without groups
	BMenuItem *item;
	if (dynamic_cast<BSeparatorItem *>(item = ItemAt(0)) != NULL)
		RemoveItem(item);
	else if (dynamic_cast<BSeparatorItem *>(item = ItemAt(CountItems() - 1)) != NULL)
		RemoveItem(item);

	return B_OK;
}


status_t QueryMenu::SetTargetForItems(BHandler *handler)
{
	fTargetHandler = handler;
	return BMenu::SetTargetForItems(handler);
}

// Include the following version of SetTargetForItems() to eliminate
// hidden polymorphism warning. Should be correct, but is unused and untested.

status_t QueryMenu::SetTargetForItems(BMessenger messenger)
{
	if (messenger.IsTargetLocal())
	{
		BLooper *ignore; // don't care what value this gets
		fTargetHandler = messenger.Target(&ignore);
		return BMenu::SetTargetForItems(messenger);
	}
	return B_ERROR;
}


void QueryMenu::EntryCreated(const entry_ref &ref, ino_t node)
{
	BMessage 		*msg;
	BMenuItem 		*item;
	
	msg = new BMessage(B_REFS_RECEIVED);
	msg->AddRef("refs", &ref);
	msg->AddInt64("node", node);
	item = new BMenuItem(ref.name, msg);
	if (fTargetHandler) {
		item->SetTarget(fTargetHandler);
	};
	AddItem(item);
}


void QueryMenu::EntryRemoved(ino_t node)
{
	// Search for item in menu
	BMenuItem  *item;
	for (int32 i = 0;(item = ItemAt(i)) != NULL;i++)
	{
		// Is it our item?
		int64 inode;
		if ((item->Message())->FindInt64("node", &inode) == B_OK
			&& inode == node)
		{
			RemoveItem(i);
			return;
		}
	}
}


BPoint QueryMenu::ScreenLocation()
{
	if (fPopUp)
		return BPopUpMenu::ScreenLocation();

	return BMenu::ScreenLocation();
}

