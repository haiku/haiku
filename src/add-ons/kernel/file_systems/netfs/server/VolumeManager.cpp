// VolumeManager.cpp

#include "VolumeManager.h"

#include <new>

#include <sys/stat.h>

#include <AutoDeleter.h>
#include <Entry.h>
#include <fs_info.h>
#include <fs_query.h>
#include <HashMap.h>
#include <NodeMonitor.h>
#include <Volume.h>

#include "ClientVolume.h"
#include "DebugSupport.h"
#include "Directory.h"
#include "Entry.h"
#include "FDManager.h"
#include "NodeHandle.h"
#include "Path.h"
#include "QueryDomain.h"
#include "Volume.h"

// TODO: We should filter recent events at some point. Otherwise we'll end up
// with one event of each kind for each entry/node.

const bigtime_t kRecentEventLifeTime = 100000;	// 0.1 s

// QueryHandler
class VolumeManager::QueryHandler : public BHandler, public QueryListener,
	public BReferenceable {
public:
	QueryHandler(NodeMonitorListener* listener, QueryDomain* queryDomain,
		QueryHandle* handle)
		:
		BHandler(),
		QueryListener(),
		BReferenceable(),
		fListener(listener),
		fQueryDomain(queryDomain),
		fHandle(handle)
	{
	}

	QueryDomain* GetQueryDomain() const
	{
		return fQueryDomain;
	}

	QueryHandle* GetQueryHandle() const
	{
		return fHandle;
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case B_QUERY_UPDATE:
			{
				NodeMonitoringEvent* event = NULL;
				int32 opcode;
				if (message->FindInt32("opcode", &opcode) == B_OK) {
					switch (opcode) {
						case B_ENTRY_CREATED:
							event = new(std::nothrow) EntryCreatedEvent;
							break;
						case B_ENTRY_REMOVED:
							event = new(std::nothrow) EntryRemovedEvent;
							break;
						case B_ENTRY_MOVED:
							event = new(std::nothrow) EntryMovedEvent;
							break;
					}
				}
				if (event) {
					event->queryHandler = this;
					AcquireReference();
					if (event->Init(message) == B_OK)
						fListener->ProcessNodeMonitoringEvent(event);
					else
						delete event;
				}
				break;
			}
			default:
				BHandler::MessageReceived(message);
		}
	}

	virtual void QueryHandleClosed(QueryHandle* handle)
	{
		BLooper* looper = Looper();
		if (looper && looper->Lock()) {
			looper->RemoveHandler(this);
			looper->Unlock();
		}
		handle->SetQueryListener(NULL);
		ReleaseReference();
	}

private:
	NodeMonitorListener*	fListener;
	QueryDomain*			fQueryDomain;
	QueryHandle*			fHandle;
};

// VolumeMap
struct VolumeManager::VolumeMap : HashMap<HashKey32<dev_t>, Volume*> {
};

// ClientVolumeMap
struct VolumeManager::ClientVolumeMap
	: HashMap<HashKey32<int32>, ClientVolume*> {
};

// private BeOS syscalls to set the FD and node monitor slot limits
extern "C" int _kset_fd_limit_(int num);
extern "C" int _kset_mon_limit_(int num);


// EntryCreatedEventMap
struct VolumeManager::EntryCreatedEventMap
	: HashMap<EntryRef, EntryCreatedEvent*> {
};

// EntryRemovedEventMap
struct VolumeManager::EntryRemovedEventMap
	: HashMap<EntryRef, EntryRemovedEvent*> {
};

// EntryMovedEventKey
struct EntryMovedEventKey : public EntryRef {
	EntryMovedEventKey()
	{
	}

	EntryMovedEventKey(dev_t volumeID, ino_t fromDirectory,
		const char* fromName, ino_t toDirectory, const char* toName)
		: EntryRef(volumeID, fromDirectory, fromName),
		  toDirectory(toDirectory),
		  toName(toName)
	{

	}

	uint32 GetHashCode() const
	{
		uint32 hash = EntryRef::GetHashCode();
		hash = 17 * hash + (uint32)(toDirectory >> 32);
		hash = 17 * hash + (uint32)toDirectory;
		hash = 17 * hash + string_hash(toName.GetString());
		return hash;
	}

	bool operator==(const EntryMovedEventKey& other) const
	{
		return (*(const EntryRef*)this) == other
			&& toDirectory == other.toDirectory
			&& toName == other.toName;
	}

	bool operator!=(const EntryMovedEventKey& other) const
	{
		return !(*this == other);
	}

	ino_t		toDirectory;
	HashString	toName;
};

// EntryMovedEventMap
struct VolumeManager::EntryMovedEventMap : HashMap<EntryRef, EntryMovedEvent*> {
};

// NodeStatChangedEventMap
struct VolumeManager::NodeStatChangedEventMap
	: HashMap<NodeRef, StatChangedEvent*> {
};

typedef EntryRef AttributeRef;

// NodeAttributeChangedEventMap
struct VolumeManager::NodeAttributeChangedEventMap
	: HashMap<AttributeRef, AttributeChangedEvent*> {
};


// #pragma mark -

// constructor
VolumeManager::VolumeManager()
	: fLock("volume manager"),
	  fVolumes(NULL),
	  fRootVolume(NULL),
	  fClientVolumes(NULL),
	  fNodeMonitor(NULL),
	  fNodeMonitoringProcessor(-1),
	  fNodeMonitoringEvents(),
	  fRecentNodeMonitoringEvents(),
	  fEntryCreatedEvents(NULL),
	  fEntryRemovedEvents(NULL),
	  fEntryMovedEvents(NULL),
	  fNodeStatChangedEvents(NULL),
	  fNodeAttributeChangedEvents(NULL),
	  fRevision(0),
	  fTerminating(false)
{
}

