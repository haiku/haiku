/*
 * Copyright 2007, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <PathMonitor.h>

#include <ObjectList.h>

#include <Application.h>
#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <Handler.h>
#include <Locker.h>
#include <Looper.h>
#include <Path.h>
#include <String.h>

#include <map>
#include <set>

#undef TRACE
//#define TRACE_PATH_MONITOR
#ifdef TRACE_PATH_MONITOR
#	define TRACE(x) printf x
#else
#	define TRACE(x) ;
#endif

using namespace BPrivate;
using namespace std;


#define WATCH_NODE_FLAG_MASK	0x00ff

namespace BPrivate {

#if __GNUC__ > 3
	bool operator<(const node_ref& a, const node_ref& b);
	class node_ref_less : public binary_function<node_ref, node_ref, bool> 
	{
		public:
		bool operator() (const node_ref& a, const node_ref& b) const
		{
			return a < b;
		}
	};
	typedef set<node_ref,node_ref_less> FileSet;
#else
	typedef set<node_ref> FileSet;
#endif

struct watched_directory {
	node_ref	node;
	bool		contained;
};
typedef set<watched_directory> DirectorySet;


class PathHandler;
typedef map<BString, PathHandler*> HandlerMap;

struct watcher {
	HandlerMap handlers;
};
typedef map<BMessenger, watcher*> WatcherMap;

class PathHandler : public BHandler {
	public:
		PathHandler(const char* path, uint32 flags, BMessenger target);
		virtual ~PathHandler();

		status_t InitCheck() const;
		void SetTarget(BMessenger target);
		void Quit();

		virtual void MessageReceived(BMessage* message);
#ifdef TRACE_PATH_MONITOR
		void Dump();
#endif

	private:
		status_t _GetClosest(const char* path, bool updatePath,
			node_ref& nodeRef);
		bool _WatchRecursively();
		bool _WatchFilesOnly();
		void _EntryCreated(BMessage* message);
		void _EntryRemoved(BMessage* message);
		void _EntryMoved(BMessage* message);
		bool _IsContained(const node_ref& nodeRef) const;
		bool _IsContained(BEntry& entry) const;
		bool _HasDirectory(const node_ref& nodeRef, bool* _contained = NULL) const;
		bool _CloserToPath(BEntry& entry) const;
		void _NotifyTarget(BMessage* message) const;
		status_t _AddDirectory(BEntry& entry);
		status_t _AddDirectory(node_ref& nodeRef);
		status_t _RemoveDirectory(const node_ref& nodeRef, ino_t directoryNode);
		status_t _RemoveDirectory(BEntry& entry, ino_t directoryNode);

		bool _HasFile(const node_ref& nodeRef) const;
		status_t _AddFile(BEntry& entry);
		status_t _RemoveFile(const node_ref& nodeRef);
		status_t _RemoveFile(BEntry& entry);

		BPath			fPath;
		int32			fPathLength;
		BMessenger		fTarget;
		uint32			fFlags;
		status_t		fStatus;
		bool			fOwnsLooper;
		DirectorySet	fDirectories;
		FileSet			fFiles;
};


static WatcherMap sWatchers;
static BLocker* sLocker;


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


bool
operator<(const watched_directory& a, const watched_directory& b)
{
	return a.node < b.node;
}


//	#pragma mark -


PathHandler::PathHandler(const char* path, uint32 flags, BMessenger target)
	: BHandler(path),
	fTarget(target),
	fFlags(flags),
	fOwnsLooper(false)
{
	if (path == NULL || !path[0]) {
		fStatus = B_BAD_VALUE;
		return;
	}

	// TODO: support watching not-yet-mounted volumes as well!
	node_ref nodeRef;
	fStatus = _GetClosest(path, true, nodeRef);
	if (fStatus < B_OK)
		return;

	BLooper* looper;
	if (be_app != NULL) {
		looper = be_app;
	} else {
		// TODO: only have a single global looper!
		looper = new BLooper("PathMonitor looper");
		looper->Run();
		fOwnsLooper = true;
	}

	looper->Lock();
	looper->AddHandler(this);
	looper->Unlock();

	fStatus = _AddDirectory(nodeRef);

	// TODO: work-around for existing files (should not watch the directory in this case)
	BEntry entry(path);
	if (entry.Exists() && !entry.IsDirectory())
		_AddFile(entry);
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
PathHandler::Quit()
{
	BMessenger me(this);
	me.SendMessage(B_QUIT_REQUESTED);
}


#ifdef TRACE_PATH_MONITOR
void
PathHandler::Dump()
{
	printf("WATCHING DIRECTORIES:\n");
	DirectorySet::iterator i = fDirectories.begin();
	for (; i != fDirectories.end(); i++) {
		printf("  %ld:%Ld (%s)\n", i->node.device, i->node.node, i->contained ? "contained" : "-");
	}

	printf("WATCHING FILES:\n");

	FileSet::iterator j = fFiles.begin();
	for (; j != fFiles.end(); j++) {
		printf("  %ld:%Ld\n", j->device, j->node);
	}
}
#endif


status_t
PathHandler::_GetClosest(const char* path, bool updatePath, node_ref& nodeRef)
{
	BPath first(path);
	BString missing;

	while (true) {
		// try to find the first part of the path that exists
		BDirectory directory;
		status_t status = directory.SetTo(first.Path());
		if (status == B_OK) {
			status = directory.GetNodeRef(&nodeRef);
			if (status == B_OK) {
				if (updatePath) {
					// normalize path
					status = fPath.SetTo(&directory, NULL, true);
					if (status == B_OK) {
						fPath.Append(missing.String());
						fPathLength = strlen(fPath.Path());
					}
				}
				return status;
			}
		}

		if (updatePath) {
			if (missing.Length() > 0)
				missing.Prepend("/");
			missing.Prepend(first.Leaf());
		}

		if (first.GetParent(&first) != B_OK)
			return B_ERROR;
	}
}


bool
PathHandler::_WatchRecursively()
{
	return (fFlags & B_WATCH_RECURSIVELY) != 0;
}


bool
PathHandler::_WatchFilesOnly()
{
	return (fFlags & B_WATCH_FILES_ONLY) != 0;
}


void
PathHandler::_EntryCreated(BMessage* message)
{
	const char* name;
	node_ref nodeRef;
	if (message->FindInt32("device", &nodeRef.device) != B_OK
		|| message->FindInt64("directory", &nodeRef.node) != B_OK
		|| message->FindString("name", &name) != B_OK)
		return;

	BEntry entry;
	if (set_entry(nodeRef, name, entry) != B_OK)
		return;

	bool parentContained = false;
	bool entryContained = _IsContained(entry);
	if (entryContained)
		parentContained = _IsContained(nodeRef);
	bool notify = entryContained;

	if (entry.IsDirectory()) {
		// ignore the directory if it's already known
		if (entry.GetNodeRef(&nodeRef) == B_OK
			&& _HasDirectory(nodeRef)) {
			TRACE(("    WE ALREADY HAVE DIR %s, %ld:%Ld\n",
				name, nodeRef.device, nodeRef.node));
			return;
		}

		// a new directory to watch for us
		if (!entryContained && !_CloserToPath(entry)
			|| parentContained && !_WatchRecursively()
			|| _AddDirectory(entry) != B_OK
			|| _WatchFilesOnly())
			notify = parentContained;
	} else if (entryContained) {
		TRACE(("  NEW ENTRY PARENT CONTAINED: %d\n", parentContained));
		_AddFile(entry);
	}

	if (notify && entryContained) {
		message->AddBool("added", true);
		_NotifyTarget(message);
	}
}


void
PathHandler::_EntryRemoved(BMessage* message)
{
	node_ref nodeRef;
	uint64 directoryNode;
	if (message->FindInt32("device", &nodeRef.device) != B_OK
		|| message->FindInt64("directory", (int64 *)&directoryNode) != B_OK
		|| message->FindInt64("node", &nodeRef.node) != B_OK)
		return;

	bool notify = false;

	if (_HasDirectory(nodeRef, &notify)) {
		// the directory has been removed, so we remove it as well
		_RemoveDirectory(nodeRef, directoryNode);
		if (_WatchFilesOnly())
			notify = false;
	} else if (_HasFile(nodeRef)) {
		_RemoveFile(nodeRef);
		notify = true;
	}

	if (notify) {
		message->AddBool("removed", true);
		_NotifyTarget(message);
	}
}


void
PathHandler::_EntryMoved(BMessage* message)
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
		return;

	BEntry entry;
	if (set_entry(nodeRef, name, entry) != B_OK)
		return;

	bool entryContained = _IsContained(entry);
	bool wasAdded = false;
	bool wasRemoved = false;
	bool notify = false;

	bool parentContained;
	if (_HasDirectory(nodeRef, &parentContained)) {
		// something has been added to our watched directories

		nodeRef.node = node;
		TRACE(("    ADDED TO PARENT (%d), has entry %d/%d, entry %d %d\n",
			parentContained, _HasDirectory(nodeRef), _HasFile(nodeRef),
			entryContained, _CloserToPath(entry)));

		if (entry.IsDirectory()) {
			if (!_HasDirectory(nodeRef)
				&& (entryContained || _CloserToPath(entry))) {
				// there is a new directory to watch for us
				if (entryContained
					|| parentContained && !_WatchRecursively())
					_AddDirectory(entry);
				else if (_GetClosest(fPath.Path(), false,
						nodeRef) == B_OK) {
					// the new directory might put us even
					// closer to the path we are after
					_AddDirectory(nodeRef);
				}

				wasAdded = true;
				notify = entryContained;
			}
			if (_WatchFilesOnly())
				notify = false;
		} else if (!_HasFile(nodeRef) && entryContained) {
			// file has been added
			wasAdded = true;
			notify = true;
			_AddFile(entry);
		}
	} else {
		// and entry has been removed from our directories
		wasRemoved = true;

		nodeRef.node = node;
		if (entry.IsDirectory()) {
			if (_HasDirectory(nodeRef, &notify))
				_RemoveDirectory(entry, fromNode);
			if (_WatchFilesOnly())
				notify = false;
		} else {
			_RemoveFile(entry);
			notify = true;
		}
	}

	if (notify) {
		if (wasAdded)
			message->AddBool("added", true);
		if (wasRemoved)
			message->AddBool("removed", true);

		_NotifyTarget(message);
	}
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
					_EntryCreated(message);
					break;

				case B_ENTRY_REMOVED:
					_EntryRemoved(message);
					break;

				case B_ENTRY_MOVED:
					_EntryMoved(message);
					break;

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

#ifdef TRACE_PATH_MONITOR
	Dump();
#endif
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

	bool contained = strncmp(path.Path(), fPath.Path(), fPathLength) == 0;
	if (!contained)
		return false;
	
	const char* last = &path.Path()[fPathLength];
	if (last[0] && last[0] != '/')
		return false;

	return true;
}


bool
PathHandler::_HasDirectory(const node_ref& nodeRef, bool* _contained /* = NULL */) const
{
	watched_directory directory;
	directory.node = nodeRef;

	DirectorySet::const_iterator iterator = fDirectories.find(directory);
	if (iterator == fDirectories.end())
		return false;

	if (_contained != NULL)
		*_contained = iterator->contained;
	return true;
}


