// StatCacheServer.cpp

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <Application.h>
#include <Autolock.h>
#include <Message.h>
#include <NodeMonitor.h>
#include <Path.h>

#include "StatCacheServer.h"
#include "StatCacheServerImpl.h"

//#define DBG(x) { x; }
#define DBG(x)
#define OUT printf

static const int32 kMaxSymlinks = 32;

// node monitor constants
static const int32 kDefaultNodeMonitorLimit = 4096;
static const int32 kNodeMonitorLimitIncrement = 512;

// private BeOS syscall to set the  node monitor slot limits
extern "C" int _kset_mon_limit_(int num);

// get_dirent_size
static inline
int32
get_dirent_size(const char *name)
{
	dirent *dummy = NULL;
	int32 entrySize = (dummy->d_name + strlen(name) + 1) - (char*)dummy;
	return (entrySize + 3) & ~0x3;
}

// node_ref_hash
static inline
uint32
node_ref_hash(dev_t device, ino_t node)
{
	uint32 hash = device;
	hash = hash * 17 + (uint32)node;
	hash = hash * 17 + (uint32)(node >> 32);
	return hash;
}

// string_hash
//
// from the Dragon Book: a slightly modified hashpjw()
static inline
uint32
string_hash(const char *name)
{
	uint32 h = 0;
	if (name) {
		for (; *name; name++) {
			uint32 g = h & 0xf0000000;
			if (g)
				h ^= g >> 24;
			h = (h << 4) + *name;
		}
	}
	return h;
}

// NodeRefHash
size_t
NodeRefHash::operator()(const node_ref &nodeRef) const
{
	return node_ref_hash(nodeRef.device, nodeRef.node);
}
 
// EntryRefHash
size_t
EntryRefHash::operator()(const entry_ref &entryRef) const
{
	uint32 hash = node_ref_hash(entryRef.device, entryRef.directory);
	hash = hash * 17 + string_hash(entryRef.name);
	return hash;
}


// #pragma mark -

// constructor
Entry::Entry()
	: Referencable(),
	  fParent(NULL),
	  fName(),
	  fNode(NULL),
	  fPrevious(NULL),
	  fNext(NULL)
{
}

// destructor
Entry::~Entry()
{
	SetNode(NULL);
}

// SetTo
status_t
Entry::SetTo(Directory *parent, const char *name)
{
	fParent = parent;
	fName = name;
	return B_OK;
}

// GetParent
Directory *
Entry::GetParent() const
{
	return fParent;
}

// GetName
const char *
Entry::GetName() const
{
	return fName.c_str();
}

// SetNode
void
Entry::SetNode(Node *node)
{
	if (fNode != node) {
		if (fNode)
			fNode->RemoveReference();
		fNode = node;
		if (fNode) {
			fNode->AddReference();
			if (!fNode->GetEntry())
				fNode->SetEntry(this);
		}
	}
}

// GetNode
Node *
Entry::GetNode() const
{
	return fNode;
}

// SetPrevious
void
Entry::SetPrevious(Entry *entry)
{
	fPrevious = entry;
}

// GetPrevious
Entry *
Entry::GetPrevious() const
{
	return fPrevious;
}

// SetNext
void
Entry::SetNext(Entry *entry)
{
	fNext = entry;
}

// GetNext
Entry *
Entry::GetNext() const
{
	return fNext;
}

// GetEntryRef
entry_ref
Entry::GetEntryRef() const
{
	node_ref dirRef(fParent->GetNodeRef());
	return entry_ref(dirRef.device, dirRef.node, fName.c_str());
}

// GetPath
status_t
Entry::GetPath(string& path)
{
	if (!fParent)
		return B_ERROR;

	// get directory path
	status_t error = fParent->GetPath(path);
	if (error != B_OK)
		return error;

	// append the entry name
	if (path[path.length() - 1] != '/')
		path += '/';
	path += fName;

	return B_OK;
}

// Unreferenced
void
Entry::Unreferenced()
{
	NodeManager::GetDefault()->EntryUnreferenced(this);
}


// #pragma mark -

// constructor
Node::Node(const struct stat &st)
	: Referencable(),
	  fEntry(NULL),
	  fStat(st),
	  fStatValid(false)
{
}

