/*
 * Copyright 2007-2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus, superstippi@gmx.de
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */


#include <PathMonitor.h>

#include <pthread.h>
#include <stdio.h>

#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <Handler.h>
#include <Locker.h>
#include <Looper.h>
#include <Path.h>
#include <String.h>

#include <AutoDeleter.h>
#include <NotOwningEntryRef.h>
#include <ObjectList.h>
#include <util/OpenHashTable.h>
#include <util/SinglyLinkedList.h>


#undef TRACE
//#define TRACE_PATH_MONITOR
#ifdef TRACE_PATH_MONITOR
#	define TRACE(...) debug_printf("BPathMonitor: " __VA_ARGS__)
#else
#	define TRACE(...) ;
#endif


// TODO: Support symlink components in the path.
// TODO: Support mounting/unmounting of volumes in path components and within
// the watched path tree.


#define WATCH_NODE_FLAG_MASK	0x00ff


namespace {


class Directory;
class Node;
struct WatcherHashDefinition;
typedef BOpenHashTable<WatcherHashDefinition> WatcherMap;


static pthread_once_t sInitOnce = PTHREAD_ONCE_INIT;
static WatcherMap* sWatchers = NULL;
static BLooper* sLooper = NULL;
static BPathMonitor::BWatchingInterface* sDefaultWatchingInterface = NULL;
static BPathMonitor::BWatchingInterface* sWatchingInterface = NULL;


//	#pragma mark -


/*! Returns empty path, if either \a parent or \a subPath is empty or an
	allocation fails.
 */
static BString
make_path(const BString& parent, const char* subPath)
{
	BString path = parent;
	int32 length = path.Length();
	if (length == 0 || subPath[0] == '\0')
		return BString();

	if (parent.ByteAt(length - 1) != '/') {
		path << '/';
		if (path.Length() < ++length)
			return BString();
	}

	path << subPath;
	if (path.Length() <= length)
		return BString();
	return path;
}


//	#pragma mark - Ancestor


class Ancestor {
public:
	Ancestor(Ancestor* parent, const BString& path, size_t pathComponentOffset)
		:
		fParent(parent),
		fChild(NULL),
		fPath(path),
		fEntryRef(-1, -1, fPath.String() + pathComponentOffset),
		fNodeRef(),
		fWatchingFlags(0),
		fIsDirectory(false)
	{
		if (pathComponentOffset == 0) {
			// must be "/"
			fEntryRef.SetTo(-1, -1, ".");
		}

		if (fParent != NULL)
			fParent->fChild = this;
	}

	Ancestor* Parent() const
	{
		return fParent;
	}

	Ancestor* Child() const
	{
		return fChild;
	}

	const BString& Path() const
	{
		return fPath;
	}

	const char* Name() const
	{
		return fEntryRef.name;
	}

	bool Exists() const
	{
		return fNodeRef.device >= 0;
	}

	const NotOwningEntryRef& EntryRef() const
	{
		return fEntryRef;
	}

	const node_ref& NodeRef() const
	{
		return fNodeRef;
	}

	bool IsDirectory() const
	{
		return fIsDirectory;
	}

	status_t StartWatching(uint32 pathFlags, BHandler* target)
	{
		// init entry ref
		BEntry entry;
		status_t error = entry.SetTo(fPath);
		if (error != B_OK)
			return error;

		entry_ref entryRef;
		error = entry.GetRef(&entryRef);
		if (error != B_OK)
			return error;

		fEntryRef.device = entryRef.device;
		fEntryRef.directory = entryRef.directory;

		// init node ref
		struct stat st;
		error = entry.GetStat(&st);
		if (error != B_OK)
			return error == B_ENTRY_NOT_FOUND ? B_OK : error;

		fNodeRef = node_ref(st.st_dev, st.st_ino);
		fIsDirectory = S_ISDIR(st.st_mode);

		// start watching
		uint32 flags = fChild == NULL ? pathFlags : B_WATCH_DIRECTORY;
			// In theory B_WATCH_NAME would suffice for all existing ancestors,
			// plus B_WATCH_DIRECTORY for the parent of the first not existing
			// ancestor. In practice this complicates the transitions when an
			// ancestor is created/removed/moved.
		if (flags != 0) {
			error = sWatchingInterface->WatchNode(&fNodeRef, flags, target);
			TRACE("  started to watch ancestor %p (\"%s\", %#" B_PRIx32
				") -> %s\n", this, Name(), flags, strerror(error));
			if (error != B_OK)
				return error;
		}

		fWatchingFlags = flags;
		return B_OK;
	}

	void StopWatching(BHandler* target)
	{
		// stop watching
		if (fWatchingFlags != 0) {
			sWatchingInterface->WatchNode(&fNodeRef, B_STOP_WATCHING, target);
			fWatchingFlags = 0;
		}

		// uninitialize node and entry ref
		fIsDirectory = false;
		fNodeRef = node_ref();
		fEntryRef.SetDirectoryNodeRef(node_ref());
	}

	Ancestor*& HashNext()
	{
		return fHashNext;
	}

private:
	Ancestor*			fParent;
	Ancestor*			fChild;
	Ancestor*			fHashNext;
	BString				fPath;
	NotOwningEntryRef	fEntryRef;
	node_ref			fNodeRef;
	uint32				fWatchingFlags;
	bool				fIsDirectory;
};


//	#pragma mark - AncestorMap


struct AncestorHashDefinition {
	typedef	node_ref	KeyType;
	typedef	Ancestor	ValueType;

	size_t HashKey(const node_ref& key) const
	{
		return size_t(key.device ^ key.node);
	}

	size_t Hash(Ancestor* value) const
	{
		return HashKey(value->NodeRef());
	}

	bool Compare(const node_ref& key, Ancestor* value) const
	{
		return key == value->NodeRef();
	}

	Ancestor*& GetLink(Ancestor* value) const
	{
		return value->HashNext();
	}
};


typedef BOpenHashTable<AncestorHashDefinition> AncestorMap;


//	#pragma mark - Entry


class Entry : public SinglyLinkedListLinkImpl<Entry> {
public:
	Entry(Directory* parent, const BString& name, ::Node* node)
		:
		fParent(parent),
		fName(name),
		fNode(node)
	{
	}

	Directory* Parent() const
	{
		return fParent;
	}

	const BString& Name() const
	{
		return fName;
	}

	::Node* Node() const
	{
		return fNode;
	}

	void SetNode(::Node* node)
	{
		fNode = node;
	}

	inline NotOwningEntryRef EntryRef() const;

	Entry*& HashNext()
	{
		return fHashNext;
	}

private:
	Directory*	fParent;
	BString		fName;
	::Node*		fNode;
	Entry*		fHashNext;
};

typedef SinglyLinkedList<Entry> EntryList;


// EntryMap


struct EntryHashDefinition {
	typedef	const char*	KeyType;
	typedef	Entry		ValueType;

	size_t HashKey(const char* key) const
	{
		return BString::HashValue(key);
	}

	size_t Hash(Entry* value) const
	{
		return value->Name().HashValue();
	}

	bool Compare(const char* key, Entry* value) const
	{
		return value->Name() == key;
	}

	Entry*& GetLink(Entry* value) const
	{
		return value->HashNext();
	}
};


typedef BOpenHashTable<EntryHashDefinition> EntryMap;


//	#pragma mark - Node


class Node {
public:
	Node(const node_ref& nodeRef)
		:
		fNodeRef(nodeRef)
	{
	}

	virtual ~Node()
	{
	}

	virtual bool IsDirectory() const
	{
		return false;
	}

	virtual Directory* ToDirectory()
	{
		return NULL;
	}

	const node_ref& NodeRef() const
	{
		return fNodeRef;
	}

	const EntryList& Entries() const
	{
		return fEntries;
	}