// destructor
VolumeManager::~VolumeManager()
{
	// terminate the node monitor and the node monitoring processor
	fTerminating = true;
	if (fNodeMonitor && fNodeMonitor->Lock())
		fNodeMonitor->Quit();
	fNodeMonitoringEvents.Close(true);
	if (fNodeMonitoringProcessor >= 0) {
		int32 result;
		wait_for_thread(fNodeMonitoringProcessor, &result);
	}

	// delete all events
	// entry created events
	for (EntryCreatedEventMap::Iterator it = fEntryCreatedEvents->GetIterator();
		 it.HasNext();) {
		it.Next().value->ReleaseReference();
	}
	delete fEntryCreatedEvents;

	// entry removed events
	for (EntryRemovedEventMap::Iterator it = fEntryRemovedEvents->GetIterator();
		 it.HasNext();) {
		it.Next().value->ReleaseReference();
	}
	delete fEntryRemovedEvents;

	// entry moved events
	for (EntryMovedEventMap::Iterator it = fEntryMovedEvents->GetIterator();
		 it.HasNext();) {
		it.Next().value->ReleaseReference();
	}
	delete fEntryMovedEvents;

	// stat changed events
	for (NodeStatChangedEventMap::Iterator it
			= fNodeStatChangedEvents->GetIterator();
		 it.HasNext();) {
		it.Next().value->ReleaseReference();
	}
	delete fNodeStatChangedEvents;

	// attribute changed events
	for (NodeAttributeChangedEventMap::Iterator it
			= fNodeAttributeChangedEvents->GetIterator();
		 it.HasNext();) {
		it.Next().value->ReleaseReference();
	}
	delete fNodeAttributeChangedEvents;

	delete fClientVolumes;

	// delete the volumes
	for (VolumeMap::Iterator it = fVolumes->GetIterator(); it.HasNext();) {
		Volume* volume = it.Next().value;
		delete volume;
	}
	delete fVolumes;
}

// Init
status_t
VolumeManager::Init()
{
	// check node monitoring message queue
	status_t error = fNodeMonitoringEvents.InitCheck();
	if (error != B_OK)
		return error;

	// entry created event map
	fEntryCreatedEvents = new(std::nothrow) EntryCreatedEventMap;
	if (!fEntryCreatedEvents)
		return B_NO_MEMORY;

	// entry removed event map
	fEntryRemovedEvents = new(std::nothrow) EntryRemovedEventMap;
	if (!fEntryRemovedEvents)
		return B_NO_MEMORY;

	// entry moved event map
	fEntryMovedEvents = new(std::nothrow) EntryMovedEventMap;
	if (!fEntryMovedEvents)
		return B_NO_MEMORY;

	// node stat changed event map
	fNodeStatChangedEvents = new(std::nothrow) NodeStatChangedEventMap;
	if (!fNodeStatChangedEvents)
		return B_NO_MEMORY;

	// node attribute changed event map
	fNodeAttributeChangedEvents = new(std::nothrow) NodeAttributeChangedEventMap;
	if (!fNodeAttributeChangedEvents)
		return B_NO_MEMORY;

	// create the node monitor
	fNodeMonitor = new(std::nothrow) NodeMonitor(this);
	if (!fNodeMonitor)
		return B_NO_MEMORY;

	// create the volume map
	fVolumes = new(std::nothrow) VolumeMap;
	if (!fVolumes)
		return B_NO_MEMORY;
	if (fVolumes->InitCheck() != B_OK)
		return fVolumes->InitCheck();

	// create the client volume map
	fClientVolumes = new(std::nothrow) ClientVolumeMap;
	if (!fClientVolumes)
		return B_NO_MEMORY;
	if (fClientVolumes->InitCheck() != B_OK)
		return fClientVolumes->InitCheck();

	// start the node monitor
	thread_id monitorThread = fNodeMonitor->Run();
	if (monitorThread < 0) {
		delete fNodeMonitor;
		fNodeMonitor = NULL;
		return monitorThread;
	}

	// create all volumes
	int32 cookie = 0;
	dev_t volumeID;
	while ((volumeID = next_dev(&cookie)) >= 0)
		_AddVolume(volumeID);

	// get the root volume
	volumeID = dev_for_path("/");
	if (volumeID < 0)
		return volumeID;
	fRootVolume = GetVolume(volumeID, true);
	if (!fRootVolume)
		return B_ERROR;

	// spawn the node monitoring message processor
	fNodeMonitoringProcessor = spawn_thread(&_NodeMonitoringProcessorEntry,
		"node monitoring processor", B_NORMAL_PRIORITY, this);
	if (fNodeMonitoringProcessor < 0)
		return fNodeMonitoringProcessor;
	resume_thread(fNodeMonitoringProcessor);

	return B_OK;
}

// GetRootVolume
Volume*
VolumeManager::GetRootVolume() const
{
	return fRootVolume;
}

// AddClientVolume
status_t
VolumeManager::AddClientVolume(ClientVolume* clientVolume)
{
	if (!clientVolume)
		return B_BAD_VALUE;

	return fClientVolumes->Put(clientVolume->GetID(), clientVolume);
}

// RemoveClientVolume
void
VolumeManager::RemoveClientVolume(ClientVolume* clientVolume)
{
	if (!clientVolume)
		return;

	fClientVolumes->Remove(clientVolume->GetID());
}

// CreateDefault
status_t
VolumeManager::CreateDefault()
{
	if (sManager)
		return B_OK;

	VolumeManager* manager = new(std::nothrow) VolumeManager;
	if (!manager)
		return B_NO_MEMORY;

	status_t error = manager->Init();
	if (error != B_OK) {
		delete manager;
		return error;
	}

	sManager = manager;
	return B_OK;
}

// DeleteDefault
void
VolumeManager::DeleteDefault()
{
	if (sManager) {
		delete sManager;
		sManager = NULL;
	}
}

// GetDefault
VolumeManager*
VolumeManager::GetDefault()
{
	return sManager;
}

// Lock
bool
VolumeManager::Lock()
{
	bool alreadyLocked = fLock.IsLocked();

	bool success = fLock.Lock();

	// If locking was successful and we didn't have a lock before, we increment
	// the revision.
	if (success && !alreadyLocked)
		fRevision++;

	return success;
}

// Unlock
void
VolumeManager::Unlock()
{
	return fLock.Unlock();
}

// GetRevision
int64
VolumeManager::GetRevision() const
{
	return fRevision;
}

// GetVolume
Volume*
VolumeManager::GetVolume(dev_t volumeID, bool add)
{
	Volume* volume = fVolumes->Get(volumeID);
	if (!volume && add)
		_AddVolume(volumeID, &volume);

	return volume;
}


// #pragma mark -

// AddNode
status_t
VolumeManager::AddNode(Node* node)
{
	if (!node || !node->GetVolume())
		return B_BAD_VALUE;

	status_t error = node->GetVolume()->AddNode(node);

	// start watching the node
	if (error == B_OK)
		fNodeMonitor->StartWatching(node->GetNodeRef());

	return error;
}