// destructor
Node::~Node()
{
	// stop watching the node
	NodeManager::GetDefault()->StopWatching(this);

	SetEntry(NULL);
}

// SetTo
status_t
Node::SetTo(Entry *entry)
{
	// start watching the node
	status_t error = NodeManager::GetDefault()->StartWatching(this);
	if (error != B_OK)
		return error;

	SetEntry(entry);

	// update the stat
	return UpdateStat();
}

// GetPath
status_t
Node::GetPath(string& path)
{
	if (this == NodeManager::GetDefault()->GetRootDirectory()) {
		path = "/";
		return B_OK;
	}

	if (!fEntry)
		return B_ERROR;
	return fEntry->GetPath(path);
}

// GetStat
const struct stat &
Node::GetStat() const
{
	return fStat;
}

// GetStat
status_t
Node::GetStat(struct stat *st)
{
	if (!fStatValid) {
		status_t error = UpdateStat();
		if (error != B_OK)
			return error;
	}
	*st = fStat;
	return B_OK;
}

// UpdateStat
status_t
Node::UpdateStat()
{
	// get path
	string path;
	status_t error = GetPath(path);
	if (error != B_OK)
		return error;

DBG(OUT("disk access: lstat(): %s\n", path.c_str()));

	// read stat
	if (lstat(path.c_str(), &fStat) < 0)
		return errno;
	fStatValid = true;
	return B_OK;
}

// MarkStatInvalid
void
Node::MarkStatInvalid()
{
	fStatValid = false;
}

// SetEntry
void
Node::SetEntry(Entry *entry)
{
	if (entry != fEntry) {
		if (fEntry)
			fEntry->RemoveReference();
		fEntry = entry;
		if (fEntry)
			fEntry->AddReference();
	}
}

// GetEntry
Entry *
Node::GetEntry() const
{
	return fEntry;
}

// GetNodeRef
node_ref
Node::GetNodeRef() const
{
	node_ref nodeRef;
	nodeRef.device = fStat.st_dev;
	nodeRef.node = fStat.st_ino;
	return nodeRef;
}

// Unreferenced
void
Node::Unreferenced()
{
	NodeManager::GetDefault()->NodeUnreferenced(this);
}


// #pragma mark -

// constructor
Directory::Directory(const struct stat &st)
	: Node(st),
	  fFirstEntry(NULL),
	  fLastEntry(NULL),
	  fIsComplete(false)
{
}

// destructor
Directory::~Directory()
{
	while (Entry *entry = GetFirstEntry())
		RemoveEntry(entry);
}

// SetTo
status_t
Directory::SetTo(Entry *entry)
{
	return Node::SetTo(entry);
}

// FindEntry
status_t
Directory::FindEntry(const char *name, Entry **entry)
{
	entry_ref ref(fStat.st_dev, fStat.st_ino, name);
	if (!fIsComplete)
		return NodeManager::GetDefault()->CreateEntry(ref, entry);
	*entry = NodeManager::GetDefault()->GetEntry(ref);
	return (*entry ? B_OK : B_ENTRY_NOT_FOUND);
}

// GetFirstEntry
Entry *
Directory::GetFirstEntry() const
{
	return fFirstEntry;
}

// GetNextEntry
Entry *
Directory::GetNextEntry(Entry *entry) const
{
	return (entry ? entry->GetNext() : NULL);
}

// ReadAllEntries
status_t
Directory::ReadAllEntries()
{
	if (fIsComplete)
		return B_OK;

	// get the path
	string path;
	status_t error = GetPath(path);
	if (error != B_OK)
		return error;

DBG(OUT("disk access: opendir(): %s\n", path.c_str()));

	// open the directory
	DIR *dir = opendir(path.c_str());
	if (!dir)
		return errno;

	// read the directory
	while (dirent *entry = readdir(dir)) {
		Entry *dummy;
		FindEntry(entry->d_name, &dummy);
	}
	closedir(dir);

	fIsComplete = true;

	return B_OK;
}

// IsComplete
bool
Directory::IsComplete() const
{
	return fIsComplete;
}