	bool HasEntries() const
	{
		return !fEntries.IsEmpty();
	}

	Entry* FirstNodeEntry() const
	{
		return fEntries.Head();
	}

	bool IsOnlyNodeEntry(Entry* entry) const
	{
		return entry == fEntries.Head() && fEntries.GetNext(entry) == NULL;
	}

	void AddNodeEntry(Entry* entry)
	{
		fEntries.Add(entry);
	}

	void RemoveNodeEntry(Entry* entry)
	{
		fEntries.Remove(entry);
	}

	Node*& HashNext()
	{
		return fHashNext;
	}

private:
	node_ref			fNodeRef;
	EntryList			fEntries;
	Node*				fHashNext;
};


struct NodeHashDefinition {
	typedef	node_ref	KeyType;
	typedef	Node		ValueType;

	size_t HashKey(const node_ref& key) const
	{
		return size_t(key.device ^ key.node);
	}

	size_t Hash(Node* value) const
	{
		return HashKey(value->NodeRef());
	}

	bool Compare(const node_ref& key, Node* value) const
	{
		return key == value->NodeRef();
	}

	Node*& GetLink(Node* value) const
	{
		return value->HashNext();
	}
};


typedef BOpenHashTable<NodeHashDefinition> NodeMap;


//	#pragma mark - Directory


class Directory : public Node {
public:
	static Directory* Create(const node_ref& nodeRef)
	{
		Directory* directory = new(std::nothrow) Directory(nodeRef);
		if (directory == NULL || directory->fEntries.Init() != B_OK) {
			delete directory;
			return NULL;
		}

		return directory;
	}

	virtual bool IsDirectory() const
	{
		return true;
	}

	virtual Directory* ToDirectory()
	{
		return this;
	}

	Entry* FindEntry(const char* name) const
	{
		return fEntries.Lookup(name);
	}

	Entry* CreateEntry(const BString& name, Node* node)
	{
		Entry* entry = new(std::nothrow) Entry(this, name, node);
		if (entry == NULL || entry->Name().IsEmpty()) {
			delete entry;
			return NULL;
		}

		AddEntry(entry);
		return entry;
	}

	void AddEntry(Entry* entry)
	{
		fEntries.Insert(entry);
	}

	void RemoveEntry(Entry* entry)
	{
		fEntries.Remove(entry);
	}

	EntryMap::Iterator GetEntryIterator() const
	{
		return fEntries.GetIterator();
	}

	Entry* RemoveAllEntries()
	{
		return fEntries.Clear(true);
	}

private:
	Directory(const node_ref& nodeRef)
		:
		Node(nodeRef)
	{
	}

private:
	EntryMap	fEntries;
};


//	#pragma mark - Entry


inline NotOwningEntryRef
Entry::EntryRef() const
{
	return NotOwningEntryRef(fParent->NodeRef(), fName);
}


//	#pragma mark - PathHandler


class PathHandler : public BHandler {
public:
								PathHandler(const char* path, uint32 flags,
									const BMessenger& target, BLooper* looper);
	virtual						~PathHandler();

			status_t			InitCheck() const;
			void				Quit();

			const BString&		OriginalPath() const
									{ return fOriginalPath; }
			uint32				Flags() const	{ return fFlags; }

	virtual	void				MessageReceived(BMessage* message);

			PathHandler*&		HashNext()	{ return fHashNext; }

private:
			status_t			_CreateAncestors();
			status_t			_StartWatchingAncestors(Ancestor* ancestor,
									bool notify);
			void				_StopWatchingAncestors(Ancestor* ancestor,
									bool notify);

			void				_EntryCreated(BMessage* message);
			void				_EntryRemoved(BMessage* message);
			void				_EntryMoved(BMessage* message);
			void				_NodeChanged(BMessage* message);

			bool				_EntryCreated(const NotOwningEntryRef& entryRef,
									const node_ref& nodeRef, bool isDirectory,
									bool dryRun, bool notify, Entry** _entry);
			bool				_EntryRemoved(const NotOwningEntryRef& entryRef,
									const node_ref& nodeRef, bool dryRun,
									bool notify, Entry** _keepEntry);

			bool				_CheckDuplicateEntryNotification(int32 opcode,
									const entry_ref& toEntryRef,
									const node_ref& nodeRef,
									const entry_ref* fromEntryRef = NULL);
			void				_UnsetDuplicateEntryNotification();

			Ancestor*			_GetAncestor(const node_ref& nodeRef) const;

			status_t			_AddNode(const node_ref& nodeRef,
									bool isDirectory, bool notify,
									Entry* entry = NULL, Node** _node = NULL);
			void				_DeleteNode(Node* node, bool notify);
			Node*				_GetNode(const node_ref& nodeRef) const;

			status_t			_AddEntryIfNeeded(Directory* directory,
									const char* name, const node_ref& nodeRef,
									bool isDirectory, bool notify,
									Entry** _entry = NULL);
			void				_DeleteEntry(Entry* entry, bool notify);
			void				_DeleteEntryAlreadyRemovedFromParent(
									Entry* entry, bool notify);

			void				_NotifyFilesCreatedOrRemoved(Entry* entry,
									int32 opcode) const;
			void				_NotifyEntryCreatedOrRemoved(Entry* entry,
									int32 opcode) const;
			void				_NotifyEntryCreatedOrRemoved(
									const entry_ref& entryRef,
									const node_ref& nodeRef, const char* path,
									bool isDirectory, int32 opcode) const;
			void				_NotifyEntryMoved(const entry_ref& fromEntryRef,
									const entry_ref& toEntryRef,
									const node_ref& nodeRef,
									const char* fromPath, const char* path,
									bool isDirectory, bool wasAdded,
									bool wasRemoved) const;
			void				_NotifyTarget(BMessage& message,
									const char* path) const;

			BString				_NodePath(const Node* node) const;
			BString				_EntryPath(const Entry* entry) const;


			bool				_WatchRecursively() const;
			bool				_WatchFilesOnly() const;
			bool				_WatchDirectoriesOnly() const;

private:
			BMessenger			fTarget;
			uint32				fFlags;
			status_t			fStatus;
			BString				fOriginalPath;
			BString				fPath;
			Ancestor*			fRoot;
			Ancestor*			fBaseAncestor;
			Node*				fBaseNode;
			AncestorMap			fAncestors;
			NodeMap				fNodes;
			PathHandler*		fHashNext;
			int32				fDuplicateEntryNotificationOpcode;
			node_ref			fDuplicateEntryNotificationNodeRef;
			entry_ref			fDuplicateEntryNotificationToEntryRef;
			entry_ref			fDuplicateEntryNotificationFromEntryRef;
};


struct PathHandlerHashDefinition {
	typedef	const char*	KeyType;
	typedef	PathHandler	ValueType;

	size_t HashKey(const char* key) const
	{
		return BString::HashValue(key);
	}

	size_t Hash(PathHandler* value) const
	{
		return value->OriginalPath().HashValue();
	}

	bool Compare(const char* key, PathHandler* value) const
	{
		return key == value->OriginalPath();
	}

	PathHandler*& GetLink(PathHandler* value) const
	{
		return value->HashNext();
	}
};


typedef BOpenHashTable<PathHandlerHashDefinition> PathHandlerMap;


//	#pragma mark - Watcher


struct Watcher : public PathHandlerMap {
	static Watcher* Create(const BMessenger& target)
	{
		Watcher* watcher = new(std::nothrow) Watcher(target);
		if (watcher == NULL || watcher->Init() != B_OK) {
			delete watcher;
			return NULL;
		}
		return watcher;
	}

	const BMessenger& Target() const
	{
		return fTarget;
	}