bool
PathHandler::_CloserToPath(BEntry& entry) const
{
	BPath path;
	if (entry.GetPath(&path) != B_OK)
		return false;

	return strncmp(path.Path(), fPath.Path(), strlen(path.Path())) == 0;
}


void
PathHandler::_NotifyTarget(BMessage* message) const
{
	BMessage update(*message);
	update.what = B_PATH_MONITOR;
	update.AddString("path", fPath.Path());

	fTarget.SendMessage(&update);
}


status_t
PathHandler::_AddDirectory(BEntry& entry)
{
	watched_directory directory;
	status_t status = entry.GetNodeRef(&directory.node);
	if (status != B_OK)
		return status;

#ifdef TRACE_PATH_MONITOR
{
	BPath path(&entry);
	printf("  ADD DIRECTORY %s, %ld:%Ld\n",
		path.Path(), directory.node.device, directory.node.node);
}
#endif

	// check if we are already know this directory

	if (_HasDirectory(directory.node))
		return B_OK;

	directory.contained = _IsContained(entry);

	uint32 flags;
	if (directory.contained)
		flags = (fFlags & WATCH_NODE_FLAG_MASK) | B_WATCH_DIRECTORY;
	else
		flags = B_WATCH_DIRECTORY;

	status = watch_node(&directory.node, flags, this);
	if (status != B_OK)
		return status;

	fDirectories.insert(directory);

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
PathHandler::_AddDirectory(node_ref& nodeRef)
{
	BDirectory directory(&nodeRef);
	status_t status = directory.InitCheck();
	if (status == B_OK) {
		BEntry entry;
		status = directory.GetEntry(&entry);
		if (status == B_OK)
			status = _AddDirectory(entry);
	}

	return status;
}


status_t
PathHandler::_RemoveDirectory(const node_ref& nodeRef, ino_t directoryNode)
{
	TRACE(("  REMOVE DIRECTORY %ld:%Ld\n", nodeRef.device, nodeRef.node));

	watched_directory directory;
	directory.node = nodeRef;

	DirectorySet::iterator iterator = fDirectories.find(directory);
	if (iterator == fDirectories.end())
		return B_ENTRY_NOT_FOUND;

	watch_node(&directory.node, B_STOP_WATCHING, this);

	node_ref directoryRef;
	directoryRef.device = nodeRef.device;
	directoryRef.node = directoryNode;

	if (!_HasDirectory(directoryRef)) {
		// we don't have the parent directory now, but we'll need it in order
		// to find this directory again in case it's added again
		if (_AddDirectory(directoryRef) != B_OK
			&& _GetClosest(fPath.Path(), false, directoryRef) == B_OK)
			_AddDirectory(directoryRef);
	}

	fDirectories.erase(iterator);
	return B_OK;
}


status_t
PathHandler::_RemoveDirectory(BEntry& entry, ino_t directoryNode)
{
	node_ref nodeRef;
	status_t status = entry.GetNodeRef(&nodeRef);
	if (status != B_OK)
		return status;

	return _RemoveDirectory(nodeRef, directoryNode);
}


bool
PathHandler::_HasFile(const node_ref& nodeRef) const
{
	FileSet::const_iterator iterator = fFiles.find(nodeRef);
	return iterator != fFiles.end();
}


status_t
PathHandler::_AddFile(BEntry& entry)
{
	if ((fFlags & (WATCH_NODE_FLAG_MASK & ~B_WATCH_DIRECTORY)) == 0)
		return B_OK;

#ifdef TRACE_PATH_MONITOR
{
	BPath path(&entry);
	printf("  ADD FILE %s\n", path.Path());
}
#endif

	node_ref nodeRef;
	status_t status = entry.GetNodeRef(&nodeRef);
	if (status != B_OK)
		return status;

	// check if we are already know this file

	if (_HasFile(nodeRef))
		return B_OK;

	status = watch_node(&nodeRef, (fFlags & WATCH_NODE_FLAG_MASK), this);
	if (status != B_OK)
		return status;

	fFiles.insert(nodeRef);
	return B_OK;
}


status_t
PathHandler::_RemoveFile(const node_ref& nodeRef)
{
	TRACE(("  REMOVE FILE %ld:%Ld\n", nodeRef.device, nodeRef.node));

	FileSet::iterator iterator = fFiles.find(nodeRef);
	if (iterator == fFiles.end())
		return B_ENTRY_NOT_FOUND;

	watch_node(&nodeRef, B_STOP_WATCHING, this);
	fFiles.erase(iterator);
	return B_OK;
}


status_t
PathHandler::_RemoveFile(BEntry& entry)
{
	node_ref nodeRef;
	status_t status = entry.GetNodeRef(&nodeRef);
	if (status != B_OK)
		return status;

	return _RemoveFile(nodeRef);
}


//	#pragma mark -


BPathMonitor::BPathMonitor()
{
}


BPathMonitor::~BPathMonitor()
{
}


/*static*/ status_t
BPathMonitor::_InitIfNeeded()
{
	static vint32 lock = 0;
	
	if (sLocker != NULL)
		return B_OK;

	while (sLocker == NULL) {
		if (atomic_add(&lock, 1) == 0) {
			sLocker = new BLocker("path monitor");
		}
		snooze(5000);
	}

	return B_OK;
}


/*static*/ status_t
BPathMonitor::StartWatching(const char* path, uint32 flags, BMessenger target)
{
	status_t status = _InitIfNeeded();
	if (status != B_OK)
		return status;

	BAutolock _(sLocker);

	WatcherMap::iterator iterator = sWatchers.find(target);
	struct watcher* watcher = NULL;
	if (iterator != sWatchers.end())
		watcher = iterator->second;

	PathHandler* handler = new PathHandler(path, flags, target);
	status = handler->InitCheck();
	if (status < B_OK)
		return status;

	if (watcher == NULL) {
		watcher = new BPrivate::watcher;
		sWatchers[target] = watcher;
	}

	watcher->handlers[path] = handler;
	return B_OK;
}


/*static*/ status_t
BPathMonitor::StopWatching(const char* path, BMessenger target)
{
	BAutolock _(sLocker);

	WatcherMap::iterator iterator = sWatchers.find(target);
	if (iterator == sWatchers.end())
		return B_BAD_VALUE;

	struct watcher* watcher = iterator->second;
	HandlerMap::iterator i = watcher->handlers.find(path);

	if (i == watcher->handlers.end())
		return B_BAD_VALUE;

	PathHandler* handler = i->second;
	watcher->handlers.erase(i);

	if (handler->LockLooper())
		handler->Quit();

	if (watcher->handlers.empty()) {
		sWatchers.erase(iterator);
		delete watcher;
	}

	return B_OK;
}


/*static*/ status_t
BPathMonitor::StopWatching(BMessenger target)
{
	BAutolock _(sLocker);

	WatcherMap::iterator iterator = sWatchers.find(target);
	if (iterator == sWatchers.end())
		return B_BAD_VALUE;

	struct watcher* watcher = iterator->second;
	while (!watcher->handlers.empty()) {
		HandlerMap::iterator i = watcher->handlers.begin();
		PathHandler* handler = i->second;
		watcher->handlers.erase(i);

		if (handler->LockLooper())
			handler->Quit();
	}

	sWatchers.erase(iterator);
	delete watcher;

	return B_OK;
}

}	// namespace BPrivate