// AddEntry
void
Directory::AddEntry(Entry *entry)
{
	if (fLastEntry) {
		entry->SetPrevious(fLastEntry);
		entry->SetNext(NULL);
		fLastEntry->SetNext(entry);
		fLastEntry = entry;
	} else {
		entry->SetPrevious(NULL);
		entry->SetNext(NULL);
		fFirstEntry = fLastEntry = entry;
	}
	entry->AddReference();

	// the reference the "." entry has, shall be ignored
	if (strcmp(entry->GetName(), ".") == 0)
		fReferenceBaseCount++;
}

// RemoveEntry
void
Directory::RemoveEntry(Entry *entry)
{
	if (entry->GetParent() != this)
		return;

	// the reference the "." entry has, shall be ignored
	if (strcmp(entry->GetName(), ".") == 0)
		fReferenceBaseCount--;

	if (entry->GetPrevious())
		entry->GetPrevious()->SetNext(entry->GetNext());
	else
		fFirstEntry = entry->GetNext();
	if (entry->GetNext())
		entry->GetNext()->SetPrevious(entry->GetPrevious());
	else
		fLastEntry = entry->GetPrevious();
	entry->SetPrevious(NULL);
	entry->SetNext(NULL);
	entry->RemoveReference();
}


// #pragma mark -

// constructor
SymLink::SymLink(const struct stat &st)
	: Node(st),
	  fTarget()
{
}

// destructor
SymLink::~SymLink()
{
}

// SetTo
status_t
SymLink::SetTo(Entry *entry)
{
	// node initialization
	status_t error = Node::SetTo(entry);
	if (error != B_OK)
		return error;

	// get the entry path
	string path;
	error = entry->GetPath(path);
	if (error != B_OK)
		return error;

	// read the link
	char target[B_PATH_NAME_LENGTH + 1];
	ssize_t bytesRead = readlink(path.c_str(), target, B_PATH_NAME_LENGTH);
	if (bytesRead < 0)
		return errno;
	target[bytesRead] = '\0';
	fTarget = target;
	return B_OK;
}

// GetTarget
const char *
SymLink::GetTarget() const
{
	return fTarget.c_str();
}


// #pragma mark -

// destructor
NodeMonitor::NodeMonitor()
	// higher priority and larger queue, since we must not miss update events
	: BLooper("node monitor", B_DISPLAY_PRIORITY, 1000),
	  fCurrentNodeMonitorLimit(kDefaultNodeMonitorLimit),
	  fMessageCountSem(-1)
{
}

// destructor
NodeMonitor::~NodeMonitor()
{
	delete_sem(fMessageCountSem);
}

// Init
status_t
NodeMonitor::Init()
{
	fMessageCountSem = create_sem(0, "nm message count");
	if (fMessageCountSem < 0)
		return fMessageCountSem;
	return B_OK;
}

// MessageReceived
void
NodeMonitor::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
			DetachCurrentMessage();
			fMessageQueue.AddMessage(message);
			release_sem(fMessageCountSem);
			break;
		default:
			BLooper::MessageReceived(message);
	}
}

// StartWatching
status_t
NodeMonitor::StartWatching(Node *node)
{
	if (!node)
		return B_BAD_VALUE;
	uint32 flags = B_WATCH_STAT;
	if (S_ISDIR(node->GetStat().st_mode))
		flags |= B_WATCH_DIRECTORY;
	node_ref ref = node->GetNodeRef();
	status_t error = watch_node(&ref, flags, this);
	// If starting to watch the node fail, we allocate more node
	// monitoring slots and try again.
	if (error != B_OK) {
		fCurrentNodeMonitorLimit += kNodeMonitorLimitIncrement;
		error = _kset_mon_limit_(fCurrentNodeMonitorLimit);
		if (error == B_OK)
			error = watch_node(&ref, flags, this);
	}
	return error;
}

// StopWatching
status_t
NodeMonitor::StopWatching(Node *node)
{
	if (!node)
		return B_BAD_VALUE;
	node_ref ref = node->GetNodeRef();
	return watch_node(&ref, B_STOP_WATCHING, this);
}

// GetNextMonitoringMessage
status_t
NodeMonitor::GetNextMonitoringMessage(BMessage **_message)
{
	// acquire the semaphore
	status_t error = B_OK;
	do {
		error = acquire_sem(fMessageCountSem);
	} while (error == B_INTERRUPTED);
	if (error != B_OK)
		return error;

	// get the message
	BMessage *message = fMessageQueue.NextMessage();
	if (!message)
		return B_ERROR;
	*_message = message;
	return B_OK;
}


