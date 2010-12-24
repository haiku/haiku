/*
 * Copyright 2007-2008, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include <PathMonitor.h>

#include <ObjectList.h>

#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <Handler.h>
#include <Locker.h>
#include <Looper.h>
#include <Path.h>
#include <String.h>

#include <map>
#include <new>
#include <set>
#include <stdio.h>

#undef TRACE
//#define TRACE_PATH_MONITOR
#ifdef TRACE_PATH_MONITOR
#	define TRACE(x...) debug_printf(x)
#else
#	define TRACE(x...) ;
#endif

using namespace BPrivate;
using namespace std;
using std::nothrow; // TODO: Remove this line if the above line is enough.

// TODO: Use optimizations where stuff is already known to avoid iterating
// the watched directory and files set too often.

#define WATCH_NODE_FLAG_MASK	0x00ff

namespace BPrivate {

struct FileEntry {
	entry_ref	ref;
	ino_t		node;
};

#if __GNUC__ > 3
	bool operator<(const FileEntry& a, const FileEntry& b);
	class FileEntryLess : public binary_function<FileEntry, FileEntry, bool>
	{
		public:
		bool operator() (const FileEntry& a, const FileEntry& b) const
		{
			return a < b;
		}
	};
	typedef set<FileEntry, FileEntryLess> FileSet;
#else
	typedef set<FileEntry> FileSet;
#endif

struct WatchedDirectory {
	node_ref		node;
	bool			contained;
};
typedef set<WatchedDirectory> DirectorySet;


class PathHandler;
typedef map<BString, PathHandler*> HandlerMap;

struct Watcher {
	HandlerMap handlers;
};
typedef map<BMessenger, Watcher*> WatcherMap;

class PathHandler : public BHandler {
	public:
		PathHandler(const char* path, uint32 flags, BMessenger target,
			BLooper* looper);
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

		bool _WatchRecursively() const;
		bool _WatchFilesOnly() const;
		bool _WatchFoldersOnly() const;

		void _EntryCreated(BMessage* message);
		void _EntryRemoved(BMessage* message);
		void _EntryMoved(BMessage* message);

		bool _IsContained(const node_ref& nodeRef) const;
		bool _IsContained(BEntry& entry) const;
		bool _HasDirectory(const node_ref& nodeRef,
			bool* _contained = NULL) const;
		bool _CloserToPath(BEntry& entry) const;

		void _NotifyTarget(BMessage* message) const;
		void _NotifyTarget(BMessage* message, const node_ref& nodeRef) const;

		status_t _AddDirectory(BEntry& entry, bool notify = false);
		status_t _AddDirectory(node_ref& nodeRef, bool notify = false);
		status_t _RemoveDirectory(const node_ref& nodeRef, ino_t directoryNode);
		status_t _RemoveDirectory(BEntry& entry, ino_t directoryNode);

		bool _HasFile(const node_ref& nodeRef) const;
		status_t _AddFile(BEntry& entry, bool notify = false);
		status_t _RemoveFile(const node_ref& nodeRef);
		status_t _RemoveFile(BEntry& entry);

		void _RemoveEntriesRecursively(BDirectory& directory);

		BPath			fPath;
		int32			fPathLength;
		BMessenger		fTarget;
		uint32			fFlags;
		status_t		fStatus;
		DirectorySet	fDirectories;
		FileSet			fFiles;
};


static WatcherMap sWatchers;
static BLocker* sLocker = NULL;
static BLooper* sLooper = NULL;


static status_t
set_entry(const node_ref& nodeRef, const char* name, BEntry& entry)
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
operator<(const FileEntry& a, const FileEntry& b)
{
	if (a.ref.device == b.ref.device && a.node < b.node)
		return true;
	if (a.ref.device < b.ref.device)
		return true;

	return false;
}


bool
operator<(const node_ref& a, const node_ref& b)
{
	if (a.device == b.device && a.node < b.node)
		return true;
	if (a.device < b.device)
		return true;

	return false;
}


bool
operator<(const WatchedDirectory& a, const WatchedDirectory& b)
{
	return a.node < b.node;
}


//	#pragma mark -


PathHandler::PathHandler(const char* path, uint32 flags, BMessenger target,
		BLooper* looper)
	: BHandler(path),
	fTarget(target),
	fFlags(flags)
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

	TRACE("PathHandler: %s\n", path);

	if (!looper->Lock())
		debugger("PathHandler: failed to lock the looper");
	looper->AddHandler(this);
	looper->Unlock();

	fStatus = _AddDirectory(nodeRef);

	// TODO: work-around for existing files (should not watch the directory in
	// this case)
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
	if (sLooper->Lock()) {
		stop_watching(this);
		sLooper->RemoveHandler(this);
		sLooper->Unlock();
	}
	delete this;
}


#ifdef TRACE_PATH_MONITOR
void
PathHandler::Dump()
{
	TRACE("WATCHING DIRECTORIES:\n");
	DirectorySet::iterator i = fDirectories.begin();
	for (; i != fDirectories.end(); i++) {
		TRACE("  %ld:%Ld (%s)\n", i->node.device, i->node.node, i->contained
			? "contained" : "-");
	}

	TRACE("WATCHING FILES:\n");

	FileSet::iterator j = fFiles.begin();
	for (; j != fFiles.end(); j++) {
		TRACE("  %ld:%Ld\n", j->ref.device, j->node);
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
PathHandler::_WatchRecursively() const
{
	return (fFlags & B_WATCH_RECURSIVELY) != 0;
}


bool
PathHandler::_WatchFilesOnly() const
{
	return (fFlags & B_WATCH_FILES_ONLY) != 0;
}


bool
PathHandler::_WatchFoldersOnly() const
{
	return (fFlags & B_WATCH_FOLDERS_ONLY) != 0;
}


void
PathHandler::_EntryCreated(BMessage* message)
{
	const char* name;
	node_ref nodeRef;
	if (message->FindInt32("device", &nodeRef.device) != B_OK
		|| message->FindInt64("directory", &nodeRef.node) != B_OK
		|| message->FindString("name", &name) != B_OK) {
		TRACE("PathHandler::_EntryCreated() - malformed message!\n");
		return;
	}

	BEntry entry;
	if (set_entry(nodeRef, name, entry) != B_OK) {
		TRACE("PathHandler::_EntryCreated() - set_entry failed!\n");
		return;
	}

	bool parentContained = false;
	bool entryContained = _IsContained(entry);
	if (entryContained)
		parentContained = _IsContained(nodeRef);
	bool notify = entryContained;

	if (entry.IsDirectory()) {
		// ignore the directory if it's already known
		if (entry.GetNodeRef(&nodeRef) == B_OK
			&& _HasDirectory(nodeRef)) {
			TRACE("    WE ALREADY HAVE DIR %s, %ld:%Ld\n",
				name, nodeRef.device, nodeRef.node);
			return;
		}

		// a new directory to watch for us
		if ((!entryContained && !_CloserToPath(entry))
			|| (parentContained && !_WatchRecursively())
			|| _AddDirectory(entry, true) != B_OK
			|| _WatchFilesOnly())
			notify = parentContained;
		// NOTE: entry is now toast after _AddDirectory() was called!
		// Does not matter right now, but if it's a problem, use the node_ref
		// version...
	} else if (entryContained) {
		TRACE("  NEW ENTRY PARENT CONTAINED: %d\n", parentContained);
		_AddFile(entry);
	}

	if (notify && entryContained) {
		message->AddBool("added", true);
		// nodeRef is pointing to the parent directory
		entry.GetNodeRef(&nodeRef);
		_NotifyTarget(message, nodeRef);
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

	bool contained;
	if (_HasDirectory(nodeRef, &contained)) {
		// the directory has been removed, so we remove it as well
		_RemoveDirectory(nodeRef, directoryNode);
		if (contained && !_WatchFilesOnly()) {
			message->AddBool("removed", true);
			_NotifyTarget(message, nodeRef);
		}
	} else if (_HasFile(nodeRef)) {
		message->AddBool("removed", true);
		_NotifyTarget(message, nodeRef);
		_RemoveFile(nodeRef);
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
		TRACE("    ADDED TO PARENT (%d), has entry %d/%d, entry %d %d\n",
			parentContained, _HasDirectory(nodeRef), _HasFile(nodeRef),
			entryContained, _CloserToPath(entry));

		if (entry.IsDirectory()) {
			if (!_HasDirectory(nodeRef)
				&& (entryContained || _CloserToPath(entry))) {
				// there is a new directory to watch for us
				if (entryContained
					|| (parentContained && !_WatchRecursively())) {
					_AddDirectory(entry, true);
					// NOTE: entry is toast now!
				} else if (_GetClosest(fPath.Path(), false,
						nodeRef) == B_OK) {
					// the new directory might put us even
					// closer to the path we are after
					_AddDirectory(nodeRef, true);
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

		_NotifyTarget(message, nodeRef);
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

		default:
			BHandler::MessageReceived(message);
			break;
	}

//#ifdef TRACE_PATH_MONITOR
//	Dump();
//#endif
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

	// Prevent the case that the entry is in another folder which happens
	// to have the same substring for fPathLength chars, like:
	// /path/we/are/watching
	// /path/we/are/watching-not/subfolder/entry
	// NOTE: We wouldn't be here if path.Path() was shorter than fPathLength,
	// strncmp() catches that case.
	const char* last = &path.Path()[fPathLength];
	if (last[0] && last[0] != '/')
		return false;

	return true;
}


bool
PathHandler::_HasDirectory(const node_ref& nodeRef,
	bool* _contained /* = NULL */) const
{
	WatchedDirectory directory;
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
	// NOTE: This version is only used for B_STAT_CHANGED and B_ATTR_CHANGED
	node_ref nodeRef;
	if (message->FindInt32("device", &nodeRef.device) != B_OK
		|| message->FindInt64("node", &nodeRef.node) != B_OK)
		return;
	_NotifyTarget(message, nodeRef);
}