// RemoveNode
void
VolumeManager::RemoveNode(Node* node)
{
	if (!node)
		return;

	// if the node is a directory, we remove all its entries first
	if (Directory* directory = dynamic_cast<Directory*>(node)) {
		while (Entry* entry = directory->GetFirstEntry()) {
			RemoveEntry(entry);
			delete entry;
		}
	}

	// remove all referring entries
	while (Entry* entry = node->GetFirstReferringEntry())
		RemoveEntry(entry);

	// remove the node from the volume
	if (node->GetVolume())
		node->GetVolume()->RemoveNode(node);

	// stop watching the node
	fNodeMonitor->StopWatching(node->GetNodeRef());
}

// GetNode
Node*
VolumeManager::GetNode(dev_t volumeID, ino_t nodeID)
{
	if (Volume* volume = GetVolume(volumeID))
		return volume->GetNode(nodeID);
	return NULL;
}

// LoadNode
status_t
VolumeManager::LoadNode(const struct stat& st, Node** _node)
{
	Node* node = GetNode(st.st_dev, st.st_ino);
	if (!node) {
		// node not known yet: create it

		// get the volume
		Volume* volume = GetVolume(st.st_dev, true);
		if (!volume)
			return B_BAD_VALUE;

		// create the node
		if (S_ISDIR(st.st_mode))
			node = new(std::nothrow) Directory(volume, st);
		else
			node = new(std::nothrow) Node(volume, st);
		if (!node)
			return B_NO_MEMORY;

		// add it
		status_t error = AddNode(node);
		if (error != B_OK) {
			delete node;
			return error;
		}
	}

	if (_node)
		*_node = node;
	return B_OK;
}


// #pragma mark -

// GetDirectory
Directory*
VolumeManager::GetDirectory(dev_t volumeID, ino_t nodeID)
{
	return dynamic_cast<Directory*>(GetNode(volumeID, nodeID));
}

// GetRootDirectory
Directory*
VolumeManager::GetRootDirectory() const
{
	return (fRootVolume ? fRootVolume->GetRootDirectory() : NULL);
}

// GetParentDirectory
Directory*
VolumeManager::GetParentDirectory(Directory* directory)
{
	if (!directory)
		return NULL;

	// get ".." entry
	Entry* parentEntry;
	if (LoadEntry(directory->GetVolumeID(), directory->GetID(), "..", true,
			&parentEntry) != B_OK) {
		return NULL;
	}

	return dynamic_cast<Directory*>(parentEntry->GetNode());
}

// LoadDirectory
status_t
VolumeManager::LoadDirectory(dev_t volumeID, ino_t directoryID,
	Directory** _directory)
{
	// try to get the node
	Node* node = GetNode(volumeID, directoryID);
	bool newNode = false;
	if (!node) {
		// directory not yet loaded: stat it
		NoAllocEntryRef entryRef(volumeID, directoryID, ".");
		struct stat st;
		BEntry bEntry;
		status_t error = FDManager::SetEntry(&bEntry, &entryRef);
		if (error == B_OK)
			error = bEntry.GetStat(&st);
		if (error != B_OK)
			return error;

		// load the node
		error = LoadNode(st, &node);
		if (error != B_OK)
			return error;

		newNode = true;
	}

	// check, if the node is a directory
	Directory* directory = dynamic_cast<Directory*>(node);
	if (!directory)
		return B_NOT_A_DIRECTORY;

	if (newNode)
		CompletePathToRoot(directory);

	if (_directory)
		*_directory = directory;
	return B_OK;
}


// #pragma mark -

// AddEntry
status_t
VolumeManager::AddEntry(Entry* entry)
{
	if (!entry || !entry->GetVolume() || !entry->GetDirectory()
		|| ! entry->GetNode()) {
		return B_BAD_VALUE;
	}

	// add the entry to the volume
	status_t error = entry->GetVolume()->AddEntry(entry);
	if (error != B_OK)
		return error;

	// add the entry to its directory and node
	entry->GetDirectory()->AddEntry(entry);
	entry->GetNode()->AddReferringEntry(entry);

//PRINT(("VolumeManager::AddEntry(): %ld, %lld, `%s', dir: %p, "
//"entry count: %ld\n", entry->GetVolumeID(), entry->GetDirectoryID(),
//entry->GetName(), entry->GetDirectory(),
//entry->GetDirectory()->CountEntries()));

	return B_OK;
}

// RemoveEntry
void
VolumeManager::RemoveEntry(Entry* entry)
{
	if (entry) {
		// remove the entry from the volume
		if (entry->GetVolume())
			entry->GetVolume()->RemoveEntry(entry);

		// remove the entry from the directory and its node
		entry->GetDirectory()->RemoveEntry(entry);
		entry->GetNode()->RemoveReferringEntry(entry);

//PRINT(("VolumeManager::RemoveEntry(): %ld, %lld, `%s', dir: %p, "
//"entry count: %ld\n", entry->GetVolumeID(), entry->GetDirectoryID(),
//entry->GetName(), entry->GetDirectory(),
//entry->GetDirectory()->CountEntries()));
	}
}

// DeleteEntry
void
VolumeManager::DeleteEntry(Entry* entry, bool keepNode)
{
	if (!entry)
		return;

	Node* node = entry->GetNode();

	// remove the entry
	RemoveEntry(entry);
	delete entry;

	// remove the node, if it doesn't have any more actual referring entries
	if (!keepNode && !node->GetActualReferringEntry()) {
		RemoveNode(node);
		if (node != node->GetVolume()->GetRootDirectory())
			delete node;
	}
}

// GetEntry
Entry*
VolumeManager::GetEntry(dev_t volumeID, ino_t directoryID, const char* name)
{
	if (Volume* volume = GetVolume(volumeID))
		return volume->GetEntry(directoryID, name);
	return NULL;
}

// GetEntry
Entry*
VolumeManager::GetEntry(const entry_ref& ref)
{
	return GetEntry(ref.device, ref.directory, ref.name);
}