// #pragma mark -

// constructor
PathResolver::PathResolver()
	: fSymLinkCounter(0)
{
}

// FindEntry
status_t
PathResolver::FindEntry(const char *path, bool traverse, Entry **_entry)
{
	return FindEntry(NULL, path, traverse, _entry);
}

// FindEntry
status_t
PathResolver::FindEntry(Entry *entry, const char *path, bool traverse,
	Entry **_entry)
{
	// we accept only absolute paths, if no entry was given
	if (!path || (!entry && *path != '/'))
		return B_BAD_VALUE;

	// get the root directory for absolute paths
	if (*path == '/') {
		entry = NodeManager::GetDefault()->GetRootDirectory()->GetEntry();
		// skip '/'
		while (*path == '/')
			path++;
	}

	while (*path != '\0') {
		// get path component
		int componentLen;
		if (char *nextSlash = strchr(path, '/'))
			componentLen = nextSlash - path;
		else
			componentLen = strlen(path);
		string component(path, componentLen);
		path += componentLen;

		// resolve symlink
		Node *node = entry->GetNode();
		if (SymLink *symlink = dynamic_cast<SymLink*>(node)) {
			status_t error = ResolveSymlink(symlink, &node);
			if (error != B_OK)
				return error;
		}

		// find the entry
		if (Directory *dir = dynamic_cast<Directory*>(node)) {
			status_t error = dir->FindEntry(component.c_str(), &entry);
			if (error != B_OK)
				return error;
		} else
			return B_ENTRY_NOT_FOUND;

		// skip '/'
		while (*path == '/')
			path++;
	}

	// traverse leaf symlink, if requested
	if (traverse) {
		status_t error = ResolveSymlink(entry, &entry);
		if (error != B_OK)
			return error;
	}

	*_entry = entry;
	return B_OK;
}

// FindNode
status_t
PathResolver::FindNode(const char *path, bool traverse, Node **node)
{
	Entry *entry;
	status_t error = FindEntry(path, traverse, &entry);
	if (error != B_OK)
		return error;
	if (!entry->GetNode())
		return B_ENTRY_NOT_FOUND;
	*node = entry->GetNode();
	return B_OK;
}

// ResolveSymlink
status_t
PathResolver::ResolveSymlink(Node *node, Node **_node)
{
	Entry *entry;
	status_t error = ResolveSymlink(node, &entry);
	if (error != B_OK)
		return error;
	if (!entry->GetNode())
		return B_ENTRY_NOT_FOUND;
	*_node = entry->GetNode();
	return B_OK;
}

// ResolveSymlink
status_t
PathResolver::ResolveSymlink(Node *node, Entry **entry)
{
	return ResolveSymlink(node->GetEntry(), entry);
}

// ResolveSymlink
status_t
PathResolver::ResolveSymlink(Entry *entry, Entry **_entry)
{
	if (!entry->GetNode())
		return B_ENTRY_NOT_FOUND;

	SymLink *symlink = dynamic_cast<SymLink*>(entry->GetNode());
	if (!symlink) {
		*_entry = entry;
		return B_OK;
	}

	if (fSymLinkCounter > kMaxSymlinks)
		return B_LINK_LIMIT;

	const char *target = symlink->GetTarget();
	if (!target || !symlink->GetEntry() || !symlink->GetEntry()->GetParent()
		|| !symlink->GetEntry()->GetParent()->GetEntry())
		return B_ENTRY_NOT_FOUND;

	fSymLinkCounter++;
	status_t error = FindEntry(symlink->GetEntry()->GetParent()->GetEntry(),
		target, true, _entry);
	fSymLinkCounter--;
	return error;
}


// #pragma mark -

// constructor
NodeManager::NodeManager()
	: BLocker("node manager"),
	  fRootDirectory(NULL),
	  fNodeMonitor(NULL),
	  fNodeMonitoringProcessor(-1)
{
}

// destructor
NodeManager::~NodeManager()
{
	if (fNodeMonitor) {
		fNodeMonitor->Lock();
		fNodeMonitor->Quit();
	}

	if (fNodeMonitoringProcessor >= 0) {
		status_t status;
		wait_for_thread(fNodeMonitoringProcessor, &status);
	}
}

