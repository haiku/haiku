// ShareNode.cpp

#include "ShareAttrDir.h"
#include "ShareNode.h"

// constructor
ShareDirEntry::ShareDirEntry(ShareDir* directory, const char* name,
	ShareNode* node)
	: Referencable(true),
	  fDirectory(directory),
	  fName(name),
	  fNode(node),
	  fRevision(-1)
{
}

// destructor
ShareDirEntry::~ShareDirEntry()
{
}

// InitCheck
status_t
ShareDirEntry::InitCheck() const
{
	if (fName.GetLength() == 0)
		return B_NO_MEMORY;

	return B_OK;
}

// GetDirectory
ShareDir*
ShareDirEntry::GetDirectory() const
{
	return fDirectory;
}

// GetName
const char*
ShareDirEntry::GetName() const
{
	return fName.GetString();
}

// GetNode
ShareNode*
ShareDirEntry::GetNode() const
{
	return fNode;
}

// SetRevision
void
ShareDirEntry::SetRevision(int64 revision)
{
	fRevision = revision;
}

// GetRevision
int64
ShareDirEntry::GetRevision() const
{
	return fRevision;
}

// IsActualEntry
bool
ShareDirEntry::IsActualEntry() const
{
	return (fName.GetLength() > 0 && fName != "." && fName != "..");
}


// #pragma mark -

// constructor
ShareNode::ShareNode(Volume* volume, vnode_id id, const NodeInfo* nodeInfo)
	: Node(volume, id),
	  fInfo(),
	  fReferringEntries(),
	  fAttrDir(NULL)
{
	if (nodeInfo) {
		fInfo = *nodeInfo;
	} else {
		// init the stat data at least a bit, if no node info is given
		fInfo.st.st_dev = -1;
		fInfo.st.st_ino = -1;
		fInfo.st.st_mode = S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP
			| S_IROTH | S_IXOTH;
		fInfo.st.st_nlink = 1;
		fInfo.st.st_size = 1;
		fInfo.st.st_blksize = 1024;
		fInfo.st.st_crtime = 0;
		fInfo.st.st_ctime = fInfo.st.st_mtime = fInfo.st.st_atime
			= fInfo.st.st_crtime;

		// negative revision, to make sure it is updated
		fInfo.revision = -1;
	}
}

// destructor
ShareNode::~ShareNode()
{
	delete fAttrDir;
}

// GetNodeInfo
const NodeInfo&
ShareNode::GetNodeInfo() const
{
	return fInfo;
}

// GetRemoteID
NodeID
ShareNode::GetRemoteID() const
{
	return fInfo.GetID();
}

// Update
void
ShareNode::Update(const NodeInfo& nodeInfo)
{
	if (fInfo.revision < nodeInfo.revision)
		fInfo = nodeInfo;
}

// AddReferringEntry
void
ShareNode::AddReferringEntry(ShareDirEntry* entry)
{
	if (entry)
		fReferringEntries.Insert(entry);
}

// RemoveReferringEntry
void
ShareNode::RemoveReferringEntry(ShareDirEntry* entry)
{
	if (entry)
		fReferringEntries.Remove(entry);
}

// GetFirstReferringEntry
ShareDirEntry*
ShareNode::GetFirstReferringEntry() const
{
	return fReferringEntries.GetFirst();
}

// GetNextReferringEntry
ShareDirEntry*
ShareNode::GetNextReferringEntry(ShareDirEntry* entry) const
{
	return (entry ? fReferringEntries.GetNext(entry) : NULL);
}

// GetActualReferringEntry
ShareDirEntry*
ShareNode::GetActualReferringEntry() const
{
	for (ShareDirEntry* entry = GetFirstReferringEntry();
		 entry;
		 entry = GetNextReferringEntry(entry)) {
		if (entry->IsActualEntry())
			return entry;
	}

	return NULL;
}

// SetAttrDir
void
ShareNode::SetAttrDir(ShareAttrDir* attrDir)
{
	delete fAttrDir;
	fAttrDir = attrDir;
}

// GetAttrDir
ShareAttrDir*
ShareNode::GetAttrDir() const
{
	return fAttrDir;
}


// #pragma mark -

// constructor
ShareDirIterator::ShareDirIterator()
{
}

// destructor
ShareDirIterator::~ShareDirIterator()
{
}


// #pragma mark -

// constructor
LocalShareDirIterator::LocalShareDirIterator()
	: fDirectory(NULL),
	  fCurrentEntry(NULL)
{
}

// destructor
LocalShareDirIterator::~LocalShareDirIterator()
{
	SetDirectory(NULL);
}

// SetDirectory
void
LocalShareDirIterator::SetDirectory(ShareDir* directory)
{
	// unset the old directory
	if (fDirectory)
		fDirectory->RemoveDirIterator(this);

	// set the new directory
	fDirectory = directory;
	if (fDirectory) {
		fDirectory->AddDirIterator(this);
		fCurrentEntry = fDirectory->GetFirstEntry();
	}
}

// GetCurrentEntry
ShareDirEntry*
LocalShareDirIterator::GetCurrentEntry() const
{
	return fCurrentEntry;
}

// NextEntry
void
LocalShareDirIterator::NextEntry()
{
	if (!fDirectory || !fCurrentEntry)
		return;

	fCurrentEntry = fDirectory->GetNextEntry(fCurrentEntry);
}

