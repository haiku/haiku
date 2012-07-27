/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

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

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

//	NodePreloader manages caching up icons from apps and prefs folder for
//	fast display
//

#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Node.h>
#include <NodeMonitor.h>
#include <Path.h>

#include "AutoLock.h"
#include "IconCache.h"
#include "NodePreloader.h"
#include "Thread.h"
#include "Tracker.h"


NodePreloader*
NodePreloader::InstallNodePreloader(const char* name, BLooper* host)
{
	NodePreloader* result = new NodePreloader(name);
	{
		AutoLock<BLooper> lock(host);
		if (!lock)
			return NULL;
		host->AddHandler(result);
	}
	result->Run();
	return result;
}


NodePreloader::NodePreloader(const char* name)
	:	BHandler(name),
		fModelList(20, true),
		fQuitRequested(false)
{
}


NodePreloader::~NodePreloader()
{
	// block deletion while we are locked
	fQuitRequested = true;
	fLock.Lock();
}


void
NodePreloader::Run()
{
	fLock.Lock();
	Thread::Launch(NewMemberFunctionObject(&NodePreloader::Preload, this));
}


Model*
NodePreloader::FindModel(node_ref itemNode) const
{
	for (int32 count = fModelList.CountItems() - 1; count >= 0; count--) {
		Model* model = fModelList.ItemAt(count);
		if (*model->NodeRef() == itemNode)
			return model;
	}
	return NULL;
}


void
NodePreloader::MessageReceived(BMessage* message)
{
	// respond to node monitor notifications
	
	node_ref itemNode;
	switch (message->what) {
		case B_NODE_MONITOR:
		{
			switch (message->FindInt32("opcode")) {
				case B_ENTRY_REMOVED:
				{
					AutoLock<Benaphore> locker(fLock);
					message->FindInt32("device", &itemNode.device);
					message->FindInt64("node", &itemNode.node);
					Model* model = FindModel(itemNode);
					if (!model)
						break;
					//PRINT(("preloader removing file %s\n", model->Name()));
					IconCache::sIconCache->Removing(model);
					fModelList.RemoveItem(model);
					break;
				}

				case B_ATTR_CHANGED:
				case B_STAT_CHANGED:
				{
					AutoLock<Benaphore> locker(fLock);
					message->FindInt32("device", &itemNode.device);
					message->FindInt64("node", &itemNode.node);

					const char* attrName;
					message->FindString("attr", &attrName);
					Model* model = FindModel(itemNode);
					if (!model)
						break;
					BModelOpener opener(model);
					IconCache::sIconCache->IconChanged(model->ResolveIfLink());
					//PRINT(("preloader updating file %s\n", model->Name()));
					break;
				}
			}
			break;
		}

		default:
			_inherited::MessageReceived(message);
			break;
	}
}


void
NodePreloader::PreloadOne(const char* dirPath)
{
	//PRINT(("preloading directory %s\n", dirPath));
	BDirectory dir(dirPath);
	if (!dir.InitCheck() == B_OK)
		return;

	node_ref nodeRef;
	dir.GetNodeRef(&nodeRef);

	// have to node monitor the whole directory
	TTracker::WatchNode(&nodeRef, B_WATCH_DIRECTORY, this);

	dir.Rewind();
	for (;;) {
		entry_ref ref;
		if (dir.GetNextRef(&ref) != B_OK)
			break;
		
		BEntry entry(&ref);
		if (!entry.IsFile())
			// only interrested in files
			continue;

		Model* model = new Model(&ref, true);
		if (model->InitCheck() == B_OK && model->IconFrom() == kUnknownSource) {
			TTracker::WatchNode(model->NodeRef(), B_WATCH_STAT
				| B_WATCH_ATTR, this);
			IconCache::sIconCache->Preload(model, kNormalIcon, B_MINI_ICON, true);
			fModelList.AddItem(model);
			model->CloseNode();
		} else
			delete model;
	}
}


void
NodePreloader::Preload()
{
	for (int32 count = 100; count >= 0; count--) {
		// wait for a little bit before going ahead to reduce disk access contention
		snooze(100000);
		if (fQuitRequested) {
			fLock.Unlock();
			return;
		}
	}

	BMessenger messenger(kTrackerSignature);
	if (!messenger.IsValid()) {
		// put out some message here!
		return;
	}

	ASSERT(fLock.IsLocked());
	BPath path;
	if (find_directory(B_BEOS_APPS_DIRECTORY, &path) == B_OK)
		PreloadOne(path.Path());
	if (find_directory(B_BEOS_PREFERENCES_DIRECTORY, &path) == B_OK)
		PreloadOne(path.Path());

	fLock.Unlock();
}