void
PathHandler::_NotifyTarget(BMessage* message, const node_ref& nodeRef) const
{
	BMessage update(*message);
	update.what = B_PATH_MONITOR;

	TRACE("_NotifyTarget(): node ref %ld.%Ld\n", nodeRef.device, nodeRef.node);

	WatchedDirectory directory;
	directory.node = nodeRef;

	DirectorySet::const_iterator iterator = fDirectories.find(directory);
	if (iterator != fDirectories.end()) {
		if (_WatchFilesOnly()) {
			// stat or attr notification for a directory
			return;
		}
		BDirectory nodeDirectory(&nodeRef);
		BEntry entry;
		if (nodeDirectory.GetEntry(&entry) == B_OK) {
			BPath path(&entry);
			update.AddString("path", path.Path());
		}
	} else {
		if (_WatchFoldersOnly()) {
			// this is bound to be a notification for a file
			return;
		}
		FileEntry setEntry;
		setEntry.ref.device = nodeRef.device;
		setEntry.node = nodeRef.node;
			// name does not need to be set, since it's not used for comparing
		FileSet::const_iterator i = fFiles.find(setEntry);
		if (i != fFiles.end()) {
			BPath path(&(i->ref));
			update.AddString("path", path.Path());
		}
	}

	// This is in case the target is interested in figuring out which
	// BPathMonitor::StartWatching() call the message is resulting from.
	update.AddString("watched_path", fPath.Path());

	fTarget.SendMessage(&update);
}