	Watcher*& HashNext()
	{
		return fHashNext;
	}

private:
	Watcher(const BMessenger& target)
		:
		fTarget(target)
	{
	}

private:
	BMessenger		fTarget;
	Watcher*		fHashNext;
};


struct WatcherHashDefinition {
	typedef	BMessenger	KeyType;
	typedef	Watcher		ValueType;

	size_t HashKey(const BMessenger& key) const
	{
		return key.HashValue();
	}

	size_t Hash(Watcher* value) const
	{
		return HashKey(value->Target());
	}

	bool Compare(const BMessenger& key, Watcher* value) const
	{
		return key == value->Target();
	}

	Watcher*& GetLink(Watcher* value) const
	{
		return value->HashNext();
	}
};


//	#pragma mark - PathHandler


PathHandler::PathHandler(const char* path, uint32 flags,
	const BMessenger& target, BLooper* looper)
	:
	BHandler(path),
	fTarget(target),
	fFlags(flags),
	fStatus(B_OK),
	fOriginalPath(path),
	fPath(),
	fRoot(NULL),
	fBaseAncestor(NULL),
	fBaseNode(NULL),
	fAncestors(),
	fNodes()
{
	TRACE("%p->PathHandler::PathHandler(\"%s\", %#" B_PRIx32 ")\n", this, path,
		flags);

	_UnsetDuplicateEntryNotification();

	fStatus = fAncestors.Init();
	if (fStatus != B_OK)
		return;

	fStatus = fNodes.Init();
	if (fStatus != B_OK)
		return;

	// normalize the flags
	if ((fFlags & B_WATCH_RECURSIVELY) != 0) {
		// We add B_WATCH_NAME and B_WATCH_DIRECTORY as needed, so clear them
		// here.
		fFlags &= ~uint32(B_WATCH_NAME | B_WATCH_DIRECTORY);
	} else {
		// The B_WATCH_*_ONLY flags are only valid for the recursive mode.
		// B_WATCH_NAME is implied (we watch the parent directory).
		fFlags &= ~uint32(B_WATCH_FILES_ONLY | B_WATCH_DIRECTORIES_ONLY
			| B_WATCH_NAME);
	}

	// Normalize the path a bit. We can't use BPath, as it may really normalize
	// the path, i.e. resolve symlinks and such, which may cause us to monitor
	// the wrong path. We want some normalization, though:
	// * relative -> absolute path
	// * fold duplicate '/'s
	// * omit "." components
	// * fail when encountering ".." components

	// make absolute
	BString normalizedPath;
	if (path[0] == '/') {
		normalizedPath = "/";
		path++;
	} else
		normalizedPath = BPath(".").Path();
	if (normalizedPath.IsEmpty()) {
		fStatus = B_NO_MEMORY;
		return;
	}

	// parse path components
	const char* pathEnd = path + strlen(path);
	for (;;) {
		// skip '/'s
		while (path[0] == '/')
			path++;
		if (path == pathEnd)
			break;

		const char* componentEnd = strchr(path, '/');
		if (componentEnd == NULL)
			componentEnd = pathEnd;
		size_t componentLength = componentEnd - path;

		// handle ".' and ".."
		if (path[0] == '.') {
			if (componentLength == 1) {
				path = componentEnd;
				continue;
			}
			if (componentLength == 2 && path[1] == '.') {
				fStatus = B_BAD_VALUE;
				return;
			}
		}

		int32 normalizedPathLength = normalizedPath.Length();
		if (normalizedPath.ByteAt(normalizedPathLength - 1) != '/') {
			normalizedPath << '/';
			normalizedPathLength++;
		}
		normalizedPath.Append(path, componentEnd - path);
		normalizedPathLength += int32(componentEnd - path);

		if (normalizedPath.Length() != normalizedPathLength) {
			fStatus = B_NO_MEMORY;
			return;
		}

		path = componentEnd;
	}

	fPath = normalizedPath;

	// Create the Ancestor objects -- they correspond to the path components and
	// are used for watching changes that affect the entries on the path.
	fStatus = _CreateAncestors();
	if (fStatus != B_OK)
		return;

	// add ourselves to the looper
	looper->AddHandler(this);

	// start watching
	fStatus = _StartWatchingAncestors(fRoot, false);
	if (fStatus != B_OK)
		return;
}


PathHandler::~PathHandler()
{
	TRACE("%p->PathHandler::~PathHandler(\"%s\", %#" B_PRIx32 ")\n", this,
		fPath.String(), fFlags);

	if (fBaseNode != NULL)
		_DeleteNode(fBaseNode, false);

	while (fRoot != NULL) {
		Ancestor* nextAncestor = fRoot->Child();
		delete fRoot;
		fRoot = nextAncestor;
	}
}


status_t
PathHandler::InitCheck() const
{
	return fStatus;
}


void
PathHandler::Quit()
{
	TRACE("%p->PathHandler::Quit()\n", this);
	sWatchingInterface->StopWatching(this);
	sLooper->RemoveHandler(this);
	delete this;
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
					_UnsetDuplicateEntryNotification();
					_NodeChanged(message);
					break;
			}

			break;
		}

		default:
			BHandler::MessageReceived(message);
			break;
	}
}


status_t
PathHandler::_CreateAncestors()
{
	TRACE("%p->PathHandler::_CreateAncestors()\n", this);

	// create the Ancestor objects
	const char* path = fPath.String();
	const char* pathEnd = path + fPath.Length();
	const char* component = path;

	Ancestor* ancestor = NULL;

	while (component < pathEnd) {
		const char* componentEnd = component == path
			? component + 1 : strchr(component, '/');
		if (componentEnd == NULL)
			componentEnd = pathEnd;

		BString ancestorPath(path, componentEnd - path);
		if (ancestorPath.IsEmpty())
			return B_NO_MEMORY;

		ancestor = new(std::nothrow) Ancestor(ancestor, ancestorPath,
			component - path);
		TRACE("  created ancestor %p (\"%s\" / \"%s\")\n", ancestor,
			ancestor->Path().String(), ancestor->Name());
		if (ancestor == NULL)
			return B_NO_MEMORY;

		if (fRoot == NULL)
			fRoot = ancestor;

		component = componentEnd[0] == '/' ? componentEnd + 1 : componentEnd;
	}

	fBaseAncestor = ancestor;

	return B_OK;
}


status_t
PathHandler::_StartWatchingAncestors(Ancestor* startAncestor, bool notify)
{
	TRACE("%p->PathHandler::_StartWatchingAncestors(%p, %d)\n", this,
		startAncestor, notify);

	// The watch flags for the path (if it exists). Recursively implies
	// directory, since we need to watch the entries.
	uint32 watchFlags = (fFlags & WATCH_NODE_FLAG_MASK)
		| (_WatchRecursively() ? B_WATCH_DIRECTORY : 0);

	for (Ancestor* ancestor = startAncestor; ancestor != NULL;
		ancestor = ancestor->Child()) {
		status_t error = ancestor->StartWatching(watchFlags, this);
		if (error != B_OK)
			return error;

		if (!ancestor->Exists()) {
			TRACE("  -> ancestor doesn't exist\n");
			break;
		}

		fAncestors.Insert(ancestor);
	}

	if (!fBaseAncestor->Exists())
		return B_OK;

	if (notify) {
		_NotifyEntryCreatedOrRemoved(fBaseAncestor->EntryRef(),
			fBaseAncestor->NodeRef(), fPath, fBaseAncestor->IsDirectory(),
			B_ENTRY_CREATED);
	}

	if (!_WatchRecursively())
		return B_OK;

	status_t error = _AddNode(fBaseAncestor->NodeRef(),
		fBaseAncestor->IsDirectory(), notify && _WatchFilesOnly(), NULL,
		&fBaseNode);
	if (error != B_OK)
		return error;

	return B_OK;
}


