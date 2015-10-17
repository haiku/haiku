/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


//!	The backbone of the VolumeMountedEvent.


#include "VolumeWatcher.h"

#include <Application.h>
#include <Autolock.h>
#include <NodeMonitor.h>


static BLocker sLocker("volume watcher");
static VolumeWatcher* sWatcher;


VolumeListener::~VolumeListener()
{
}


// #pragma mark -


VolumeWatcher::VolumeWatcher()
	:
	BHandler("volume watcher")
{
	if (be_app->Lock()) {
		be_app->AddHandler(this);

		watch_node(NULL, B_WATCH_MOUNT, this);
		be_app->Unlock();
	}
}


VolumeWatcher::~VolumeWatcher()
{
	if (be_app->Lock()) {
		stop_watching(this);

		be_app->RemoveHandler(this);
		be_app->Unlock();
	}
}


void
VolumeWatcher::AddListener(VolumeListener* listener)
{
	BAutolock lock(sLocker);
	fListeners.AddItem(listener);
}


void
VolumeWatcher::RemoveListener(VolumeListener* listener)
{
	BAutolock lock(sLocker);
	fListeners.RemoveItem(listener);
}


int32
VolumeWatcher::CountListeners() const
{
	BAutolock lock(sLocker);
	return fListeners.CountItems();
}


void
VolumeWatcher::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
		{
			int32 opcode = message->GetInt32("opcode", -1);
			if (opcode == B_DEVICE_MOUNTED) {
				dev_t device;
				if (message->FindInt32("new device", &device) == B_OK) {
					BAutolock lock(sLocker);
					for (int32 i = 0; i < fListeners.CountItems(); i++) {
						fListeners.ItemAt(i)->VolumeMounted(device);
					}
				}
			} else if (opcode == B_DEVICE_UNMOUNTED) {
				dev_t device;
				if (message->FindInt32("device", &device) == B_OK) {
					BAutolock lock(sLocker);
					for (int32 i = 0; i < fListeners.CountItems(); i++) {
						fListeners.ItemAt(i)->VolumeUnmounted(device);
					}
				}
			}
			break;
		}
	}
}


/*static*/ void
VolumeWatcher::Register(VolumeListener* listener)
{
	BAutolock lock(sLocker);
	if (sWatcher == NULL)
		sWatcher = new VolumeWatcher();

	sWatcher->AddListener(listener);
}


/*static*/ void
VolumeWatcher::Unregister(VolumeListener* listener)
{
	BAutolock lock(sLocker);
	sWatcher->RemoveListener(listener);

	if (sWatcher->CountListeners() == 0)
		delete sWatcher;
}