// LoadEntry
status_t
VolumeManager::LoadEntry(dev_t volumeID, ino_t directoryID, const char* name,
	bool loadDir, Entry** _entry)
{
	Entry* entry = GetEntry(volumeID, directoryID, name);
	if (!entry) {
		// entry not known yet: create it
		PRINT("VolumeManager::LoadEntry(%ld, %lld, `%s')\n", volumeID,
			directoryID, name);

		// get the volume
		Volume* volume = GetVolume(volumeID, true);
		if (!volume)
			return B_BAD_VALUE;

		// get the directory
		status_t error = B_OK;
		Directory* directory = GetDirectory(volumeID, directoryID);
		if (!directory) {
			if (!loadDir)
				return B_ENTRY_NOT_FOUND;

//PRINT(("  loading directory...\n"));
			// load the directory
			error = LoadDirectory(volumeID, directoryID, &directory);
			if (error != B_OK)
				return error;
		}

//PRINT(("  opening BNode...\n"));
		// stat the entry
		NoAllocEntryRef entryRef(volumeID, directoryID, name);
		struct stat st;
		BNode bNode;
		error = bNode.SetTo(&entryRef);
//PRINT(("  stat()ing BNode...\n"));
		if (error == B_OK)
			error = bNode.GetStat(&st);
		if (error != B_OK)
			return error;

//PRINT(("  loading node...\n"));
		// load the node
		Node* node;
		error = LoadNode(st, &node);
		if (error != B_OK)
			return error;

//PRINT(("  creating and adding entry...\n"));
		// create the entry
		entry = new(std::nothrow) Entry(volume, directory, name, node);
		if (!entry)
			return B_NO_MEMORY;

		// add it
		error = AddEntry(entry);
		if (error != B_OK) {
			delete entry;
			return error;
		}
//PRINT(("  adding entry done\n"));
	}

	if (_entry)
		*_entry = entry;
	return B_OK;
}


// #pragma mark -

// OpenQuery
status_t
VolumeManager::OpenQuery(QueryDomain* queryDomain, const char* queryString,
	uint32 flags, port_id remotePort, int32 remoteToken, QueryHandle** handle)
{
	if (!queryDomain || !queryString || !handle)
		return B_BAD_VALUE;
	bool liveQuery = (flags & B_LIVE_QUERY);
	PRINT("VolumeManager::OpenQuery(%p, \"%s\", 0x%lx, %ld, %ld)\n",
		queryDomain, queryString, flags, remotePort, remoteToken);

	// allocate the handle
	QueryHandle* queryHandle = new(std::nothrow) QueryHandle(remotePort,
		remoteToken);
	if (!queryHandle)
		return B_NO_MEMORY;
	ObjectDeleter<QueryHandle> handleDeleter(queryHandle);

	// allocate a query handler, if this is a live query
	QueryHandler* queryHandler = NULL;
	if (liveQuery) {
		queryHandler = new(std::nothrow) QueryHandler(this, queryDomain,
			queryHandle);
		if (!queryHandler)
			return B_NO_MEMORY;

		fNodeMonitor->Lock();
		fNodeMonitor->AddHandler(queryHandler);
		fNodeMonitor->Unlock();
		queryHandle->SetQueryListener(queryHandler);
	}

	// iterate through the volumes and create a query for each one
	// supporting queries
	for (VolumeMap::Iterator it = fVolumes->GetIterator(); it.HasNext();) {
		Volume* volume = it.Next().value;
		if (!volume->KnowsQuery())
			continue;

		// The volume should either be contained by the client volume or
		// the other way around. Otherwise they are located in different
		// branches of the FS tree and don't have common nodes.
		if (!queryDomain->QueryDomainIntersectsWith(volume))
			continue;
		PRINT("VolumeManager::OpenQuery(): adding Query for volume %ld"
			"\n", volume->GetID());

		// create the query for this volume
		BVolume bVolume(volume->GetID());
		Query* query = new(std::nothrow) Query;
		if (!query)
			return B_NO_MEMORY;

		// init the query
		ObjectDeleter<Query> queryDeleter(query);
		status_t error = query->SetVolume(&bVolume);
		if (error != B_OK)
			return error;
		error = query->SetPredicate(queryString);
		if (error != B_OK)
			return error;
		if (liveQuery) {
			error = query->SetTarget(queryHandler);
			if (error != B_OK)
				return error;
		}

		// fetch
		error = query->Fetch();
		if (error != B_OK)
			return error;

		queryHandle->AddQuery(query);
		queryDeleter.Detach();
	}

	*handle = queryHandle;
	handleDeleter.Detach();
	return B_OK;
}

// CompletePathToRoot
status_t
VolumeManager::CompletePathToRoot(Directory* directory)
{
	if (!directory)
		return B_BAD_VALUE;

	while (directory != GetRootDirectory()) {
		// if the dir has a valid entry referring to it, we've nothing to do
		if (directory->GetActualReferringEntry())
			return B_OK;

		// get a proper entry_ref
		BEntry bEntry;
		entry_ref entryRef(directory->GetVolumeID(), directory->GetID(), ".");
		status_t error = FDManager::SetEntry(&bEntry, &entryRef);
		if (error == B_OK)
			error = bEntry.GetRef(&entryRef);
		if (error != B_OK)
			return error;

		// if the entry is already loaded, we're done
		if (GetEntry(entryRef))
			return B_OK;

		// the entry is not yet known -- load it
		Entry* entry;
		error = LoadEntry(entryRef.device, entryRef.directory, entryRef.name,
			true, &entry);
		if (error != B_OK)
			return error;

		// get the entry's parent dir and enter the next round
		directory = entry->GetDirectory();
	}

	return B_OK;
}

// GetPath
status_t
VolumeManager::GetPath(Entry* entry, Path* path)
{
	// get directory path
	status_t error = GetPath(entry->GetDirectory(), path);
	if (error != B_OK)
		return error;

	// append the entry name
	return path->Append(entry->GetName());
}

// GetPath
status_t
VolumeManager::GetPath(Node* node, Path* path)
{
	if (node == GetRootDirectory())
		return path->SetTo("/");

	// get an entry referring to the node
	Entry* entry = node->GetActualReferringEntry();
	if (!entry) {
		// if the node is a directory, we complete the path to the root and
		// try again
		if (Directory* directory = dynamic_cast<Directory*>(node)) {
			CompletePathToRoot(directory);
			entry = node->GetActualReferringEntry();
		}

		if (!entry)
			return B_ERROR;
	}

	return GetPath(entry, path);
}

// DirectoryContains
bool
VolumeManager::DirectoryContains(Directory* directory, Entry* entry)
{
	if (!directory || !entry)
		return false;

	return DirectoryContains(directory, entry->GetDirectory(), true);
}

// DirectoryContains
bool
VolumeManager::DirectoryContains(Directory* directory, Directory* descendant,
	bool reflexive)
{
	if (!directory || !descendant)
		return false;

	// a directory contains itself, just as defined by the caller
	if (directory == descendant)
		return reflexive;

	// if the directory is the root directory, it contains everything
	Directory* rootDir = GetRootDirectory();
	if (directory == rootDir)
		return true;

	// recursively get the descendant's parent dir until reaching the root dir
	// or the given dir
	while (descendant != rootDir) {
		descendant = GetParentDirectory(descendant);
		if (!descendant)
			return false;

		if (descendant == directory)
			return true;
	}

	return false;
}