void
PathHandler::_StopWatchingAncestors(Ancestor* ancestor, bool notify)
{
	// stop watching the tree below path
	if (fBaseNode != NULL) {
		_DeleteNode(fBaseNode, notify && _WatchFilesOnly());
		fBaseNode = NULL;
	}

	if (notify && fBaseAncestor->Exists()
		&& (fBaseAncestor->IsDirectory()
			? !_WatchFilesOnly() : !_WatchDirectoriesOnly())) {
		_NotifyEntryCreatedOrRemoved(fBaseAncestor->EntryRef(),
			fBaseAncestor->NodeRef(), fPath, fBaseAncestor->IsDirectory(),
			B_ENTRY_REMOVED);
	}

	// stop watching the ancestors and uninitialize their entries
	for (; ancestor != NULL; ancestor = ancestor->Child()) {
		if (ancestor->Exists())
			fAncestors.Remove(ancestor);
		ancestor->StopWatching(this);
	}
}


void
PathHandler::_EntryCreated(BMessage* message)
{
	// TODO: Unless we're watching files only, we might want to forward (some
	// of) the messages that don't agree with our model, since our client
	// maintains its model at a different time and the notification might be
	// necessary to keep it up-to-date. E.g. consider the following case:
	// 1. a directory is created
	// 2. a file is created in the directory
	// 3. the file is removed from the directory
	// If we get the notification after 1. and before 2., we pass it on to the
	// client, which may get it after 2. and before 3., thus seeing the file.
	// If we then get the entry-created notification after 3., we don't see the
	// file anymore and ignore the notification as well as the following
	// entry-removed notification. That is the client will never know that the
	// file has been removed. This can only happen in recursive mode. Otherwise
	// (and with B_WATCH_DIRECTORY) we just pass on all notifications.
	// A possible solution could be to just create a zombie entry and pass on
	// the entry-created notification. We wouldn't be able to adhere to the
	// B_WATCH_FILES_ONLY/B_WATCH_DIRECTORIES_ONLY flags, but that should be
	// acceptable. Either the client hasn't seen the entry either -- then it
	// doesn't matter -- or it likely has ignored a not matching entry anyway.

	NotOwningEntryRef entryRef;
	node_ref nodeRef;

	if (message->FindInt32("device", &nodeRef.device) != B_OK
		|| message->FindInt64("node", &nodeRef.node) != B_OK
		|| message->FindInt64("directory", &entryRef.directory) != B_OK
		|| message->FindString("name", (const char**)&entryRef.name) != B_OK) {
		return;
	}
	entryRef.device = nodeRef.device;

	if (_CheckDuplicateEntryNotification(B_ENTRY_CREATED, entryRef, nodeRef))
		return;

	TRACE("%p->PathHandler::_EntryCreated(): entry: %" B_PRIdDEV ":%" B_PRIdINO
		":\"%s\", node: %" B_PRIdDEV ":%" B_PRIdINO "\n", this, entryRef.device,
		entryRef.directory, entryRef.name, nodeRef.device, nodeRef.node);

	BEntry entry;
	struct stat st;
	if (entry.SetTo(&entryRef) != B_OK || entry.GetStat(&st) != B_OK
		|| nodeRef != node_ref(st.st_dev, st.st_ino)) {
		return;
	}

	_EntryCreated(entryRef, nodeRef, S_ISDIR(st.st_mode), false, true, NULL);
}


void
PathHandler::_EntryRemoved(BMessage* message)
{
	NotOwningEntryRef entryRef;
	node_ref nodeRef;

	if (message->FindInt32("device", &nodeRef.device) != B_OK
		|| message->FindInt64("node", &nodeRef.node) != B_OK
		|| message->FindInt64("directory", &entryRef.directory) != B_OK
		|| message->FindString("name", (const char**)&entryRef.name) != B_OK) {
		return;
	}
	entryRef.device = nodeRef.device;

	if (_CheckDuplicateEntryNotification(B_ENTRY_REMOVED, entryRef, nodeRef))
		return;

	TRACE("%p->PathHandler::_EntryRemoved(): entry: %" B_PRIdDEV ":%" B_PRIdINO
		":\"%s\", node: %" B_PRIdDEV ":%" B_PRIdINO "\n", this, entryRef.device,
		entryRef.directory, entryRef.name, nodeRef.device, nodeRef.node);

	_EntryRemoved(entryRef, nodeRef, false, true, NULL);
}


