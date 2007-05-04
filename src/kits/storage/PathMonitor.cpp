/*
 * Copyright 2007, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <PathMonitor.h>

#include <Application.h>
#include <Directory.h>
#include <Entry.h>
#include <Handler.h>
#include <Looper.h>
#include <Path.h>

#include <set>

using namespace BPrivate;
using namespace std;


#define WATCH_NODE_FLAG_MASK	0x00ff

typedef set<node_ref> DirectorySet;

namespace BPrivate {

class PathHandler : public BHandler {
	public:
		PathHandler(const char* path, uint32 flags, BMessenger target);
		virtual ~PathHandler();

		status_t InitCheck() const;
		void SetTarget(BMessenger target);
		void Quit();

		virtual void MessageReceived(BMessage* message);

	private:
		bool _IsContained(const node_ref& nodeRef) const;
		bool _IsContained(BEntry& entry) const;
		bool _HasDirectory(const node_ref& nodeRef) const;
		void _NotifyTarget(BMessage* message) const;
		status_t _AddDirectory(BEntry& entry);
		status_t _RemoveDirectory(const node_ref& nodeRef);
		status_t _RemoveDirectory(BEntry& entry);

		BPath			fPath;
		int32			fPathLength;
		BMessenger		fTarget;
		uint32			fFlags;
		status_t		fStatus;
		bool			fOwnsLooper;
		DirectorySet	fDirectories;
};

}


static status_t
set_entry(node_ref& nodeRef, const char* name, BEntry& entry)
{
	entry_ref ref;
	ref.device = nodeRef.device;
	ref.directory = nodeRef.node;

	status_t status = ref.set_name(name);
	if (status != B_OK)
		return status;

	return entry.SetTo(&ref, true);
}


bool
operator<(const node_ref& a, const node_ref& b)
{
	if (a.device < b.device)
		return true;
	if (a.device == b.device && a.node < b.node)
		return true;

	return false;
}


//	#pragma mark -


PathHandler::PathHandler(const char* path, uint32 flags, BMessenger target)
	: BHandler(path),
	fPath(path, NULL, true),
	fTarget(target),
	fFlags(flags),
	fOwnsLooper(false)
{
	fPathLength = strlen(fPath.Path());

	BPath first(path);
	node_ref nodeRef;

	while (true) {
		// try to find the first part of the path that exists
		BDirectory directory;
		fStatus = directory.SetTo(first.Path());
		if (fStatus == B_OK) {
			fStatus = directory.GetNodeRef(&nodeRef);
			if (fStatus == B_OK)
				break;
		}

		if (first.GetParent(&first) != B_OK) {
			fStatus = B_ERROR;
			break;
		}
	}

	if (fStatus < B_OK)
		return;

	if (be_app != NULL) {
		be_app->AddHandler(this);
	} else {
		// TODO: only have a single global looper!
		BLooper* looper = new BLooper("PathMonitor looper");
		looper->Run();
		looper->AddHandler(this);
		fOwnsLooper = true;
	}

	fStatus = watch_node(&nodeRef, flags & WATCH_NODE_FLAG_MASK, this);
}


PathHandler::~PathHandler()
{
}


status_t
PathHandler::InitCheck() const
{
	return fStatus;
}


void
PathHandler::SetTarget(BMessenger target)
{
	fTarget = target;
}


void
PathHandler::Quit()
{
	BMessenger me(this);
	me.SendMessage(B_QUIT_REQUESTED);
}


void
PathHandler::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
		{
			int32 opcode;
			if (message->FindInt32("opcode", &opcode) != B_OK)
				return;

			switch (opcode) {
				case B_ENTRY_CREATED:
				{
					const char* name;
					node_ref nodeRef;
					if (message->FindInt32("device", &nodeRef.device) != B_OK
						|| message->FindInt64("directory", &nodeRef.node) != B_OK
						|| message->FindString("name", &name) != B_OK)
						break;

					BEntry entry;
					if (set_entry(nodeRef, name, entry) != B_OK)
						break;

					bool notify = true;

					if (entry.IsDirectory()) {
						// a new directory to watch for us
						if (_AddDirectory(entry) != B_OK
							|| (fFlags & B_WATCH_FILES_ONLY) != 0)
							notify = false;
					}

					if (notify && _IsContained(entry)) {
						message->AddBool("added", true);
						_NotifyTarget(message);
					}
					break;
				}

				case B_ENTRY_MOVED:
				{
					// has the entry been moved into a monitored directory or has
					// it been removed from one?
					const char* name;
					node_ref nodeRef;
					uint64 fromNode;
					uint64 node;
					if (message->FindInt32("device", &nodeRef.device) != B_OK
						|| message->FindInt64("to directory", &nodeRef.node) != B_OK
						|| message->FindInt64("from directory", (int64 *)&fromNode) != B_OK
						|| message->FindInt64("node", (int64 *)&node) != B_OK
						|| message->FindString("name", &name) != B_OK)
						break;

					BEntry entry;
					if (set_entry(nodeRef, name, entry) != B_OK)
						break;

					bool wasAdded = false;
					bool wasRemoved = false;
					bool notify = true;

					if (_HasDirectory(nodeRef)) {
						// something has been added to our watched directories

						// test, if the source directory is one of ours as well
						nodeRef.node = fromNode;
						bool hasFromDirectory = _HasDirectory(nodeRef);

						if (entry.IsDirectory()) {
							if (!hasFromDirectory) {
								// there is a new directory to watch for us
								_AddDirectory(entry);
								wasAdded = true;
							}
							if ((fFlags & B_WATCH_FILES_ONLY) != 0)
								notify = false;
						} else  if (!hasFromDirectory) {
							// file has been added
							wasAdded = true;
						}
					} else {
						// and entry has been removed from our directories
						wasRemoved = true;

						if (entry.IsDirectory()) {
							_RemoveDirectory(entry);
							if ((fFlags & B_WATCH_FILES_ONLY) != 0)
								notify = false;
						}
					}

					if (notify && _IsContained(entry)) {					
						if (wasAdded)
							message->AddBool("added", true);
						if (wasRemoved)
							message->AddBool("removed", true);

						_NotifyTarget(message);
					}
					break;
				}

				case B_ENTRY_REMOVED:
				{
					node_ref nodeRef;
					uint64 directoryNode;
					if (message->FindInt32("device", &nodeRef.device) != B_OK
						|| message->FindInt64("directory", (int64 *)&directoryNode) != B_OK
						|| message->FindInt64("node", &nodeRef.node) != B_OK)
						break;

					bool notify = true;

					if (_HasDirectory(nodeRef)) {
						// the directory has been removed, so we remove it as well
						_RemoveDirectory(nodeRef);
						if ((fFlags & B_WATCH_FILES_ONLY) != 0)
							notify = false;
					}

					nodeRef.node = directoryNode;
					if (notify && _IsContained(nodeRef)) {
						message->AddBool("removed", true);
						_NotifyTarget(message);
					}
					break;
				}

				default:
					_NotifyTarget(message);
					break;
			}
			break;
		}

		case B_QUIT_REQUESTED:
		{
			BLooper* looper = Looper();

			stop_watching(this);
			looper->RemoveHandler(this);
			delete this;

			if (fOwnsLooper)
				looper->Quit();
			return;
		}

		default:
			BHandler::MessageReceived(message);
			break;
	}
}


bool
PathHandler::_IsContained(const node_ref& nodeRef) const
{
	BDirectory directory(&nodeRef);
	if (directory.InitCheck() != B_OK)
		return false;

	BEntry entry;
	if (directory.GetEntry(&entry) != B_OK)
		return false;

	return _IsContained(entry);
}


bool
PathHandler::_IsContained(BEntry& entry) const
{
	BPath path;
	if (entry.GetPath(&path) != B_OK)
		return false;

	return strncmp(path.Path(), fPath.Path(), fPathLength) == 0;
}


bool
PathHandler::_HasDirectory(const node_ref& nodeRef) const
{
	DirectorySet::const_iterator iterator = fDirectories.find(nodeRef);
	return iterator != fDirectories.end();
}


void
PathHandler::_NotifyTarget(BMessage* message) const
{
	BMessage update(*message);
	update.what = B_PATH_MONITOR;
	fTarget.SendMessage(&update);
}


status_t
PathHandler::_AddDirectory(BEntry& entry)
{
	node_ref nodeRef;
	status_t status = entry.GetNodeRef(&nodeRef);
	if (status != B_OK)
		return status;

	// check if we are already know this directory

	if (_HasDirectory(nodeRef))
		return B_OK;

	uint32 flags;
	if (_IsContained(entry))
		flags = fFlags & WATCH_NODE_FLAG_MASK;
	else
		flags = B_WATCH_DIRECTORY;

	status = watch_node(&nodeRef, flags, this);
	if (status != B_OK)
		return status;

	fDirectories.insert(nodeRef);

#if 0
	BEntry parent;
	if (entry.GetParent(&parent) == B_OK
		&& !_IsContained(parent)) {
		// TODO: remove parent from watched directories
	}
#endif
	return B_OK;
}


status_t
PathHandler::_RemoveDirectory(const node_ref& nodeRef)
{
	DirectorySet::iterator iterator = fDirectories.find(nodeRef);
	if (iterator == fDirectories.end())
		return B_ENTRY_NOT_FOUND;

	fDirectories.erase(iterator);
	return B_OK;
}


status_t
PathHandler::_RemoveDirectory(BEntry& entry)
{
	node_ref nodeRef;
	status_t status = entry.GetNodeRef(&nodeRef);
	if (status != B_OK)
		return status;

	return _RemoveDirectory(nodeRef);
}


//	#pragma mark -


BPathMonitor::BPathMonitor()
	:
	fHandler(NULL),
	fStatus(B_NO_INIT)
{
}


BPathMonitor::BPathMonitor(const char* path, uint32 flags, BMessenger target)
	:
	fHandler(NULL),
	fStatus(B_NO_INIT)
{
	SetTo(path, flags, target);
}


BPathMonitor::~BPathMonitor()
{
	Unset();
}


status_t
BPathMonitor::InitCheck() const
{
	return fStatus;
}


status_t
BPathMonitor::SetTo(const char* path, uint32 flags, BMessenger target)
{
	Unset();

	fHandler = new PathHandler(path, flags, target);
	status_t status = fHandler->InitCheck();
	if (status < B_OK)
		Unset();

	return fStatus = status;
}


status_t
BPathMonitor::SetTarget(BMessenger target)
{
	if (fStatus < B_OK)
		return B_NO_INIT;

	fHandler->SetTarget(target);
	return B_OK;
}


void
BPathMonitor::Unset()
{
	if (fHandler != NULL) {
		fHandler->Quit();
		fHandler = NULL;
	}
	fStatus = B_NO_INIT;
}

//}	// namespace BPrivate