// DirectoryContains
bool
VolumeManager::DirectoryContains(Directory* directory, Node* descendant,
	bool reflexive)
{
	if (!directory || !descendant)
		return false;

	// if the node is a directory, let the other version do the job
	if (Directory* dir = dynamic_cast<Directory*>(descendant))
		return DirectoryContains(directory, dir, reflexive);

	// iterate through the referring entries and check, if the directory
	// contains any of them
	for (Entry* entry = descendant->GetFirstReferringEntry();
		 entry;
		 entry = descendant->GetNextReferringEntry(entry)) {
		if (DirectoryContains(directory, entry))
			return true;
	}

	return false;
}


// #pragma mark -

// ProcessNodeMonitoringEvent
void
VolumeManager::ProcessNodeMonitoringEvent(NodeMonitoringEvent* event)
{
	if (fNodeMonitoringEvents.Push(event) != B_OK)
		delete event;
}

// _AddVolume
status_t
VolumeManager::_AddVolume(dev_t volumeID, Volume** _volume)
{
	if (GetVolume(volumeID))
		return B_OK;

	// create the volume
	Volume* volume = new(std::nothrow) Volume(volumeID);
	if (!volume)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<Volume> volumeDeleter(volume);
	status_t error = volume->Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	// add it
	error = fVolumes->Put(volumeID, volume);
	if (error != B_OK)
		RETURN_ERROR(error);

	// add the root node
	error = AddNode(volume->GetRootDirectory());
	if (error != B_OK) {
		fVolumes->Remove(volumeID);
		RETURN_ERROR(error);
	}

	// complete the root dir path
	CompletePathToRoot(volume->GetRootDirectory());

	volumeDeleter.Detach();
	if (_volume)
		*_volume = volume;
	return B_OK;
}

// _EntryCreated
void
VolumeManager::_EntryCreated(EntryCreatedEvent* event)
{
	// get the directory
	Directory* directory = GetDirectory(event->volumeID, event->directoryID);
	if (!directory)
		return;

	// check, if there is an earlier similar event
	bool notify = true;
	NoAllocEntryRef ref(event->volumeID, event->directoryID,
		event->name.GetString());
	EntryCreatedEvent* oldEvent = fEntryCreatedEvents->Get(ref);

	// remove the old event
	if (oldEvent) {
		fEntryCreatedEvents->Remove(ref);
		fRecentNodeMonitoringEvents.Remove(oldEvent);
		notify = !_IsRecentEvent(oldEvent);
		oldEvent->ReleaseReference();
	}

	// add the new event
	if (fEntryCreatedEvents->Put(ref, event) == B_OK) {
		fRecentNodeMonitoringEvents.Insert(event);
		event->AcquireReference();
	}

	// if the directory is complete or at least has iterators attached to it,
	// we load the entry
	if (directory->IsComplete() || directory->HasDirIterators()) {
		Entry* entry;
		LoadEntry(ref.device, ref.directory, ref.name, false, &entry);
	}

	// send notifications
	if (notify) {
		for (ClientVolumeMap::Iterator it = fClientVolumes->GetIterator();
			 it.HasNext();) {
			ClientVolume* clientVolume = it.Next().value;
			if (DirectoryContains(clientVolume->GetRootDirectory(), directory,
				true)) {
				clientVolume->ProcessNodeMonitoringEvent(event);
			}
		}
	}
}

// _EntryRemoved
void
VolumeManager::_EntryRemoved(EntryRemovedEvent* event, bool keepNode)
{
	// get node and directory
	Node* node = GetNode(event->nodeVolumeID, event->nodeID);
	Directory* directory = GetDirectory(event->volumeID, event->directoryID);
	if (!directory)
		return;

	// find the entry
	Entry* entry = NULL;
	if (node) {
		if (event->name.GetLength() == 0) {
			for (entry = node->GetFirstReferringEntry();
				 entry;
				 entry = node->GetNextReferringEntry(entry)) {
				if (!entry->Exists()) {
					event->name.SetTo(entry->GetName());
					break;
				}
			}
		} else {
			entry = GetEntry(directory->GetVolumeID(), directory->GetID(),
				event->name.GetString());
		}
	}

	// check, if there is an earlier similar event
	bool notify = true;
	NoAllocEntryRef ref(event->volumeID, event->directoryID,
		event->name.GetString());
	EntryRemovedEvent* oldEvent = fEntryRemovedEvents->Get(ref);
		// TODO: Under BeOS R5 the entry name is not encoded in the
		// "entry removed" node monitoring message. If we have seen the entry
		// before, we can get the entry nevertheless (see above). Usually we
		// get 2 "entry removed" events: One for watching the directory and one
		// for watching the node. After the first one has been processed, we've
		// forgotten everything about the entry and we won't be able to find out
		// the entry's name for the second one. Hence we will never find the
		// previous event in the fEntryRemovedEvents map. We should probably
		// fall back to using a NodeRef as key under BeOS R5.

	// remove the old event
	if (oldEvent) {
		fEntryRemovedEvents->Remove(ref);
		fRecentNodeMonitoringEvents.Remove(oldEvent);
		notify = !_IsRecentEvent(oldEvent);
		oldEvent->ReleaseReference();
	}

	// add the new event
	if (fEntryRemovedEvents->Put(ref, event) == B_OK) {
		fRecentNodeMonitoringEvents.Insert(event);
		event->AcquireReference();
	}

	// remove the entry
	if (entry) {
		RemoveEntry(entry);
		delete entry;
	}

	// remove the node, if it doesn't have any more actual referring entries
	if (node && !keepNode && !node->GetActualReferringEntry()) {
		RemoveNode(node);
		if (node != node->GetVolume()->GetRootDirectory())
			delete node;
	}

	// send notifications
	if (notify) {
		NodeRef nodeRef(event->nodeVolumeID, event->nodeID);
		for (ClientVolumeMap::Iterator it = fClientVolumes->GetIterator();
			 it.HasNext();) {
			// We send a notification, if the client volume contains the entry,
			// but also, if the removed entry refers to the client volume's
			// root. The client connection has a special handling for this
			// case.
			ClientVolume* clientVolume = it.Next().value;
			Directory* rootDir = clientVolume->GetRootDirectory();
			if (DirectoryContains(rootDir, directory, true)
				|| clientVolume->GetRootNodeRef() == nodeRef) {
				clientVolume->ProcessNodeMonitoringEvent(event);
			}
		}
	}
}