void
PathHandler::_EntryMoved(BMessage* message)
{
	NotOwningEntryRef fromEntryRef;
	NotOwningEntryRef toEntryRef;
	node_ref nodeRef;

	if (message->FindInt32("node device", &nodeRef.device) != B_OK
		|| message->FindInt64("node", &nodeRef.node) != B_OK
		|| message->FindInt32("device", &fromEntryRef.device) != B_OK
		|| message->FindInt64("from directory", &fromEntryRef.directory) != B_OK
		|| message->FindInt64("to directory", &toEntryRef.directory) != B_OK
		|| message->FindString("from name", (const char**)&fromEntryRef.name)
			!= B_OK
		|| message->FindString("name", (const char**)&toEntryRef.name)
			!= B_OK) {
		return;
	}
	toEntryRef.device = fromEntryRef.device;

	if (_CheckDuplicateEntryNotification(B_ENTRY_MOVED, toEntryRef, nodeRef,
			&fromEntryRef)) {
		return;
	}

	TRACE("%p->PathHandler::_EntryMoved(): entry: %" B_PRIdDEV ":%" B_PRIdINO
		":\"%s\" -> %" B_PRIdDEV ":%" B_PRIdINO ":\"%s\", node: %" B_PRIdDEV
		":%" B_PRIdINO "\n", this, fromEntryRef.device, fromEntryRef.directory,
		fromEntryRef.name, toEntryRef.device, toEntryRef.directory,
		toEntryRef.name, nodeRef.device, nodeRef.node);

	BEntry entry;
	struct stat st;
	if (entry.SetTo(&toEntryRef) != B_OK || entry.GetStat(&st) != B_OK
		|| nodeRef != node_ref(st.st_dev, st.st_ino)) {
		_EntryRemoved(fromEntryRef, nodeRef, false, true, NULL);
		return;
	}
	bool isDirectory = S_ISDIR(st.st_mode);

	Ancestor* fromAncestor = _GetAncestor(fromEntryRef.DirectoryNodeRef());
	Ancestor* toAncestor = _GetAncestor(toEntryRef.DirectoryNodeRef());

	if (_WatchRecursively()) {
		Node* fromDirectoryNode = _GetNode(fromEntryRef.DirectoryNodeRef());
		Node* toDirectoryNode = _GetNode(toEntryRef.DirectoryNodeRef());
		if (fromDirectoryNode != NULL || toDirectoryNode != NULL) {
			// Check whether _EntryRemoved()/_EntryCreated() can handle the
			// respective entry regularly (i.e. don't encounter an out-of-sync
			// issue) or don't need to be called at all (entry outside the
			// monitored tree).
			if ((fromDirectoryNode == NULL
					|| _EntryRemoved(fromEntryRef, nodeRef, true, false, NULL))
				&& (toDirectoryNode == NULL
					|| _EntryCreated(toEntryRef, nodeRef, isDirectory, true,
						false, NULL))) {
				// The entries can be handled regularly. We delegate the work to
				// _EntryRemoved() and _EntryCreated() and only handle the
				// notification ourselves.

				// handle removed
				Entry* removedEntry = NULL;
				if (fromDirectoryNode != NULL) {
					_EntryRemoved(fromEntryRef, nodeRef, false, false,
						&removedEntry);
				}

				// handle created
				Entry* createdEntry = NULL;
				if (toDirectoryNode != NULL) {
					_EntryCreated(toEntryRef, nodeRef, isDirectory, false,
						false, &createdEntry);
				}

				// notify
				if (_WatchFilesOnly() && isDirectory) {
					// recursively iterate through the removed and created
					// hierarchy and send notifications for the files
					if (removedEntry != NULL) {
						_NotifyFilesCreatedOrRemoved(removedEntry,
							B_ENTRY_REMOVED);
					}

					if (createdEntry != NULL) {
						_NotifyFilesCreatedOrRemoved(createdEntry,
							B_ENTRY_CREATED);
					}
				} else {
					BString fromPath;
					if (fromDirectoryNode != NULL) {
						fromPath = make_path(_NodePath(fromDirectoryNode),
							fromEntryRef.name);
					}

					BString path;
					if (toDirectoryNode != NULL) {
						path = make_path(_NodePath(toDirectoryNode),
							toEntryRef.name);
					}

					_NotifyEntryMoved(fromEntryRef, toEntryRef, nodeRef,
						fromPath, path, isDirectory, fromDirectoryNode == NULL,
						toDirectoryNode == NULL);
				}

				if (removedEntry != NULL)
					_DeleteEntry(removedEntry, false);
			} else {
				// The entries can't be handled regularly. We delegate all the
				// work to _EntryRemoved() and _EntryCreated(). This will
				// generate separate entry-removed and entry-created
				// notifications.

				// handle removed
				if (fromDirectoryNode != NULL)
					_EntryRemoved(fromEntryRef, nodeRef, false, true, NULL);

				// handle created
				if (toDirectoryNode != NULL) {
					_EntryCreated(toEntryRef, nodeRef, isDirectory, false, true,
						NULL);
				}
			}

			return;
		}

		if (fromAncestor == fBaseAncestor || toAncestor == fBaseAncestor) {
			// That should never happen, as we should have found a matching
			// directory node in this case.
#ifdef DEBUG
			debugger("path ancestor exists, but doesn't have a directory");
			// Could actually be an out-of-memory situation, if we simply failed
			// to create the directory earlier.
#endif
			_StopWatchingAncestors(fRoot, false);
			_StartWatchingAncestors(fRoot, false);
			return;
		}
	} else {
		// Non-recursive mode: This notification is only of interest to us, if
		// it is either a move into/within/out of the path and B_WATCH_DIRECTORY
		// is set, or an ancestor might be affected.
		if (fromAncestor == NULL && toAncestor == NULL)
			return;

		if (fromAncestor == fBaseAncestor || toAncestor == fBaseAncestor) {
			if ((fFlags & B_WATCH_DIRECTORY) != 0) {
				BString fromPath;
				if (fromAncestor == fBaseAncestor)
					fromPath = make_path(fPath, fromEntryRef.name);

				BString path;
				if (toAncestor == fBaseAncestor)
					path = make_path(fPath, toEntryRef.name);

				_NotifyEntryMoved(fromEntryRef, toEntryRef, nodeRef,
					fromPath, path, isDirectory, fromAncestor == NULL,
					toAncestor == NULL);
			}
			return;
		}
	}

	if (fromAncestor == NULL && toAncestor == NULL)
		return;

	if (fromAncestor == NULL) {
		_EntryCreated(toEntryRef, nodeRef, isDirectory, false, true, NULL);
		return;
	}

	if (toAncestor == NULL) {
		_EntryRemoved(fromEntryRef, nodeRef, false, true, NULL);
		return;
	}

	// An entry was moved in a true ancestor directory or between true ancestor
	// directories. Unless the moved entry was or becomes our base ancestor, we
	// let _EntryRemoved() and _EntryCreated() handle it.
	bool fromIsBase = fromAncestor == fBaseAncestor->Parent()
		&& strcmp(fromEntryRef.name, fBaseAncestor->Name()) == 0;
	bool toIsBase = toAncestor == fBaseAncestor->Parent()
		&& strcmp(toEntryRef.name, fBaseAncestor->Name()) == 0;
	if (fromIsBase || toIsBase) {
		// This might be a duplicate notification. Check whether our model
		// already reflects the change. Otherwise stop/start watching the base
		// ancestor as required.
		bool notifyFilesRecursively = _WatchFilesOnly() && isDirectory;
		if (fromIsBase) {
			if (!fBaseAncestor->Exists())
				return;
			_StopWatchingAncestors(fBaseAncestor, notifyFilesRecursively);
		} else {
			if (fBaseAncestor->Exists()) {
				if (fBaseAncestor->NodeRef() == nodeRef
					&& isDirectory == fBaseAncestor->IsDirectory()) {
					return;
				}

				// We're out of sync with reality.
				_StopWatchingAncestors(fBaseAncestor, true);
				_StartWatchingAncestors(fBaseAncestor, true);
				return;
			}

			_StartWatchingAncestors(fBaseAncestor, notifyFilesRecursively);
		}

		if (!notifyFilesRecursively) {
			_NotifyEntryMoved(fromEntryRef, toEntryRef, nodeRef,
				fromIsBase ? fPath.String() : NULL,
				toIsBase ? fPath.String() : NULL,
				isDirectory, toIsBase, fromIsBase);
		}
		return;
	}

	_EntryRemoved(fromEntryRef, nodeRef, false, true, NULL);
	_EntryCreated(toEntryRef, nodeRef, isDirectory, false, true, NULL);
}


void
PathHandler::_NodeChanged(BMessage* message)
{
	node_ref nodeRef;

	if (message->FindInt32("device", &nodeRef.device) != B_OK
		|| message->FindInt64("node", &nodeRef.node) != B_OK) {
		return;
	}

	TRACE("%p->PathHandler::_NodeChanged(): node: %" B_PRIdDEV ":%" B_PRIdINO
		", %s%s\n", this, nodeRef.device, nodeRef.node,
			message->GetInt32("opcode", B_STAT_CHANGED) == B_ATTR_CHANGED
				? "attribute: " : "stat",
			message->GetInt32("opcode", B_STAT_CHANGED) == B_ATTR_CHANGED
				? message->GetString("attr", "") : "");

	bool isDirectory = false;
	BString path;
	if (Ancestor* ancestor = _GetAncestor(nodeRef)) {
		if (ancestor != fBaseAncestor)
			return;
		isDirectory = ancestor->IsDirectory();
		path = fPath;
	} else if (Node* node = _GetNode(nodeRef)) {
		isDirectory = node->IsDirectory();
		path = _NodePath(node);
	} else
		return;

	if (isDirectory ? _WatchFilesOnly() : _WatchDirectoriesOnly())
		return;

	_NotifyTarget(*message, path);
}