status_t
PathHandler::_AddDirectory(BEntry& entry, bool notify)
{
	WatchedDirectory directory;
	status_t status = entry.GetNodeRef(&directory.node);
	if (status != B_OK)
		return status;

#ifdef TRACE_PATH_MONITOR
{
	BPath path(&entry);
	TRACE("  ADD DIRECTORY %s, %ld:%Ld\n",
		path.Path(), directory.node.device, directory.node.node);
}
#endif

	// check if we are already know this directory

	// TODO: It should be possible to ommit this check if we know it
	// can't be the case (for example when adding subfolders recursively,
	// although in that case, the API user may still have added this folder
	// independently, so for now, it should be the safest to perform this
	// check in all cases.)
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

	if (_WatchRecursively()) {
		BDirectory dir(&directory.node);
		while (dir.GetNextEntry(&entry) == B_OK) {
			if (entry.IsDirectory()) {
				// and here is the recursion:
				if (_AddDirectory(entry, notify) != B_OK)
					break;
			} else if (!_WatchFoldersOnly()) {
				if (_AddFile(entry, notify) != B_OK)
					break;
			}
		}
	}

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
PathHandler::_AddDirectory(node_ref& nodeRef, bool notify)
{
	BDirectory directory(&nodeRef);
	status_t status = directory.InitCheck();
	if (status == B_OK) {
		BEntry entry;
		status = directory.GetEntry(&entry);
		if (status == B_OK)
			status = _AddDirectory(entry, notify);
	}

	return status;
}


status_t
PathHandler::_RemoveDirectory(const node_ref& nodeRef, ino_t directoryNode)
{
	TRACE("  REMOVE DIRECTORY %ld:%Ld\n", nodeRef.device, nodeRef.node);

	WatchedDirectory directory;
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

	// stop watching subdirectories and their files when in recursive mode
	if (_WatchRecursively()) {
		BDirectory entryDirectory(&nodeRef);
		if (entryDirectory.InitCheck() == B_OK) {
			// The directory still exists, but was moved outside our watched
			// folder hierarchy.
			_RemoveEntriesRecursively(entryDirectory);
		} else {
			// Actually, it shouldn't be possible to remove non-empty
			// folders so for this case we don't need to do anything. We should
			// have received remove notifications for all affected files and
			// folders that used to live in this directory.
		}
	}

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
	FileEntry setEntry;
	setEntry.ref.device = nodeRef.device;
	setEntry.node = nodeRef.node;
		// name does not need to be set, since it's not used for comparing
	FileSet::const_iterator iterator = fFiles.find(setEntry);
	return iterator != fFiles.end();
}


status_t
PathHandler::_AddFile(BEntry& entry, bool notify)
{
	if ((fFlags & (WATCH_NODE_FLAG_MASK & ~B_WATCH_DIRECTORY)) == 0)
		return B_OK;

#ifdef TRACE_PATH_MONITOR
{
	BPath path(&entry);
	TRACE("  ADD FILE %s\n", path.Path());
}
#endif

	node_ref nodeRef;
	status_t status = entry.GetNodeRef(&nodeRef);
	if (status != B_OK)
		return status;

	// check if we already know this file

	// TODO: It should be possible to omit this check if we know it
	// can't be the case (for example when adding subfolders recursively,
	// although in that case, the API user may still have added this file
	// independently, so for now, it should be the safest to perform this
	// check in all cases.)
	if (_HasFile(nodeRef))
		return B_OK;

	status = watch_node(&nodeRef, (fFlags & WATCH_NODE_FLAG_MASK), this);
	if (status != B_OK)
		return status;

	FileEntry setEntry;
	entry.GetRef(&setEntry.ref);
	setEntry.node = nodeRef.node;

	fFiles.insert(setEntry);

	if (notify && _WatchFilesOnly()) {
		// We also notify our target about new files if it's only interested
		// in files; it won't be notified about new directories, so it cannot
		// know when to search for them.
		BMessage update;
		update.AddInt32("opcode", B_ENTRY_CREATED);
		update.AddInt32("device", nodeRef.device);
		update.AddInt64("directory", setEntry.ref.directory);
		update.AddString("name", setEntry.ref.name);
		update.AddBool("added", true);

		_NotifyTarget(&update, nodeRef);
	}

	return B_OK;
}


status_t
PathHandler::_RemoveFile(const node_ref& nodeRef)
{
	TRACE("  REMOVE FILE %ld:%Ld\n", nodeRef.device, nodeRef.node);

	FileEntry setEntry;
	setEntry.ref.device = nodeRef.device;
	setEntry.node = nodeRef.node;
		// name does not need to be set, since it's not used for comparing
	FileSet::iterator iterator = fFiles.find(setEntry);
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


void
PathHandler::_RemoveEntriesRecursively(BDirectory& directory)
{
	node_ref directoryNode;
	directory.GetNodeRef(&directoryNode);

	BMessage message(B_PATH_MONITOR);
	message.AddInt32("opcode", B_ENTRY_REMOVED);
		// TODO: B_ENTRY_MOVED could be regarded as more correct,
		// but then we would definitely need more information in this
		// function.
	message.AddInt32("device", directoryNode.device);
	message.AddInt64("directory", directoryNode.node);
	message.AddInt64("node", 0LL);
		// dummy node, will be replaced by real node

	// NOTE: The _NotifyTarget() gets the node id, but constructs
	// the path to the previous location of the entry according to the file
	// or folder in our sets. This makes it more expensive of course, but
	// I have no inspiration for improvement at the moment.

	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK) {
		node_ref nodeRef;
		if (entry.GetNodeRef(&nodeRef) != B_OK) {
			fprintf(stderr, "PathHandler::_RemoveEntriesRecursively() - "
				"failed to get node_ref\n");
			continue;
		}

		message.ReplaceInt64("node", nodeRef.node);

		if (entry.IsDirectory()) {
			// notification
			if (!_WatchFilesOnly())
				_NotifyTarget(&message, nodeRef);

			_RemoveDirectory(nodeRef, directoryNode.node);
			BDirectory subDirectory(&entry);
			_RemoveEntriesRecursively(subDirectory);
		} else {
			// notification
			if (!_WatchFoldersOnly())
				_NotifyTarget(&message, nodeRef);

			_RemoveFile(nodeRef);
		}
	}
}


//	#pragma mark -


BPathMonitor::BPathMonitor()
{
}


BPathMonitor::~BPathMonitor()
{
}


/*static*/ status_t
BPathMonitor::_InitLockerIfNeeded()
{
	static vint32 lock = 0;

	if (sLocker != NULL)
		return B_OK;

	while (sLocker == NULL) {
		if (atomic_add(&lock, 1) == 0) {
			sLocker = new (nothrow) BLocker("path monitor");
			TRACE("Create PathMonitor locker\n");
			if (sLocker == NULL)
				return B_NO_MEMORY;
		}
		snooze(5000);
	}

	return B_OK;
}


/*static*/ status_t
BPathMonitor::_InitLooperIfNeeded()
{
	static vint32 lock = 0;

	if (sLooper != NULL)
		return B_OK;

	while (sLooper == NULL) {
		if (atomic_add(&lock, 1) == 0) {
			// first thread initializes the global looper
			sLooper = new (nothrow) BLooper("PathMonitor looper");
			TRACE("Start PathMonitor looper\n");
			if (sLooper == NULL)
				return B_NO_MEMORY;
			thread_id thread = sLooper->Run();
			if (thread < B_OK)
				return (status_t)thread;
		}
		snooze(5000);
	}

	return sLooper->Thread() >= 0 ? B_OK : B_ERROR;
}


/*static*/ status_t
BPathMonitor::StartWatching(const char* path, uint32 flags, BMessenger target)
{
	TRACE("StartWatching(%s)\n", path);

	status_t status = _InitLockerIfNeeded();
	if (status != B_OK)
		return status;

	// use the global looper for receiving node monitor notifications
	status = _InitLooperIfNeeded();
	if (status < B_OK)
		return status;

	BAutolock _(sLocker);

	WatcherMap::iterator iterator = sWatchers.find(target);
	Watcher* watcher = NULL;
	if (iterator != sWatchers.end())
		watcher = iterator->second;

	PathHandler* handler = new (nothrow) PathHandler(path, flags, target,
		sLooper);
	if (handler == NULL)
		return B_NO_MEMORY;
	status = handler->InitCheck();
	if (status < B_OK) {
		delete handler;
		return status;
	}

	if (watcher == NULL) {
		watcher = new (nothrow) BPrivate::Watcher;
		if (watcher == NULL) {
			delete handler;
			return B_NO_MEMORY;
		}
		sWatchers[target] = watcher;
	}

	watcher->handlers[path] = handler;
	return B_OK;
}


/*static*/ status_t
BPathMonitor::StopWatching(const char* path, BMessenger target)
{
	if (sLocker == NULL)
		return B_NO_INIT;

	TRACE("StopWatching(%s)\n", path);

	BAutolock _(sLocker);

	WatcherMap::iterator iterator = sWatchers.find(target);
	if (iterator == sWatchers.end())
		return B_BAD_VALUE;

	Watcher* watcher = iterator->second;
	HandlerMap::iterator i = watcher->handlers.find(path);

	if (i == watcher->handlers.end())
		return B_BAD_VALUE;

	PathHandler* handler = i->second;
	watcher->handlers.erase(i);

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
	if (sLocker == NULL)
		return B_NO_INIT;

	BAutolock _(sLocker);

	WatcherMap::iterator iterator = sWatchers.find(target);
	if (iterator == sWatchers.end())
		return B_BAD_VALUE;

	Watcher* watcher = iterator->second;
	while (!watcher->handlers.empty()) {
		HandlerMap::iterator i = watcher->handlers.begin();
		PathHandler* handler = i->second;
		watcher->handlers.erase(i);

		handler->Quit();
	}

	sWatchers.erase(iterator);
	delete watcher;

	return B_OK;
}

}	// namespace BPrivate