// _EntryMoved
void
VolumeManager::_EntryMoved(EntryMovedEvent* event)
{
	_CheckVolumeRootMoved(event);

	Directory* fromDirectory
		= GetDirectory(event->volumeID, event->fromDirectoryID);
	Directory* toDirectory
		= GetDirectory(event->volumeID, event->toDirectoryID);
	Node* node = GetNode(event->nodeVolumeID, event->nodeID);

	// we should at least have one of the directories
	if (!fromDirectory && !toDirectory)
		return;

	// find the old entry
	Entry* oldEntry = NULL;
	if (node) {
		if (event->fromName.GetLength() == 0) {
			for (oldEntry = node->GetFirstReferringEntry();
				 oldEntry;
				 oldEntry = node->GetNextReferringEntry(oldEntry)) {
				if (!oldEntry->Exists()) {
					event->fromName.SetTo(oldEntry->GetName());
					break;
				}
			}
		} else {
			oldEntry = GetEntry(event->volumeID, event->fromDirectoryID,
				event->fromName.GetString());
		}
	}

	// check, if there is an earlier similar event
	bool notify = true;
	if (event->fromName.GetLength() > 0) {
		EntryMovedEventKey key(event->volumeID, event->fromDirectoryID,
			event->fromName.GetString(), event->toDirectoryID,
			event->toName.GetString());
		EntryMovedEvent* oldEvent = fEntryMovedEvents->Get(key);

		// remove the old event
		if (oldEvent) {
			fEntryMovedEvents->Remove(key);
			fRecentNodeMonitoringEvents.Remove(oldEvent);
			notify = !_IsRecentEvent(oldEvent);
			oldEvent->ReleaseReference();
		}

		// add the new event
		if (fEntryMovedEvents->Put(key, event) == B_OK) {
			fRecentNodeMonitoringEvents.Insert(event);
			event->AcquireReference();
		}
	}

	// remove the old entry
	if (oldEntry) {
		RemoveEntry(oldEntry);
		delete oldEntry;
	}

	// If the to directory is complete or at least has iterators attached to it,
	// we load the new entry. We also load it, if the node is the root of a
	// volume.
	if (toDirectory
		&& (toDirectory->IsComplete() || toDirectory->HasDirIterators()
			|| (node && node == node->GetVolume()->GetRootDirectory()))) {
		Entry* newEntry;
		LoadEntry(event->volumeID, event->toDirectoryID,
			event->toName.GetString(), false, &newEntry);
	}

	// remove the node, if it doesn't have any more actual referring entries
	if (node && !node->GetActualReferringEntry()) {
		RemoveNode(node);
		if (node != node->GetVolume()->GetRootDirectory())
			delete node;
	}

	// send notifications
	if (notify) {
		for (ClientVolumeMap::Iterator it = fClientVolumes->GetIterator();
			 it.HasNext();) {
			ClientVolume* clientVolume = it.Next().value;

			// check, if it contains the from/to directories
			Directory* rootDir = clientVolume->GetRootDirectory();
			bool containsFrom = DirectoryContains(rootDir, fromDirectory, true);
			bool containsTo = DirectoryContains(rootDir, toDirectory, true);

			if (containsFrom) {
				if (containsTo) {
					// contains source and target dir
					clientVolume->ProcessNodeMonitoringEvent(event);
				} else {
					// contains only the source dir: generate an "entry removed"
					// event
					EntryRemovedEvent *removedEvent
						= new(std::nothrow) EntryRemovedEvent;
					if (!removedEvent)
						continue;
					removedEvent->opcode = B_ENTRY_REMOVED;
					removedEvent->time = event->time;
					removedEvent->volumeID = event->volumeID;
					removedEvent->directoryID = event->fromDirectoryID;
					removedEvent->nodeVolumeID = event->nodeVolumeID;
					removedEvent->nodeID = event->nodeID;
					if (event->fromName.GetLength() > 0)
						removedEvent->name = event->fromName;
					clientVolume->ProcessNodeMonitoringEvent(removedEvent);
					removedEvent->ReleaseReference();
				}
			} else if (containsTo) {
				// contains only the target directory: generate an
				// "entry created" event
				EntryCreatedEvent *createdEvent
					= new(std::nothrow) EntryCreatedEvent;
				if (!createdEvent)
					continue;
				createdEvent->opcode = B_ENTRY_CREATED;
				createdEvent->time = event->time;
				createdEvent->volumeID = event->volumeID;
				createdEvent->directoryID = event->toDirectoryID;
				createdEvent->name = event->toName;
				clientVolume->ProcessNodeMonitoringEvent(createdEvent);
				createdEvent->ReleaseReference();
			}
		}
	}
}

// _NodeStatChanged
void
VolumeManager::_NodeStatChanged(StatChangedEvent* event)
{
	// get the node
	Node* node = GetNode(event->volumeID, event->nodeID);
	if (!node)
		return;

	// check, if there is an earlier similar event
	bool notify = true;
	NodeRef ref(event->volumeID, event->nodeID);
	StatChangedEvent* oldEvent = fNodeStatChangedEvents->Get(ref);

	// remove the old event
	if (oldEvent) {
		fNodeStatChangedEvents->Remove(ref);
		fRecentNodeMonitoringEvents.Remove(oldEvent);
		notify = !_IsRecentEvent(oldEvent);
		oldEvent->ReleaseReference();
	}

	// add the new event
	if (fNodeStatChangedEvents->Put(ref, event) == B_OK) {
		fRecentNodeMonitoringEvents.Insert(event);
		event->AcquireReference();
	}

	if (notify) {
		// update the cached node stat
		node->UpdateStat();

		// send notifications
		for (ClientVolumeMap::Iterator it = fClientVolumes->GetIterator();
			 it.HasNext();) {
			ClientVolume* clientVolume = it.Next().value;
			if (DirectoryContains(clientVolume->GetRootDirectory(), node, true))
				clientVolume->ProcessNodeMonitoringEvent(event);
		}
	}
}