// GetDefault
NodeManager *
NodeManager::GetDefault()
{
	return &sManager;
}

// Init
status_t
NodeManager::Init()
{
	// create the node monitor
	fNodeMonitor = new NodeMonitor;
	status_t error = fNodeMonitor->Init();
	if (error != B_OK)
		return error;
	fNodeMonitor->Run();

	// spawn the node monitoring processor
	fNodeMonitoringProcessor = spawn_thread(&_NodeMonitoringProcessorEntry,
		"node monitoring processor", B_NORMAL_PRIORITY, this);
	if (fNodeMonitoringProcessor < 0)
		return fNodeMonitoringProcessor;
	resume_thread(fNodeMonitoringProcessor);

	// get root dir stat
	struct stat st;
	if (lstat("/", &st) < 0)
		return errno;

	// create the root node
	node_ref nodeRef;
	nodeRef.device = st.st_dev;
	nodeRef.node = st.st_ino;
	fRootDirectory = new Directory(st);
	fNodes[nodeRef] = fRootDirectory;

	// create an entry pointing to the root node
	entry_ref entryRef(st.st_dev, st.st_ino, ".");
	Entry *entry = new Entry;
	error = entry->SetTo(fRootDirectory, ".");
	if (error != B_OK)
		return error;
	entry->SetNode(fRootDirectory);
	fEntries[entryRef] = entry;

	// now we can initialize the root directory
	error = fRootDirectory->SetTo(entry);

	return error;
}

// GetRootDirectory
Directory *
NodeManager::GetRootDirectory() const
{
	return fRootDirectory;
}

// GetNode
Node *
NodeManager::GetNode(const node_ref &nodeRef)
{
	NodeMap::iterator it = fNodes.find(nodeRef);
	if (it == fNodes.end())
		return NULL;
	return it->second;
}

// GetEntry
Entry *
NodeManager::GetEntry(const entry_ref &entryRef)
{
	EntryMap::iterator it = fEntries.find(entryRef);
	if (it == fEntries.end())
		return NULL;
	return it->second;
}

// CreateEntry
status_t
NodeManager::CreateEntry(const entry_ref &entryRef, Entry **_entry)
{
	Entry *entry = GetEntry(entryRef);
	if (!entry) {
		// entry does not yet exist -- create it

		// get the parent directory
		node_ref parentDirRef;
		parentDirRef.device = entryRef.device;
		parentDirRef.node = entryRef.directory;
		Directory *dir;
		status_t error = CreateDirectory(parentDirRef, &dir);
		if (error != B_OK)
			return error;

		// if the directory hasn't created it, we need to do that now
		entry = GetEntry(entryRef);
		if (!entry) {
			entry = new Entry;
			error = entry->SetTo(dir, entryRef.name);
			if (error != B_OK) {
				delete entry;
				return error;
			}

			// get the entry's node
			Node *node;
			error = NodeManager::GetDefault()->_CreateNode(entry, &node);
			if (error != B_OK) {
				delete entry;
				return error;
			}
			entry->SetNode(node);
			node->RemoveReference();

			// initialization successful: add the entry to the dir and to the
			// entry map
			dir->AddEntry(entry);
			fEntries[entryRef] = entry;
			entry->RemoveReference();
DBG(
string path;
entry->GetPath(path);
OUT("entry created: `%s'\n", path.c_str());
)
		}
	}

	*_entry = entry;
	return B_OK;
}

// CreateDirectory
status_t
NodeManager::CreateDirectory(const node_ref &nodeRef, Directory **_dir)
{
	Node *node = GetNode(nodeRef);
	if (!node) {
		// node not yet known -- load the directory
		// get the full path
		entry_ref entryRef(nodeRef.device, nodeRef.node, ".");
		BPath path;
		status_t error = path.SetTo(&entryRef);
		if (error != B_OK)
			return error;

		// find the node
		error = PathResolver().FindNode(path.Path(), false, &node);
		if (error != B_OK)
			return error;
	}

	// node found -- check, if it is a directory
	Directory *dir = dynamic_cast<Directory*>(node);
	if (!dir)
		return B_NOT_A_DIRECTORY;

	*_dir = dir;
	return B_OK;
}