bool
PathHandler::_EntryCreated(const NotOwningEntryRef& entryRef,
	const node_ref& nodeRef, bool isDirectory, bool dryRun, bool notify,
	Entry** _entry)
{
	if (_entry != NULL)
		*_entry = NULL;

	Ancestor* ancestor = _GetAncestor(nodeRef);
	if (ancestor != NULL) {
		if (isDirectory == ancestor->IsDirectory()
			&& entryRef == ancestor->EntryRef()) {
			// just a duplicate notification
			TRACE("  -> we already know the ancestor\n");
			return true;
		}

		struct stat ancestorStat;
		if (BEntry(&ancestor->EntryRef()).GetStat(&ancestorStat) == B_OK
			&& node_ref(ancestorStat.st_dev, ancestorStat.st_ino)
				== ancestor->NodeRef()
			&& S_ISDIR(ancestorStat.st_mode) == ancestor->IsDirectory()) {
			// Our information for the ancestor is up-to-date, so ignore the
			// notification.
			TRACE("  -> we know a different ancestor, but our info is "
				"up-to-date\n");
			return true;
		}

		// We're out of sync with reality.
		TRACE("  -> ancestor mismatch -> resyncing\n");
		if (!dryRun) {
			_StopWatchingAncestors(ancestor, true);
			_StartWatchingAncestors(ancestor, true);
		}
		return false;
	}

	ancestor = _GetAncestor(entryRef.DirectoryNodeRef());
	if (ancestor != NULL) {
		if (ancestor != fBaseAncestor) {
			// The directory is a true ancestor -- the notification is only of
			// interest, if the entry matches the child ancestor.
			Ancestor* childAncestor = ancestor->Child();
			if (strcmp(entryRef.name, childAncestor->Name()) != 0) {
				TRACE("  -> not an ancestor entry we're interested in "
					"(\"%s\")\n", childAncestor->Name());
				return true;
			}

			if (!dryRun) {
				if (childAncestor->Exists()) {
					TRACE("  ancestor entry mismatch -> resyncing\n");
					// We're out of sync with reality -- the new entry refers to
					// a different node.
					_StopWatchingAncestors(childAncestor, true);
				}

				TRACE("  -> starting to watch newly appeared ancestor\n");
				_StartWatchingAncestors(childAncestor, true);
			}
			return false;
		}

		// The directory is our path. If watching recursively, just fall
		// through. Otherwise, we want to pass on the notification, if directory
		// watching is enabled.
		if (!_WatchRecursively()) {
			if ((fFlags & B_WATCH_DIRECTORY) != 0) {
				_NotifyEntryCreatedOrRemoved(entryRef, nodeRef,
					make_path(fPath, entryRef.name), isDirectory,
					B_ENTRY_CREATED);
			}
			return true;
		}
	}

	if (!_WatchRecursively()) {
		// That shouldn't happen, since we only watch the ancestors in this
		// case.
		return true;
	}

	Node* directoryNode = _GetNode(entryRef.DirectoryNodeRef());
	if (directoryNode == NULL)
		return true;

	Directory* directory = directoryNode->ToDirectory();
	if (directory == NULL) {
		// We're out of sync with reality.
		if (!dryRun) {
			if (Entry* nodeEntry = directoryNode->FirstNodeEntry()) {
				// remove the entry that is in the way and re-add the proper
				// entry
				NotOwningEntryRef directoryEntryRef = nodeEntry->EntryRef();
				BString directoryName = nodeEntry->Name();
				_DeleteEntry(nodeEntry, true);
				_EntryCreated(directoryEntryRef, entryRef.DirectoryNodeRef(),
					true, false, notify, NULL);
			} else {
				// It's either the base node or something's severely fishy.
				// Resync the whole path.
				_StopWatchingAncestors(fBaseAncestor, true);
				_StartWatchingAncestors(fBaseAncestor, true);
			}
		}

		return false;
	}

	// Check, if there's a colliding entry.
	if (Entry* nodeEntry = directory->FindEntry(entryRef.name)) {
		Node* entryNode = nodeEntry->Node();
		if (entryNode != NULL && entryNode->NodeRef() == nodeRef)
			return true;

		// We're out of sync with reality -- the new entry refers to a different
		// node.
		_DeleteEntry(nodeEntry, true);
	}

	if (dryRun)
		return true;

	_AddEntryIfNeeded(directory, entryRef.name, nodeRef, isDirectory, notify,
		_entry);
	return true;
}


bool
PathHandler::_EntryRemoved(const NotOwningEntryRef& entryRef,
	const node_ref& nodeRef, bool dryRun, bool notify, Entry** _keepEntry)
{
	if (_keepEntry != NULL)
		*_keepEntry = NULL;

	Ancestor* ancestor = _GetAncestor(nodeRef);
	if (ancestor != NULL) {
		// The node is an ancestor. If this is a true match, stop watching the
		// ancestor.
		if (!ancestor->Exists())
			return true;

		if (entryRef != ancestor->EntryRef()) {
			// We might be out of sync with reality -- the new entry refers to a
			// different node.
			struct stat ancestorStat;
			if (BEntry(&ancestor->EntryRef()).GetStat(&ancestorStat) != B_OK) {
				if (!dryRun)
					_StopWatchingAncestors(ancestor, true);
				return false;
			}

			if (node_ref(ancestorStat.st_dev, ancestorStat.st_ino)
					!= ancestor->NodeRef()
				|| S_ISDIR(ancestorStat.st_mode) != ancestor->IsDirectory()) {
				if (!dryRun) {
					_StopWatchingAncestors(ancestor, true);
					_StartWatchingAncestors(ancestor, true);
				}
				return false;
			}
			return true;
		}

		if (!dryRun)
			_StopWatchingAncestors(ancestor, true);
		return false;
	}

	ancestor = _GetAncestor(entryRef.DirectoryNodeRef());
	if (ancestor != NULL) {
		if (ancestor != fBaseAncestor) {
			// The directory is a true ancestor -- the notification cannot be
			// of interest, since the node didn't match a known ancestor.
			return true;
		}

		// The directory is our path. If watching recursively, just fall
		// through. Otherwise, we want to pass on the notification, if directory
		// watching is enabled.
		if (!_WatchRecursively()) {
			if (notify && (fFlags & B_WATCH_DIRECTORY) != 0) {
				_NotifyEntryCreatedOrRemoved(entryRef, nodeRef,
					make_path(fPath, entryRef.name), false, B_ENTRY_REMOVED);
					// We don't know whether this was a directory, but it
					// doesn't matter in this case.
			}
			return true;
		}
	}

	if (!_WatchRecursively()) {
		// That shouldn't happen, since we only watch the ancestors in this
		// case.
		return true;
	}

	Node* directoryNode = _GetNode(entryRef.DirectoryNodeRef());
	if (directoryNode == NULL) {
		// We shouldn't get a notification, if we don't known the directory.
		return true;
	}

	Directory* directory = directoryNode->ToDirectory();
	if (directory == NULL) {
		// We might be out of sync with reality or the notification is just
		// late. The former case is extremely unlikely (we are watching the node
		// and its parent directory after all) and rather hard to verify.
		return true;
	}

	Entry* nodeEntry = directory->FindEntry(entryRef.name);
	if (nodeEntry == NULL) {
		// might be a non-directory node while we're in directories-only mode
		return true;
	}

	if (!dryRun) {
		if (_keepEntry != NULL)
			*_keepEntry = nodeEntry;
		else
			_DeleteEntry(nodeEntry, notify);
	}
	return true;
}


bool
PathHandler::_CheckDuplicateEntryNotification(int32 opcode,
	const entry_ref& toEntryRef, const node_ref& nodeRef,
	const entry_ref* fromEntryRef)
{
	if (opcode == fDuplicateEntryNotificationOpcode
		&& nodeRef == fDuplicateEntryNotificationNodeRef
		&& toEntryRef == fDuplicateEntryNotificationToEntryRef
		&& (fromEntryRef == NULL
			|| *fromEntryRef == fDuplicateEntryNotificationFromEntryRef)) {
		return true;
	}

	fDuplicateEntryNotificationOpcode = opcode;
	fDuplicateEntryNotificationNodeRef = nodeRef;
	fDuplicateEntryNotificationToEntryRef = toEntryRef;
	fDuplicateEntryNotificationFromEntryRef = fromEntryRef != NULL
		? *fromEntryRef : entry_ref();
	return false;
}


void
PathHandler::_UnsetDuplicateEntryNotification()
{
	fDuplicateEntryNotificationOpcode = B_STAT_CHANGED;
	fDuplicateEntryNotificationNodeRef = node_ref();
	fDuplicateEntryNotificationFromEntryRef = entry_ref();
	fDuplicateEntryNotificationToEntryRef = entry_ref();
}


Ancestor*
PathHandler::_GetAncestor(const node_ref& nodeRef) const
{
	return fAncestors.Lookup(nodeRef);
}