// _NodeAttributeChanged
void
VolumeManager::_NodeAttributeChanged(AttributeChangedEvent* event)
{
	// get the node
	Node* node = GetNode(event->volumeID, event->nodeID);
	if (!node)
		return;

	// check, if there is an earlier similar event
	bool notify = true;
	AttributeRef ref(event->volumeID, event->nodeID,
		event->attribute.GetString());
	AttributeChangedEvent* oldEvent = fNodeAttributeChangedEvents->Get(ref);

	// remove the old event
	if (oldEvent) {
		fNodeAttributeChangedEvents->Remove(ref);
		fRecentNodeMonitoringEvents.Remove(oldEvent);
		notify = !_IsRecentEvent(oldEvent);
		oldEvent->ReleaseReference();
	}

	// add the new event
	if (fNodeAttributeChangedEvents->Put(ref, event) == B_OK) {
		fRecentNodeMonitoringEvents.Insert(event);
		event->AcquireReference();
	}

	// send notifications
	if (notify) {
		for (ClientVolumeMap::Iterator it = fClientVolumes->GetIterator();
			 it.HasNext();) {
			ClientVolume* clientVolume = it.Next().value;
			if (DirectoryContains(clientVolume->GetRootDirectory(), node, true))
				clientVolume->ProcessNodeMonitoringEvent(event);
		}
	}
}

// _VolumeMounted
void
VolumeManager::_VolumeMounted(VolumeMountedEvent* event)
{
	entry_ref rootRef;
	bool rootRefInitialized = false;

	// remove the entry referring to the covered directory
	Directory* coveredDirectory = GetDirectory(event->volumeID,
		event->directoryID);
	if (coveredDirectory) {
		if (Entry* entry = coveredDirectory->GetActualReferringEntry()) {
			// get an entry for later
			rootRef = entry->GetEntryRef();
			rootRefInitialized = true;

			// send the "entry removed" event
			EntryRemovedEvent* event;
			if (_GenerateEntryRemovedEvent(entry, system_time(),
					&event) == B_OK) {
				_EntryRemoved(event, true);
				event->ReleaseReference();
			} else {
				RemoveEntry(entry);
				delete entry;
			}
		}
	}

	// add the volume
	_AddVolume(event->newVolumeID);

	// generate an "entry created" event for the root dir entry
	if (rootRefInitialized)
		_GenerateEntryCreatedEvent(rootRef, event->time);
}

// _VolumeUnmounted
void
VolumeManager::_VolumeUnmounted(VolumeUnmountedEvent* event)
{
	// get the volume
	Volume* volume = GetVolume(event->volumeID);
	if (!volume)
		return;

	entry_ref rootRef;
	bool rootRefInitialized = false;

	// remove all actual entries referring to the root directory (should only
	// be one)
	if (Directory* rootDir = volume->GetRootDirectory()) {
		// get an entry ref for the root dir
		if (Entry* entry = rootDir->GetActualReferringEntry()) {
			rootRef = entry->GetEntryRef();
			rootRefInitialized = true;
		}

		Entry* entry = rootDir->GetFirstReferringEntry();
		while (entry) {
			Entry* nextEntry = rootDir->GetNextReferringEntry(entry);

			if (entry->IsActualEntry()) {
				EntryRemovedEvent* removedEvent;
				if (_GenerateEntryRemovedEvent(entry, event->time,
						&removedEvent) == B_OK) {
					_EntryRemoved(removedEvent, true);
					removedEvent->ReleaseReference();
				} else {
					RemoveEntry(entry);
					delete entry;
				}
			}

			entry = nextEntry;
		}
	}

	// remove all entries of the volume
	while (Entry* entry = volume->GetFirstEntry()) {
		bool remove = true;
		if (entry->IsActualEntry()) {
			if (_GenerateEntryRemovedEvent(entry, event->time) != B_OK)
				remove = false;
		}

		if (remove) {
			RemoveEntry(entry);
			delete entry;
		}
	}

	// remove all nodes
	while (Node* node = volume->GetFirstNode()) {
		RemoveNode(node);
		if (node != volume->GetRootDirectory())
			delete node;
	}

	// remove the volume
	fVolumes->Remove(volume->GetID());
	delete volume;

	// generate an "entry created" event for the covered node
	if (rootRefInitialized)
		_GenerateEntryCreatedEvent(rootRef, event->time);
}

// _QueryEntryCreated
void
VolumeManager::_QueryEntryCreated(EntryCreatedEvent* event)
{
	// get the query handler
	QueryHandler* queryHandler
		= dynamic_cast<QueryHandler*>(event->queryHandler);
	if (!queryHandler)
		return;

	// load the entry (just to make sure that it really exists)
	Entry* entry = NULL;
	status_t error = LoadEntry(event->volumeID, event->directoryID,
		event->name.GetString(), true, &entry);
	if (error != B_OK)
		return;

	// get remote port and token
	if (!queryHandler->LockLooper())
		return;

	QueryHandle* queryHandle = queryHandler->GetQueryHandle();
	event->remotePort = queryHandle->GetRemotePort();
	event->remoteToken = queryHandle->GetRemoteToken();
	queryHandler->UnlockLooper();

	// send a notification to the client volume
	queryHandler->GetQueryDomain()->ProcessQueryEvent(event);
}

// _QueryEntryRemoved
void
VolumeManager::_QueryEntryRemoved(EntryRemovedEvent* event)
{
	// get the query handler
	QueryHandler* queryHandler
		= dynamic_cast<QueryHandler*>(event->queryHandler);
	if (!queryHandler)
		return;

	// load the directory (just to make sure that it really exists)
	Directory* directory = NULL;
	status_t error = LoadDirectory(event->volumeID, event->directoryID,
		&directory);
	if (error != B_OK)
		return;

	// get remote port and token
	if (!queryHandler->LockLooper())
		return;
	QueryHandle* queryHandle = queryHandler->GetQueryHandle();
	event->remotePort = queryHandle->GetRemotePort();
	event->remoteToken = queryHandle->GetRemoteToken();
	queryHandler->UnlockLooper();

	// send a notification to the client volume
	queryHandler->GetQueryDomain()->ProcessQueryEvent(event);
}