// RemoveEntry
void
NodeManager::RemoveEntry(Entry *entry)
{
	if (!entry)
		return;

DBG(
string path;
entry->GetPath(path);
OUT("entry removed: `%s'\n", path.c_str());
)

	// get a temporary reference, so that the entry will not be deleted when
	// we unset the node
	entry->AddReference();

	// remove from directory and node
	if (entry->GetParent())
		entry->GetParent()->RemoveEntry(entry);

	// detach from node
	Node *node = entry->GetNode();
	if (node) {
		if (node->GetEntry() == entry)
			node->SetEntry(NULL);
		entry->SetNode(NULL);
	}

	// surrender our temporary reference: now the entry should be unreference
	entry->RemoveReference();
}

// MoveEntry
void
NodeManager::MoveEntry(Entry *entry, const entry_ref &newRef)
{
	// get the target directory
	node_ref newDirRef;
	newDirRef.device = newRef.device;
	newDirRef.node = newRef.directory;
	Directory *newDir = dynamic_cast<Directory*>(GetNode(newDirRef));
	if (!newDir) {
		// target directory unknown -- simply remove the entry
		RemoveEntry(entry);
		return;
	}

	// If the directory and/or the name changed, we remove the old entry and
	// create a new one.
	if (newDir != entry->GetParent()
		|| strcmp(newRef.name, entry->GetName()) != 0) {
		// get a temporary reference to the node, so it won't be unnecessarily
		// deleted
		Node *node = entry->GetNode();
		if (node)
			node->AddReference();

		RemoveEntry(entry);
		CreateEntry(newRef, &entry);

		if (node)
			node->RemoveReference();
	}
}

// EntryUnreferenced
void
NodeManager::EntryUnreferenced(Entry *entry)
{
DBG(OUT("NodeManager::EntryUnreferenced(%p): (%p, `%s')\n", entry, entry->GetParent(), entry->GetName()));
	// remove entry from the map and delete it
	if (fEntries.erase(entry->GetEntryRef()) > 0)
		delete entry;
}

// NodeUnreferenced
void
NodeManager::NodeUnreferenced(Node *node)
{
DBG(OUT("NodeManager::NodeUnreferenced(%p): entry: %p\n", node, node->GetEntry()));
	// remove node from the map and delete it
	if (fNodes.erase(node->GetNodeRef()) > 0)
		delete node;
}

// StartWatching
status_t
NodeManager::StartWatching(Node *node)
{
	return fNodeMonitor->StartWatching(node);
}

// StopWatching
status_t
NodeManager::StopWatching(Node *node)
{
	return fNodeMonitor->StopWatching(node);
}

// _NodeMonitoringProcessorEntry
int32
NodeManager::_NodeMonitoringProcessorEntry(void *data)
{
	return ((NodeManager*)data)->_NodeMonitoringProcessor();
}

// _NodeMonitoringProcessor
int32
NodeManager::_NodeMonitoringProcessor()
{
	BMessage *message;
	while (fNodeMonitor->GetNextMonitoringMessage(&message) == B_OK) {
		int32 opcode;
		if (message->FindInt32("opcode", &opcode) == B_OK) {
			BAutolock _(this);
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
				case B_STAT_CHANGED:
					_StatChanged(message);
					break;
			}
		}
		delete message;
	}
}

// _CreateNode
//
// On success the caller gets a reference to the node, they are required to
// surrender, if done with the node.
status_t
NodeManager::_CreateNode(Entry *entry, Node **_node)
{
	// get the path
	string path;
	status_t error = entry->GetPath(path);
	if (error != B_OK)
		return error;

DBG(OUT("disk access: lstat(): %s\n", path.c_str()));

	// read the stat
	struct stat st;
	if (lstat(path.c_str(), &st) < 0)
		return errno;

	// check, if the node does already exist
	node_ref nodeRef;
	nodeRef.device = st.st_dev;
	nodeRef.node = st.st_ino;
	Node *node = GetNode(nodeRef);

	if (node) {
		node->AddReference();
	} else {
		// node does not yet exist -- create it
		if (S_ISLNK(st.st_mode))
			node = new SymLink(st);
		else if (S_ISDIR(st.st_mode))
			node = new Directory(st);
		else
			node = new Node(st);

		error = node->SetTo(entry);
		if (error != B_OK) {
			delete node;
			return error;
		}

		fNodes[nodeRef] = node;
	}

	*_node = node;
	return B_OK;
}

