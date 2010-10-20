/*
 * Copyright 2007-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include <USBKit.h>
#include <Directory.h>
#include <Entry.h>
#include <Looper.h>
#include <Messenger.h>
#include <Node.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <stdio.h>
#include <string.h>
#include <new>


class WatchedEntry {
public:
							WatchedEntry(BUSBRoster *roster,
								BMessenger *messenger, entry_ref *ref);
							~WatchedEntry();

		bool				EntryCreated(entry_ref *ref);
		bool				EntryRemoved(ino_t node);

private:
		BUSBRoster *		fRoster;
		BMessenger *		fMessenger;

		node_ref			fNode;
		bool				fIsDirectory;
		BUSBDevice *		fDevice;

		WatchedEntry *		fEntries;
		WatchedEntry *		fLink;
};


class RosterLooper : public BLooper {
public:
							RosterLooper(BUSBRoster *roster);

		void				Stop();

virtual	void				MessageReceived(BMessage *message);

private:
		BUSBRoster *		fRoster;
		WatchedEntry *		fRoot;
		BMessenger *		fMessenger;
};


WatchedEntry::WatchedEntry(BUSBRoster *roster, BMessenger *messenger,
	entry_ref *ref)
	:	fRoster(roster),
		fMessenger(messenger),
		fIsDirectory(false),
		fDevice(NULL),
		fEntries(NULL),
		fLink(NULL)
{
	BEntry entry(ref);
	entry.GetNodeRef(&fNode);

	BDirectory directory;
	if (entry.IsDirectory() && directory.SetTo(ref) >= B_OK) {
		fIsDirectory = true;

		while(directory.GetNextEntry(&entry) >= B_OK) {
			if (entry.GetRef(ref) < B_OK)
				continue;

			WatchedEntry *child = new(std::nothrow) WatchedEntry(fRoster,
				fMessenger, ref);
			if (child == NULL)
				continue;

			child->fLink = fEntries;
			fEntries = child;
		}

		watch_node(&fNode, B_WATCH_DIRECTORY, *fMessenger);
	} else {
		// filter pseudoentry that only handles ioctls
		if (strncmp(ref->name, "raw", 3) == 0)
			return;

		BPath path;
		entry.GetPath(&path);
		fDevice = new(std::nothrow) BUSBDevice(path.Path());
		if (fDevice != NULL) {
			if (fRoster->DeviceAdded(fDevice) != B_OK) {
				delete fDevice;
				fDevice = NULL;
			}
		}
	}
}


WatchedEntry::~WatchedEntry()
{
	if (fIsDirectory) {
		watch_node(&fNode, B_STOP_WATCHING, *fMessenger);

		WatchedEntry *child = fEntries;
		while (child) {
			WatchedEntry *next = child->fLink;
			delete child;
			child = next;
		}
	}

	if (fDevice) {
		fRoster->DeviceRemoved(fDevice);
		delete fDevice;
	}
}


bool
WatchedEntry::EntryCreated(entry_ref *ref)
{
	if (!fIsDirectory)
		return false;

	if (ref->directory != fNode.node) {
		WatchedEntry *child = fEntries;
		while (child) {
			if (child->EntryCreated(ref))
				return true;
			child = child->fLink;
		}

		return false;
	}

	WatchedEntry *child = new(std::nothrow) WatchedEntry(fRoster, fMessenger,
		ref);
	if (child == NULL)
		return false;

	child->fLink = fEntries;
	fEntries = child;
	return true;
}


bool
WatchedEntry::EntryRemoved(ino_t node)
{
	if (!fIsDirectory)
		return false;

	WatchedEntry *child = fEntries;
	WatchedEntry *lastChild = NULL;
	while (child) {
		if (child->fNode.node == node) {
			if (lastChild)
				lastChild->fLink = child->fLink;
			else
				fEntries = child->fLink;

			delete child;
			return true;
		}

		if (child->EntryRemoved(node))
			return true;

		lastChild = child;
		child = child->fLink;
	}

	return false;
}


RosterLooper::RosterLooper(BUSBRoster *roster)
	:	BLooper("BUSBRoster looper"),
		fRoster(roster),
		fRoot(NULL),
		fMessenger(NULL)
{
	BEntry entry("/dev/bus/usb");
	if (!entry.Exists()) {
		fprintf(stderr, "USBKit: usb_raw not published\n");
		return;
	}

	Run();
	fMessenger = new(std::nothrow) BMessenger(this);
	if (fMessenger == NULL)
		return;

	if (Lock()) {
		entry_ref ref;
		entry.GetRef(&ref);
		fRoot = new(std::nothrow) WatchedEntry(fRoster, fMessenger, &ref);
		Unlock();
	}
}


void
RosterLooper::Stop()
{
	Lock();
	delete fRoot;
	Quit();
}


void
RosterLooper::MessageReceived(BMessage *message)
{
	int32 opcode;
	if (message->FindInt32("opcode", &opcode) < B_OK)
		return;

	switch (opcode) {
		case B_ENTRY_CREATED: {
			dev_t device;
			ino_t directory;
			const char *name;
			if (message->FindInt32("device", &device) < B_OK
				|| message->FindInt64("directory", &directory) < B_OK
				|| message->FindString("name", &name) < B_OK)
				break;

			entry_ref ref(device, directory, name);
			fRoot->EntryCreated(&ref);
			break;
		}

		case B_ENTRY_REMOVED: {
			ino_t node;
			if (message->FindInt64("node", &node) < B_OK)
				break;

			fRoot->EntryRemoved(node);
			break;
		}
	}
}


BUSBRoster::BUSBRoster()
	:	fLooper(NULL)
{
}


BUSBRoster::~BUSBRoster()
{
	Stop();
}


void
BUSBRoster::Start()
{
	if (fLooper)
		return;

	fLooper = new(std::nothrow) RosterLooper(this);
}


void
BUSBRoster::Stop()
{
	if (!fLooper)
		return;

	((RosterLooper *)fLooper)->Stop();
	fLooper = NULL;
}


// definition of reserved virtual functions
void BUSBRoster::_ReservedUSBRoster1() {};
void BUSBRoster::_ReservedUSBRoster2() {};
void BUSBRoster::_ReservedUSBRoster3() {};
void BUSBRoster::_ReservedUSBRoster4() {};
void BUSBRoster::_ReservedUSBRoster5() {};