// _QueryEntryMoved
void
VolumeManager::_QueryEntryMoved(EntryMovedEvent* event)
{
	// we simply split the event into a `removed' and a `created' event

	// allocate the events
	EntryRemovedEvent* removedEvent = new(std::nothrow) EntryRemovedEvent;
	EntryCreatedEvent* createdEvent = new(std::nothrow) EntryCreatedEvent;
	if (!removedEvent || !createdEvent) {
		delete removedEvent;
		delete createdEvent;
		return;
	}

	// init the removed event
	removedEvent->opcode = B_ENTRY_REMOVED;
	removedEvent->time = event->time;
	removedEvent->queryHandler = event->queryHandler;
	removedEvent->queryHandler->AcquireReference();
	removedEvent->volumeID = event->volumeID;
	removedEvent->directoryID = event->fromDirectoryID;
	removedEvent->nodeVolumeID = event->volumeID;
	removedEvent->nodeID = event->nodeID;
	removedEvent->name = event->fromName;

	// init the created event
	createdEvent->opcode = B_ENTRY_CREATED;
	createdEvent->time = event->time;
	createdEvent->queryHandler = event->queryHandler;
	createdEvent->queryHandler->AcquireReference();
	createdEvent->volumeID = event->volumeID;
	createdEvent->directoryID = event->toDirectoryID;
	createdEvent->nodeID = event->nodeID;
	createdEvent->name = event->toName;

	// send them
	_QueryEntryRemoved(removedEvent);
	removedEvent->ReleaseReference();
	_QueryEntryCreated(createdEvent);
	createdEvent->ReleaseReference();
}

// _IsRecentEvent
bool
VolumeManager::_IsRecentEvent(NodeMonitoringEvent* event) const
{
	return (event && system_time() < event->time + kRecentEventLifeTime);
}

// _GenerateEntryCreatedEvent
status_t
VolumeManager::_GenerateEntryCreatedEvent(const entry_ref& ref, bigtime_t time,
	EntryCreatedEvent** _event)
{
	// load the entry
	Entry* entry;
	status_t error = LoadEntry(ref.device, ref.directory, ref.name, true,
		&entry);
	if (error != B_OK)
		return error;

	// create the event
	EntryCreatedEvent* event = new(std::nothrow) EntryCreatedEvent;
	if (!event)
		return B_NO_MEMORY;

	// fill in the fields
	event->opcode = B_ENTRY_CREATED;
	event->time = time;
	event->volumeID = entry->GetVolumeID();
	event->directoryID = entry->GetDirectoryID();
	event->nodeID = entry->GetNode()->GetID();
	event->name.SetTo(entry->GetName());

	if (_event) {
		*_event = event;
	} else {
		_EntryCreated(event);
		event->ReleaseReference();
	}

	return B_OK;
}

// _GenerateEntryRemovedEvent
status_t
VolumeManager::_GenerateEntryRemovedEvent(Entry* entry, bigtime_t time,
	EntryRemovedEvent** _event)
{
	if (!entry)
		return B_BAD_VALUE;

	// create the event
	EntryRemovedEvent* event = new(std::nothrow) EntryRemovedEvent;
	if (!event)
		return B_NO_MEMORY;

	// fill in the fields
	event->opcode = B_ENTRY_REMOVED;
	event->time = time;
	event->volumeID = entry->GetVolumeID();
	event->directoryID = entry->GetDirectoryID();
	event->nodeVolumeID = entry->GetNode()->GetVolumeID();
	event->nodeID = entry->GetNode()->GetID();
	event->name.SetTo(entry->GetName());

	if (_event) {
		*_event = event;
	} else {
		_EntryRemoved(event, false);
		event->ReleaseReference();
	}

	return B_OK;
}

// _CheckVolumeRootMoved
void
VolumeManager::_CheckVolumeRootMoved(EntryMovedEvent* event)
{
	// If a volume root is moved, the sent node monitoring message does
	// unforunately contain the node_ref of the covered node, not that of the
	// volume root -- a BeOS R5 VFS bug. Since we have the entry_ref of the
	// new entry, we can stat the node.

	// check whether the node is the root of a volume
	NoAllocEntryRef ref(event->volumeID, event->toDirectoryID,
		event->toName.GetString());
	BEntry entry;
	struct stat st;
	if (FDManager::SetEntry(&entry, &ref) == B_OK
		&& entry.GetStat(&st) == B_OK) {
		event->nodeVolumeID = st.st_dev;
		event->nodeID = st.st_ino;
		if (Volume* volume = GetVolume(st.st_dev)) {
			if (volume->GetRootID() == st.st_ino) {
				PRINT("Mount point for volume %ld renamed\n",
					volume->GetID());
			}
		}
	}
}

// _NodeMonitoringProcessorEntry
int32
VolumeManager::_NodeMonitoringProcessorEntry(void* data)
{
	return ((VolumeManager*)data)->_NodeMonitoringProcessor();
}

// _NodeMonitoryProcessor
int32
VolumeManager::_NodeMonitoringProcessor()
{
	do {
		NodeMonitoringEvent* event = NULL;
		status_t error = fNodeMonitoringEvents.Pop(&event);

		VolumeManagerLocker managerLocker;

		while (error == B_OK) {
			if (event->queryHandler) {
				switch (event->opcode) {
					case B_ENTRY_CREATED:
						_QueryEntryCreated(
							dynamic_cast<EntryCreatedEvent*>(event));
						break;
					case B_ENTRY_REMOVED:
						_QueryEntryRemoved(
							dynamic_cast<EntryRemovedEvent*>(event));
						break;
					case B_ENTRY_MOVED:
						_QueryEntryMoved(dynamic_cast<EntryMovedEvent*>(event));
						break;
				}
			} else {
				switch (event->opcode) {
					case B_ENTRY_CREATED:
						_EntryCreated(dynamic_cast<EntryCreatedEvent*>(event));
						break;
					case B_ENTRY_REMOVED:
						_EntryRemoved(dynamic_cast<EntryRemovedEvent*>(event),
							false);
						break;
					case B_ENTRY_MOVED:
						_EntryMoved(dynamic_cast<EntryMovedEvent*>(event));
						break;
					case B_STAT_CHANGED:
						_NodeStatChanged(
							dynamic_cast<StatChangedEvent*>(event));
						break;
					case B_ATTR_CHANGED:
						_NodeAttributeChanged(
							dynamic_cast<AttributeChangedEvent*>(event));
						break;
					case B_DEVICE_MOUNTED:
						_VolumeMounted(dynamic_cast<VolumeMountedEvent*>(event));
						break;
					case B_DEVICE_UNMOUNTED:
						_VolumeUnmounted(
							dynamic_cast<VolumeUnmountedEvent*>(event));
						break;
				}
			}
			event->ReleaseReference();

			// If there is another event available, get it as long as we
			// have the VolumeManager lock.
			error = fNodeMonitoringEvents.Pop(&event, 0);
		}
	} while (!fTerminating);

	return 0;
}


// sManager
VolumeManager* VolumeManager::sManager = NULL;