// _EntryCreated
void
NodeManager::_EntryCreated(BMessage *message)
{
	// get the info
	node_ref dirNodeRef;
	const char* name;
	if (message->FindInt32("device", &dirNodeRef.device) != B_OK
		|| message->FindInt64("directory", &dirNodeRef.node) != B_OK
//		|| message->FindInt64("node", &nodeID) != B_OK
		|| message->FindString("name", &name) != B_OK) {
		return;
	}

	// get the directory
	Node *node = NodeManager::GetDefault()->GetNode(dirNodeRef);
	Directory *dir = dynamic_cast<Directory*>(node);
	if (!dir)
		return;

	// add the entry, if the directory is complete
	if (dir->IsComplete()) {
		Entry *entry;
		if (dir->FindEntry(name, &entry) != B_OK) {
			entry_ref ref(dirNodeRef.device, dirNodeRef.node, name);
			NodeManager::GetDefault()->CreateEntry(ref, &entry);
		}
	}
}

// _EntryRemoved
void
NodeManager::_EntryRemoved(BMessage *message)
{
	// get the info
	node_ref nodeRef;
	const char* name;
	if (message->FindInt32("device", &nodeRef.device) != B_OK
//		|| message->FindInt64("directory", &nodeRef.node) != B_OK
		|| message->FindInt64("node", &nodeRef.node) != B_OK) {
		return;
	}

	// get the node
	Node *node = NodeManager::GetDefault()->GetNode(nodeRef);
	if (!node)
		return;

	// remove it
	NodeManager::GetDefault()->RemoveEntry(node->GetEntry());
}

// _EntryMoved
void
NodeManager::_EntryMoved(BMessage *message)
{
	// get the info
	node_ref nodeRef;
	ino_t newDirID;
	const char* name;
	if (message->FindInt32("device", &nodeRef.device) != B_OK
//		|| message->FindInt64("from directory", &fromDirectoryID) != B_OK
		|| message->FindInt64("to directory", &newDirID) != B_OK
		|| message->FindInt64("node", &nodeRef.node) != B_OK
		|| message->FindString("name", &name) != B_OK) {
		return;
	}

	// get the node
	Node *node = NodeManager::GetDefault()->GetNode(nodeRef);
	if (!node)
		return;

	// move it
	entry_ref newRef(nodeRef.device, newDirID, name);
	NodeManager::GetDefault()->MoveEntry(node->GetEntry(), newRef);
}

// _StatChanged
void
NodeManager::_StatChanged(BMessage *message)
{
	// get the node ref
	node_ref nodeRef;
	if (message->FindInt32("device", &nodeRef.device) != B_OK
		|| message->FindInt64("node", &nodeRef.node)) {
		return;
	}

	// get the node
	Node *node = GetNode(nodeRef);
	if (!node)
		return;

	node->MarkStatInvalid();
}

// sManager
NodeManager NodeManager::sManager;


// #pragma mark -

// read_request
static
status_t
read_request(port_id port, stat_cache_request &request)
{
	status_t error = B_OK;
	bool done = false;
	do {
		int32 code;
		ssize_t bytesRead = read_port(port, &code, &request,
			sizeof(stat_cache_request));
		if (bytesRead < 0) {
			error = bytesRead;
			done = (error != B_INTERRUPTED);
		} else if (bytesRead
			< ((stat_cache_request*)NULL)->path + 2 - (char*)NULL) {
DBG(OUT("request too short: %ld\n", bytesRead));
			error = B_ERROR;
		} else {
			done = true;
			error = B_OK;
		}
	} while (!done);
	return error;
}

// handle_stat_request
static
status_t
handle_stat_request(stat_cache_request &request)
{
DBG(OUT("handle_stat_request(): `%s'\n", request.path));
	stat_cache_stat_reply reply;

	// get the node
	PathResolver resolver;
	Node *node;
	reply.error = resolver.FindNode(request.path, true, &node);

	// get the stat
	if (reply.error == B_OK)
		reply.error = node->GetStat(&reply.st);
DBG(OUT("  -> `%s'\n", strerror(reply.error)));

	// send the reply
	return write_port(request.replyPort, 0, &reply, sizeof(reply));
}