status_t
PathHandler::_AddNode(const node_ref& nodeRef, bool isDirectory, bool notify,
	Entry* entry, Node** _node)
{
	TRACE("%p->PathHandler::_AddNode(%" B_PRIdDEV ":%" B_PRIdINO
		", isDirectory: %d, notify: %d)\n", this, nodeRef.device, nodeRef.node,
		isDirectory, notify);

	// If hard links are supported, we may already know the node.
	Node* node = _GetNode(nodeRef);
	if (node != NULL) {
		if (entry != NULL) {
			entry->SetNode(node);
			node->AddNodeEntry(entry);
		}

		if (_node != NULL)
			*_node = node;
		return B_OK;
	}

	// create the node
	Directory* directoryNode = NULL;
	if (isDirectory)
		node = directoryNode = Directory::Create(nodeRef);
	else
		node = new(std::nothrow) Node(nodeRef);

	if (node == NULL)
		return B_NO_MEMORY;

	ObjectDeleter<Node> nodeDeleter(node);

	// start watching (don't do that for the base node, since we watch it
	// already via fBaseAncestor)
	if (nodeRef != fBaseAncestor->NodeRef()) {
		uint32 flags = (fFlags & WATCH_NODE_FLAG_MASK) | B_WATCH_DIRECTORY;
		status_t error = sWatchingInterface->WatchNode(&nodeRef, flags, this);
		if (error != B_OK)
			return error;
	}

	fNodes.Insert(nodeDeleter.Detach());

	if (entry != NULL) {
		entry->SetNode(node);
		node->AddNodeEntry(entry);
	}

	if (_node != NULL)
		*_node = node;

	if (!isDirectory)
		return B_OK;

	// recursively add the directory's descendents
	BDirectory directory;
	if (directory.SetTo(&nodeRef) != B_OK) {
		if (_node != NULL)
			*_node = node;
		return B_OK;
	}

	entry_ref entryRef;
	while (directory.GetNextRef(&entryRef) == B_OK) {
		struct stat st;
		if (BEntry(&entryRef).GetStat(&st) != B_OK)
			continue;

		bool isDirectory = S_ISDIR(st.st_mode);
		status_t error = _AddEntryIfNeeded(directoryNode, entryRef.name,
			node_ref(st.st_dev, st.st_ino), isDirectory, notify);
		if (error != B_OK) {
			TRACE("%p->PathHandler::_AddNode(%" B_PRIdDEV ":%" B_PRIdINO
				", isDirectory: %d, notify: %d): failed to add directory "
				"entry: \"%s\"\n", this, nodeRef.device, nodeRef.node,
				isDirectory, notify, entryRef.name);
			continue;
		}
	}

	return B_OK;
}


void
PathHandler::_DeleteNode(Node* node, bool notify)
{
	if (Directory* directory = node->ToDirectory()) {
		Entry* entry = directory->RemoveAllEntries();
		while (entry != NULL) {
			Entry* nextEntry = entry->HashNext();
			_DeleteEntryAlreadyRemovedFromParent(entry, notify);
			entry = nextEntry;
		}
	}

	if (node->NodeRef() != fBaseAncestor->NodeRef())
		sWatchingInterface->WatchNode(&node->NodeRef(), B_STOP_WATCHING, this);

	fNodes.Remove(node);
	delete node;
}


Node*
PathHandler::_GetNode(const node_ref& nodeRef) const
{
	return fNodes.Lookup(nodeRef);
}


status_t
PathHandler::_AddEntryIfNeeded(Directory* directory, const char* name,
	const node_ref& nodeRef, bool isDirectory, bool notify,
	Entry** _entry)
{
	TRACE("%p->PathHandler::_AddEntryIfNeeded(%" B_PRIdDEV ":%" B_PRIdINO
		":\"%s\", %" B_PRIdDEV ":%" B_PRIdINO
		", isDirectory: %d, notify: %d)\n", this, directory->NodeRef().device,
		directory->NodeRef().node, name, nodeRef.device, nodeRef.node,
		isDirectory, notify);

	if (!isDirectory && _WatchDirectoriesOnly()) {
		if (_entry != NULL)
			*_entry = NULL;
		return B_OK;
	}

	Entry* entry = directory->CreateEntry(name, NULL);
	if (entry == NULL)
		return B_NO_MEMORY;

	status_t error = _AddNode(nodeRef, isDirectory, notify && _WatchFilesOnly(),
		entry);
	if (error != B_OK) {
		directory->RemoveEntry(entry);
		delete entry;
		return error;
	}

	if (notify)
		_NotifyEntryCreatedOrRemoved(entry, B_ENTRY_CREATED);

	if (_entry != NULL)
		*_entry = entry;
	return B_OK;
}


void
PathHandler::_DeleteEntry(Entry* entry, bool notify)
{
	entry->Parent()->RemoveEntry(entry);
	_DeleteEntryAlreadyRemovedFromParent(entry, notify);
}


void
PathHandler::_DeleteEntryAlreadyRemovedFromParent(Entry* entry, bool notify)
{
	if (notify)
		_NotifyEntryCreatedOrRemoved(entry, B_ENTRY_REMOVED);

	Node* node = entry->Node();
	if (node->IsOnlyNodeEntry(entry))
		_DeleteNode(node, notify && _WatchFilesOnly());

	delete entry;
}


void
PathHandler::_NotifyFilesCreatedOrRemoved(Entry* entry, int32 opcode) const
{
	Directory* directory = entry->Node()->ToDirectory();
	if (directory == NULL) {
		_NotifyEntryCreatedOrRemoved(entry, opcode);
		return;
	}

	for (EntryMap::Iterator it = directory->GetEntryIterator(); it.HasNext();)
		_NotifyFilesCreatedOrRemoved(it.Next(), opcode);
}


void
PathHandler::_NotifyEntryCreatedOrRemoved(Entry* entry, int32 opcode) const
{
	Node* node = entry->Node();
	_NotifyEntryCreatedOrRemoved(
		NotOwningEntryRef(entry->Parent()->NodeRef(), entry->Name()),
		node->NodeRef(), _EntryPath(entry), node->IsDirectory(), opcode);
}


void
PathHandler::_NotifyEntryCreatedOrRemoved(const entry_ref& entryRef,
	const node_ref& nodeRef, const char* path, bool isDirectory, int32 opcode)
	const
{
	if (isDirectory ? _WatchFilesOnly() : _WatchDirectoriesOnly())
		return;

	TRACE("%p->PathHandler::_NotifyEntryCreatedOrRemoved(): entry %s: %"
		B_PRIdDEV ":%" B_PRIdINO ":\"%s\", node: %" B_PRIdDEV ":%" B_PRIdINO
		"\n", this, opcode == B_ENTRY_CREATED ? "created" : "removed",
		entryRef.device, entryRef.directory, entryRef.name, nodeRef.device,
		nodeRef.node);

	BMessage message(B_PATH_MONITOR);
	message.AddInt32("opcode", opcode);
	message.AddInt32("device", entryRef.device);
	message.AddInt64("directory", entryRef.directory);
	message.AddInt32("node device", nodeRef.device);
		// This field is not in a usual node monitoring message, since the node
		// the created/removed entry refers to always belongs to the same FS as
		// the directory, as another FS cannot yet/no longer be mounted there.
		// In our case, however, this can very well be the case, e.g. when the
		// the notification is triggered in response to a directory tree having
		// been moved into/out of our path.
	message.AddInt64("node", nodeRef.node);
	message.AddString("name", entryRef.name);

	_NotifyTarget(message, path);
}


