/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2025, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


//!	The backbone of the FileCreatedEvent.


#include "FileWatcher.h"

#include <Application.h>
#include <Autolock.h>
#include <Path.h>

#include <PathMonitor.h>

#include <stdio.h>


class Path;


static BLocker sLocker("file watcher");
static FileWatcher* sWatcher;


FileListener::~FileListener()
{
}


// #pragma mark -


FileWatcher::FileWatcher()
	:
	BHandler("file watcher")
{
	if (be_app->Lock()) {
		be_app->AddHandler(this);

		watch_node(NULL, B_WATCH_MOUNT, this);
		be_app->Unlock();
	}
}


FileWatcher::~FileWatcher()
{
	if (be_app->Lock()) {
		stop_watching(this);

		be_app->RemoveHandler(this);
		be_app->Unlock();
	}
}


void
FileWatcher::AddListener(FileListener* listener)
{
	BAutolock lock(sLocker);
	fListeners.AddItem(listener);
}


void
FileWatcher::RemoveListener(FileListener* listener)
{
	BAutolock lock(sLocker);
	fListeners.RemoveItem(listener);
}


int32
FileWatcher::CountListeners() const
{
	BAutolock lock(sLocker);
	return fListeners.CountItems();
}


void
FileWatcher::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_PATH_MONITOR:
		{
			int32 opcode = message->GetInt32("opcode", -1);
			if (opcode == B_ENTRY_CREATED) {
				const char* path;
				const char* watchedPath;
				if (message->FindString("watched_path", &watchedPath) != B_OK
					|| message->FindString("path", &path) != B_OK) {
					break;
				}

				BAutolock lock(sLocker);
				for (int32 i = 0; i < fListeners.CountItems(); i++)
					fListeners.ItemAt(i)->FileCreated(watchedPath);
			}
			break;
		}
	}
}


/*static*/ status_t
FileWatcher::Register(FileListener* listener, BPath& path)
{
	BAutolock lock(sLocker);
	if (sWatcher == NULL)
		sWatcher = new FileWatcher();

	status_t status = BPathMonitor::StartWatching(path.Path(),
			B_WATCH_FILES_ONLY, sWatcher);
	if (status != B_OK)
		return status;
	sWatcher->AddListener(listener);
	return B_OK;
}


/*static*/ void
FileWatcher::Unregister(FileListener* listener, BPath& path)
{
	BAutolock lock(sLocker);
	BPathMonitor::StopWatching(path.Path(), sWatcher);
	sWatcher->RemoveListener(listener);

	if (sWatcher->CountListeners() == 0)
		delete sWatcher;
}