// handle_read_dir_request
static
status_t
handle_read_dir_request(stat_cache_request &request)
{
DBG(OUT("handle_read_dir_request(): `%s'\n", request.path));
	// get the directory
	PathResolver resolver;
	Node *node = NULL;
	status_t error = resolver.FindNode(request.path, true, &node);
	Directory *dir = dynamic_cast<Directory*>(node);
	if (error == B_OK && !dir)
		error = B_NOT_A_DIRECTORY;

	// read all entries
	if (error == B_OK)
		error = dir->ReadAllEntries();

	// compute the reply size
	int32 replySize = sizeof(stat_cache_readdir_reply);
	int32 entryCount = 0;
	if (error == B_OK) {
		for (Entry *entry = dir->GetFirstEntry();
			 entry;
			 entry = dir->GetNextEntry(entry)) {
			replySize += get_dirent_size(entry->GetName());
			entryCount++;
		}
	}

	// allocate a reply
	stat_cache_readdir_reply *reply
		= (stat_cache_readdir_reply*)new uint8[replySize];
	reply->error = error;
	reply->entryCount = entryCount;

	// copy the entries into the reply
	if (error == B_OK) {
		uint8 *buffer = reply->buffer;
		for (Entry *entry = dir->GetFirstEntry();
			 entry;
			 entry = dir->GetNextEntry(entry)) {
			// get the required info
			int32 entrySize = get_dirent_size(entry->GetName());
			node_ref parentNodeRef(entry->GetParent()->GetNodeRef());
			node_ref nodeRef(entry->GetNode()->GetNodeRef());

			// fill in the dirent
			dirent *ent = (dirent*)buffer;
			ent->d_pdev = parentNodeRef.device;
			ent->d_pino = parentNodeRef.node;
			ent->d_dev = nodeRef.device;
			ent->d_ino = nodeRef.node;
			ent->d_reclen = entrySize;
			strcpy(ent->d_name, entry->GetName());

			buffer += entrySize;
		}
	}

DBG(OUT("  -> entryCount: %ld, error: `%s'\n", reply->entryCount, strerror(reply->error)));
	// send the reply
	error = write_port(request.replyPort, 0, reply, replySize);
	delete[] (uint8*)reply;
	return error;
}

// request_loop
int32
request_loop(void *data)
{
	port_id requestPort = *(port_id*)data;

	// init node manager
	status_t error = NodeManager::GetDefault()->Init();
	if (error != B_OK) {
		fprintf(stderr, "Failed to init node manager: %s\n", strerror(error));
		return 1;
	}

	// handle requests until our port goes away
	stat_cache_request request;
	while (read_request(requestPort, request) == B_OK) {
		BAutolock _(NodeManager::GetDefault());
		switch (request.command) {
			case STAT_CACHE_COMMAND_STAT:
				handle_stat_request(request);
				break;
			case STAT_CACHE_COMMAND_READDIR:
				handle_read_dir_request(request);
				break;
			default:
				fprintf(stderr, "Unknown command: %ld\n", request.command);
				break;
		}
	}

	// delete the request port
	if (delete_port(requestPort) == B_OK)
		be_app->PostMessage(B_QUIT_REQUESTED);
	return 0;
}

// main
int
main()
{
	// create the request port
	port_id requestPort = create_port(1, STAT_CACHE_SERVER_PORT_NAME);
	if (requestPort < 0) {
		fprintf(stderr, "Failed to create request port: %s\n",
			strerror(requestPort));
		return 1;
	}

	// start the request handling loop
	thread_id requestLoop = spawn_thread(request_loop, "request loop",
		B_NORMAL_PRIORITY, &requestPort);
	if (resume_thread(requestLoop) < B_OK)
		return -1;

	// the BApplication is only needed for a smooth server shutdown
	BApplication app("application/x-vnd.haiku.jam-cacheserver");
	app.Run();

	// delete the request port
	delete_port(requestPort);

	// wait for the request handling loop to quit
	status_t result;
	wait_for_thread(requestLoop, &result);

	return 0;
}
