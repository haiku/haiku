// Volume.cpp

#include <new>

#include <sys/stat.h>

#include <Directory.h>
#include <fs_info.h>

#include "Directory.h"
#include "FDManager.h"
#include "Entry.h"
#include "HashMap.h"
#include "Node.h"
#include "Volume.h"

// NodeMap
struct Volume::NodeMap : HashMap<HashKey64<ino_t>, Node*> {
};

// EntryKey
//
// NOTE: This class doesn't make a copy of the name string it is constructed
// with. So, when entering the key in a map, one must make sure, that the
// string stays valid as long as the entry is in the map.
struct Volume::EntryKey {
	EntryKey() {}

	EntryKey(ino_t directoryID, const char* name)
		: directoryID(directoryID),
		  name(name)
	{
	}

	EntryKey(Entry* entry)
		: directoryID(entry->GetDirectoryID()),
		  name(entry->GetName())
	{
	}

	EntryKey(const EntryKey& other)
		: directoryID(other.directoryID),
		  name(other.name)
	{
	}

	uint32 GetHashCode() const
	{
		uint32 hash = (uint32)directoryID;
		hash = 31 * hash + (uint32)(directoryID >> 32);
		hash = 31 * hash + string_hash(name);
		return hash;
	}

	EntryKey& operator=(const EntryKey& other)
	{
		directoryID = other.directoryID;
		name = other.name;
		return *this;
	}

	bool operator==(const EntryKey& other) const
	{
		if (directoryID != other.directoryID)
			return false;

		if (name)
			return (other.name && strcmp(name, other.name) == 0);

		return !other.name;
	}

	bool operator!=(const EntryKey& other) const
	{
		return !(*this == other);
	}

	ino_t		directoryID;
	const char*	name;
};

// EntryMap
struct Volume::EntryMap : HashMap<EntryKey, Entry*> {
};


// constructor
Volume::Volume(dev_t id)
	: fID(id),
	  fRootDir(NULL),
	  fFSFlags(0),
	  fNodes(NULL),
	  fEntries(NULL)
{
}

// destructor
Volume::~Volume()
{
	// delete all entries
	if (fEntries) {
		for (EntryMap::Iterator it = fEntries->GetIterator(); it.HasNext();) {
			Entry* entry = it.Next().value;
			delete entry;
		}
		delete fEntries;
	}

	// delete all nodes
	if (fNodes) {
		// remove the root dir -- we delete it separately
		if (fRootDir)
			RemoveNode(fRootDir);

		// delete the nodes
		for (NodeMap::Iterator it = fNodes->GetIterator(); it.HasNext();) {
			Node* node = it.Next().value;
			delete node;
		}

		delete fNodes;
	}

	// delete the root dir
	delete fRootDir;
}

// Init
status_t
Volume::Init()
{
	// create the node map
	fNodes = new(nothrow) NodeMap;
	if (!fNodes)
		return B_NO_MEMORY;
	if (fNodes->InitCheck() != B_OK)
		return fNodes->InitCheck();

	// create the entry map
	fEntries = new(nothrow) EntryMap;
	if (!fEntries)
		return B_NO_MEMORY;
	if (fEntries->InitCheck() != B_OK)
		return fEntries->InitCheck();

	// get a volume info
	fs_info info;
	status_t error = fs_stat_dev(fID, &info);
	if (error != B_OK)
		return error;
	fFSFlags = info.flags;

	// open the root directory
	node_ref rootRef;
	rootRef.device = fID;
	rootRef.node = info.root;
	BDirectory rootDir;
	error = FDManager::SetDirectory(&rootDir, &rootRef);
	if (error != B_OK)
		return error;

	// stat the root dir
	struct stat st;
	error = rootDir.GetStat(&st);
	if (error != B_OK)
		return error;

	// create the root dir
	fRootDir = new(nothrow) Directory(this, st);
	if (!fRootDir)
		return B_NO_MEMORY;

	// the root dir is added by the VolumeManager

	return B_OK;
}

// GetID
dev_t
Volume::GetID() const
{
	return fID;
}

// GetRootDirectory
Directory*
Volume::GetRootDirectory() const
{
	return fRootDir;
}

// GetRootID
ino_t
Volume::GetRootID() const
{
	return fRootDir->GetID();
}

// KnowsQuery
bool
Volume::KnowsQuery() const
{
	return (fFSFlags & B_FS_HAS_QUERY);
}


// AddNode
status_t
Volume::AddNode(Node* node)
{
	if (!node || node->GetVolume() != this || GetNode(node->GetID()))
		return B_BAD_VALUE;

	return fNodes->Put(node->GetID(), node);
}

// RemoveNode
bool
Volume::RemoveNode(Node* node)
{
	if (node && GetNode(node->GetID()) == node) {
		fNodes->Remove(node->GetID());
		return true;
	}

	return false;
}

// GetNode
Node*
Volume::GetNode(ino_t nodeID)
{
	return fNodes->Get(nodeID);
}

// GetFirstNode
Node*
Volume::GetFirstNode() const
{
	NodeMap::Iterator it = fNodes->GetIterator();
	if (it.HasNext())
		return it.Next().value;
	return NULL;
}

// AddEntry
status_t
Volume::AddEntry(Entry* entry)
{
	if (!entry || entry->GetVolume() != this
		|| GetEntry(entry->GetDirectoryID(), entry->GetName())) {
		return B_BAD_VALUE;
	}

	return fEntries->Put(EntryKey(entry), entry);
}

// RemoveEntry
bool
Volume::RemoveEntry(Entry* entry)
{
	if (entry && GetEntry(entry->GetDirectoryID(), entry->GetName()) == entry) {
		fEntries->Remove(EntryKey(entry));
		return true;
	}

	return false;
}

// GetEntry
Entry*
Volume::GetEntry(ino_t dirID, const char* name)
{
	return fEntries->Get(EntryKey(dirID, name));
}

// GetFirstEntry
Entry*
Volume::GetFirstEntry() const
{
	EntryMap::Iterator it = fEntries->GetIterator();
	if (it.HasNext())
		return it.Next().value;
	return NULL;
}