void
PathHandler::_NotifyEntryMoved(const entry_ref& fromEntryRef,
	const entry_ref& toEntryRef, const node_ref& nodeRef, const char* fromPath,
	const char* path, bool isDirectory, bool wasAdded, bool wasRemoved) const
{
	if ((isDirectory && _WatchFilesOnly())
		|| (!isDirectory && _WatchDirectoriesOnly())) {
		return;
	}

	TRACE("%p->PathHandler::_NotifyEntryMoved(): entry: %" B_PRIdDEV ":%"
		B_PRIdINO ":\"%s\" -> %" B_PRIdDEV ":%" B_PRIdINO ":\"%s\", node: %"
		B_PRIdDEV ":%" B_PRIdINO "\n", this, fromEntryRef.device,
		fromEntryRef.directory, fromEntryRef.name, toEntryRef.device,
		toEntryRef.directory, toEntryRef.name, nodeRef.device, nodeRef.node);

	BMessage message(B_PATH_MONITOR);
	message.AddInt32("opcode", B_ENTRY_MOVED);
	message.AddInt32("device", fromEntryRef.device);
	message.AddInt64("from directory", fromEntryRef.directory);
	message.AddInt64("to directory", toEntryRef.directory);
	message.AddInt32("node device", nodeRef.device);
	message.AddInt64("node", nodeRef.node);
	message.AddString("from name", fromEntryRef.name);
	message.AddString("name", toEntryRef.name);

	if (wasAdded)
		message.AddBool("added", true);
	if (wasRemoved)
		message.AddBool("removed", true);

	if (fromPath != NULL && fromPath[0] != '\0')
		message.AddString("from path", fromPath);

	_NotifyTarget(message, path);
}


void
PathHandler::_NotifyTarget(BMessage& message, const char* path) const
{
	message.what = B_PATH_MONITOR;
	if (path != NULL && path[0] != '\0')
		message.AddString("path", path);
	message.AddString("watched_path", fPath.String());
	fTarget.SendMessage(&message);
}



BString
PathHandler::_NodePath(const Node* node) const
{
	if (Entry* entry = node->FirstNodeEntry())
		return _EntryPath(entry);
	return node == fBaseNode ? fPath : BString();
}


BString
PathHandler::_EntryPath(const Entry* entry) const
{
	return make_path(_NodePath(entry->Parent()), entry->Name());
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
PathHandler::_WatchDirectoriesOnly() const
{
	return (fFlags & B_WATCH_DIRECTORIES_ONLY) != 0;
}


} // namespace


//	#pragma mark - BPathMonitor


namespace BPrivate {


BPathMonitor::BPathMonitor()
{
}


BPathMonitor::~BPathMonitor()
{
}


/*static*/ status_t
BPathMonitor::StartWatching(const char* path, uint32 flags,
	const BMessenger& target)
{
	TRACE("BPathMonitor::StartWatching(%s, %" B_PRIx32 ")\n", path, flags);

	if (path == NULL || path[0] == '\0')
		return B_BAD_VALUE;

	// B_WATCH_FILES_ONLY and B_WATCH_DIRECTORIES_ONLY are mutual exclusive
	if ((flags & B_WATCH_FILES_ONLY) != 0
		&& (flags & B_WATCH_DIRECTORIES_ONLY) != 0) {
		return B_BAD_VALUE;
	}

	status_t status = _InitIfNeeded();
	if (status != B_OK)
		return status;

	BAutolock _(sLooper);

	Watcher* watcher = sWatchers->Lookup(target);
	bool newWatcher = false;
	if (watcher != NULL) {
		// If there's already a handler for the path, we'll replace it, but
		// add its flags.
		if (PathHandler* handler = watcher->Lookup(path)) {
			// keep old flags save for conflicting mutually exclusive ones
			uint32 oldFlags = handler->Flags();
			const uint32 kMutuallyExclusiveFlags
				= B_WATCH_FILES_ONLY | B_WATCH_DIRECTORIES_ONLY;
			if ((flags & kMutuallyExclusiveFlags) != 0)
				oldFlags &= ~(uint32)kMutuallyExclusiveFlags;
			flags |= oldFlags;

			watcher->Remove(handler);
			handler->Quit();
		}
	} else {
		watcher = Watcher::Create(target);
		if (watcher == NULL)
			return B_NO_MEMORY;
		sWatchers->Insert(watcher);
		newWatcher = true;
	}

	PathHandler* handler = new (std::nothrow) PathHandler(path, flags, target,
		sLooper);
	status = handler != NULL ? handler->InitCheck() : B_NO_MEMORY;

	if (status != B_OK) {
		if (handler != NULL)
			handler->Quit();

		if (newWatcher) {
			sWatchers->Remove(watcher);
			delete watcher;
		}
		return status;
	}

	watcher->Insert(handler);
	return B_OK;
}


/*static*/ status_t
BPathMonitor::StopWatching(const char* path, const BMessenger& target)
{
	if (sLooper == NULL)
		return B_BAD_VALUE;

	TRACE("BPathMonitor::StopWatching(%s)\n", path);

	BAutolock _(sLooper);

	Watcher* watcher = sWatchers->Lookup(target);
	if (watcher == NULL)
		return B_BAD_VALUE;

	PathHandler* handler = watcher->Lookup(path);
	if (handler == NULL)
		return B_BAD_VALUE;

	watcher->Remove(handler);
	handler->Quit();

	if (watcher->IsEmpty()) {
		sWatchers->Remove(watcher);
		delete watcher;
	}

	return B_OK;
}


/*static*/ status_t
BPathMonitor::StopWatching(const BMessenger& target)
{
	if (sLooper == NULL)
		return B_BAD_VALUE;

	BAutolock _(sLooper);

	Watcher* watcher = sWatchers->Lookup(target);
	if (watcher == NULL)
		return B_BAD_VALUE;

	// delete handlers
	PathHandler* handler = watcher->Clear(true);
	while (handler != NULL) {
		PathHandler* nextHandler = handler->HashNext();
		handler->Quit();
		handler = nextHandler;
	}

	sWatchers->Remove(watcher);
	delete watcher;

	return B_OK;
}


/*static*/ void
BPathMonitor::SetWatchingInterface(BWatchingInterface* watchingInterface)
{
	sWatchingInterface = watchingInterface != NULL
		? watchingInterface : sDefaultWatchingInterface;
}


/*static*/ status_t
BPathMonitor::_InitIfNeeded()
{
	pthread_once(&sInitOnce, &BPathMonitor::_Init);
	return sLooper != NULL ? B_OK : B_NO_MEMORY;
}


/*static*/ void
BPathMonitor::_Init()
{
	sDefaultWatchingInterface = new(std::nothrow) BWatchingInterface;
	if (sDefaultWatchingInterface == NULL)
		return;

	sWatchers = new(std::nothrow) WatcherMap;
	if (sWatchers == NULL || sWatchers->Init() != B_OK)
		return;

	if (sWatchingInterface == NULL)
		SetWatchingInterface(sDefaultWatchingInterface);

	BLooper* looper = new (std::nothrow) BLooper("PathMonitor looper");
	TRACE("Start PathMonitor looper\n");
	if (looper == NULL)
		return;
	thread_id thread = looper->Run();
	if (thread < 0) {
		delete looper;
		return;
	}

	sLooper = looper;
}


// #pragma mark - BWatchingInterface


BPathMonitor::BWatchingInterface::BWatchingInterface()
{
}


BPathMonitor::BWatchingInterface::~BWatchingInterface()
{
}


status_t
BPathMonitor::BWatchingInterface::WatchNode(const node_ref* node, uint32 flags,
	const BMessenger& target)
{
	return watch_node(node, flags, target);
}


status_t
BPathMonitor::BWatchingInterface::WatchNode(const node_ref* node, uint32 flags,
	const BHandler* handler, const BLooper* looper)
{
	return watch_node(node, flags, handler, looper);
}


status_t
BPathMonitor::BWatchingInterface::StopWatching(const BMessenger& target)
{
	return stop_watching(target);
}


status_t
BPathMonitor::BWatchingInterface::StopWatching(const BHandler* handler,
	const BLooper* looper)
{
	return stop_watching(handler, looper);
}


}	// namespace BPrivate