// Rewind
void
LocalShareDirIterator::Rewind()
{
	fCurrentEntry = (fDirectory ? fDirectory->GetFirstEntry() : NULL);
}

// IsDone
bool
LocalShareDirIterator::IsDone() const
{
	return !fCurrentEntry;
}


// #pragma mark -

// constructor
RemoteShareDirIterator::RemoteShareDirIterator()
	: fCookie(-1),
	  fCapacity(kRemoteShareDirIteratorCapacity),
	  fCount(0),
	  fIndex(0),
	  fRevision(-1),
	  fDone(false),
	  fRewind(false)
{
}

// destructor
RemoteShareDirIterator::~RemoteShareDirIterator()
{
	Clear();
}

// GetCurrentEntry
ShareDirEntry*
RemoteShareDirIterator::GetCurrentEntry() const
{
	return (!fRewind && fIndex < fCount ? fEntries[fIndex] : NULL);
}

// NextEntry
void
RemoteShareDirIterator::NextEntry()
{
	if (fIndex < fCount)
		fIndex++;
}

// Rewind
void
RemoteShareDirIterator::Rewind()
{
	fRewind = true;
	fDone = false;
}

// IsDone
bool
RemoteShareDirIterator::IsDone() const
{
	return fDone;
}

// GetCapacity
int32
RemoteShareDirIterator::GetCapacity() const
{
	return fCapacity;
}

// SetCookie
void
RemoteShareDirIterator::SetCookie(int32 cookie)
{
	fCookie = cookie;
}

// GetCookie
int32
RemoteShareDirIterator::GetCookie() const
{
	return fCookie;
}

// Clear
void
RemoteShareDirIterator::Clear()
{
	for (int32 i = 0; i < fCount; i++)
		fEntries[i]->RemoveReference();
	fCount = 0;
	fIndex = 0;
	fDone = false;
	fRewind = false;
}

// AddEntry
bool
RemoteShareDirIterator::AddEntry(ShareDirEntry* entry)
{
	if (!entry || fCount >= fCapacity)
		return false;

	fEntries[fCount++] = entry;
	entry->AddReference();
	return true;
}

// SetRevision
void
RemoteShareDirIterator::SetRevision(int64 revision)
{
	fRevision = revision;
}

// GetRevision
int64
RemoteShareDirIterator::GetRevision() const
{
	return fRevision;
}

// SetDone
void
RemoteShareDirIterator::SetDone(bool done)
{
	fDone = done;
}

// GetRewind
bool
RemoteShareDirIterator::GetRewind() const
{
	return fRewind;
}


// #pragma mark -

// constructor
ShareDir::ShareDir(Volume* volume, vnode_id id, const NodeInfo* nodeInfo)
	: ShareNode(volume, id, nodeInfo),
	  fEntries(),
	  fIterators(),
	  fEntryCreatedEventRevision(-1),
	  fEntryRemovedEventRevision(-1),
	  fIsComplete(false)
{
}

// destructor
ShareDir::~ShareDir()
{
}

// UpdateEntryCreatedEventRevision
void
ShareDir::UpdateEntryCreatedEventRevision(int64 revision)
{
	if (revision > fEntryCreatedEventRevision)
		fEntryCreatedEventRevision = revision;
}

// GetEntryCreatedEventRevision
int64
ShareDir::GetEntryCreatedEventRevision() const
{
	return fEntryCreatedEventRevision;
}

// UpdateEntryRemovedEventRevision
void
ShareDir::UpdateEntryRemovedEventRevision(int64 revision)
{
	if (revision > fEntryRemovedEventRevision)
		fEntryRemovedEventRevision = revision;
}

// GetEntryRemovedEventRevision
int64
ShareDir::GetEntryRemovedEventRevision() const
{
	return fEntryRemovedEventRevision;
}

// SetComplete
void
ShareDir::SetComplete(bool complete)
{
	fIsComplete = complete;
}

// IsComplete
bool
ShareDir::IsComplete() const
{
	return fIsComplete;
}

// AddEntry
void
ShareDir::AddEntry(ShareDirEntry* entry)
{
	if (entry)
		fEntries.Insert(entry);
}

// RemoveEntry
void
ShareDir::RemoveEntry(ShareDirEntry* entry)
{
	if (entry) {
		// update the directory iterators pointing to the removed entry
		for (LocalShareDirIterator* iterator = fIterators.GetFirst();
			 iterator;
			 iterator = fIterators.GetNext(iterator)) {
			if (iterator->GetCurrentEntry() == entry)
				iterator->NextEntry();
		}

		fEntries.Remove(entry);
	}
}

// GetFirstEntry
ShareDirEntry*
ShareDir::GetFirstEntry() const
{
	return fEntries.GetFirst();
}

// GetNextEntry
ShareDirEntry*
ShareDir::GetNextEntry(ShareDirEntry* entry) const
{
	if (!entry)
		return NULL;

	return fEntries.GetNext(entry);
}

// AddDirIterator
void
ShareDir::AddDirIterator(LocalShareDirIterator* iterator)
{
	if (!iterator)
		return;

	fIterators.Insert(iterator);
}

// RemoveDirIterator
void
ShareDir::RemoveDirIterator(LocalShareDirIterator* iterator)
{
	if (!iterator)
		return;

	fIterators.Remove(iterator);
}

