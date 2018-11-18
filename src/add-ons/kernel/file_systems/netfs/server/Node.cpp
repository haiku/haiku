// Node.cpp

#include <errno.h>
#include <fcntl.h>
#include <new>
#include <unistd.h>
#include <sys/stat.h>

#include <AutoDeleter.h>
#include <fs_attr.h>

#include "DebugSupport.h"
#include "Entry.h"
#include "FDManager.h"
#include "Node.h"
#include "NodeHandle.h"
#include "Path.h"
#include "VolumeManager.h"

// constructor
Node::Node(Volume* volume, const struct stat& st)
	: AttributeDirectory(),
	  fVolume(volume),
	  fStat(st),
	  fReferringEntries()
{
}

// destructor
Node::~Node()
{
}

// GetVolume
Volume*
Node::GetVolume() const
{
	return fVolume;
}

// GetNodeRef
node_ref
Node::GetNodeRef() const
{
	node_ref ref;
	ref.device = fStat.st_dev;
	ref.node = fStat.st_ino;
	return ref;
}

// GetVolumeID
dev_t
Node::GetVolumeID() const
{
	return fStat.st_dev;
}

// GetID
ino_t
Node::GetID() const
{
	return fStat.st_ino;
}

// AddReferringEntry
void
Node::AddReferringEntry(Entry* entry)
{
	if (!entry)
		return;
	fReferringEntries.Insert(entry);
}

// RemoveReferringEntry
void
Node::RemoveReferringEntry(Entry* entry)
{
	if (entry)
		fReferringEntries.Remove(entry);
}

// GetFirstReferringEntry
Entry*
Node::GetFirstReferringEntry() const
{
	return fReferringEntries.First();
}

// GetNextReferringEntry
Entry*
Node::GetNextReferringEntry(Entry* entry) const
{
	return (entry ? fReferringEntries.GetNext(entry) : NULL);
}

// FindReferringEntry
Entry*
Node::FindReferringEntry(dev_t volumeID, ino_t directoryID, const char* name)
{
	if (!name)
		return NULL;

	NoAllocEntryRef ref(volumeID, directoryID, name);
	return FindReferringEntry(ref);
}

// FindReferringEntry
Entry*
Node::FindReferringEntry(const entry_ref& entryRef)
{
	for (Entry* entry = GetFirstReferringEntry();
		 entry;
		 entry = GetNextReferringEntry(entry)) {
		NoAllocEntryRef ref(entry->GetEntryRef());
		if (ref == entryRef)
			return entry;
	}

	return NULL;
}

// GetActualReferringEntry
Entry*
Node::GetActualReferringEntry() const
{
	return GetFirstReferringEntry();
}

// GetStat
const struct stat&
Node::GetStat() const
{
	return fStat;
}

// UpdateStat
status_t
Node::UpdateStat()
{
	// get a path
	Path path;
	status_t error = GetPath(&path);
	if (error != B_OK)
		return error;

	// read stat
	struct stat st;
	if (lstat(path.GetPath(), &st) < 0)
		return errno;

	// check if it is the same node
	if (st.st_dev != fStat.st_dev || st.st_ino != fStat.st_ino) {
		ERROR("Node::UpdateStat(): ERROR: GetPath() returned path that "
			"doesn't point to this node: node: (%" B_PRIdDEV ", %" B_PRIdINO "), "
			"path: `%s'\n",
			GetVolumeID(), GetID(), path.GetPath());
		return B_ENTRY_NOT_FOUND;
	}

	fStat = st;
	return B_OK;
}

// IsDirectory
bool
Node::IsDirectory() const
{
	return S_ISDIR(fStat.st_mode);
}

// IsFile
bool
Node::IsFile() const
{
	return S_ISREG(fStat.st_mode);
}

// IsSymlink
bool
Node::IsSymlink() const
{
	return S_ISLNK(fStat.st_mode);
}

// GetPath
status_t
Node::GetPath(Path* path)
{
	return VolumeManager::GetDefault()->GetPath(this, path);
}

// Open
status_t
Node::Open(int openMode, FileHandle** _fileHandle)
{
	if (!_fileHandle)
		return B_BAD_VALUE;

	// allocate the file handle
	FileHandle* fileHandle = new(std::nothrow) FileHandle;
	if (!fileHandle)
		return B_NO_MEMORY;
	ObjectDeleter<FileHandle> fileHandleDeleter(fileHandle);

	// open the file
	status_t error = fileHandle->Open(this, openMode);
	if (error != B_OK)
		return error;

	// check, if it really belongs to this node
	error = _CheckNodeHandle(fileHandle);
	if (error != B_OK)
		return error;

	fileHandleDeleter.Detach();
	*_fileHandle = fileHandle;
	return B_OK;
}

// OpenAttrDir
status_t
Node::OpenAttrDir(AttrDirIterator** _iterator)
{
	if (!_iterator)
		return B_BAD_VALUE;

	// allocate the file handle
	AttrDirIterator* iterator = new(std::nothrow) AttrDirIterator;
	if (!iterator)
		return B_NO_MEMORY;
	ObjectDeleter<AttrDirIterator> iteratorDeleter(iterator);

	// open the attr dir
	status_t error = iterator->Open(this);
	if (error != B_OK)
		return error;

	// check, if it really belongs to this node
	error = _CheckNodeHandle(iterator);
	if (error != B_OK)
		return error;

	iteratorDeleter.Detach();
	*_iterator = iterator;
	return B_OK;
}

// OpenNode
status_t
Node::OpenNode(BNode& node)
{
	Entry* entry = GetActualReferringEntry();
	if (!entry)
		return B_ENTRY_NOT_FOUND;

	NoAllocEntryRef entryRef(entry->GetEntryRef());
	return FDManager::SetNode(&node, &entryRef);
}

// ReadSymlink
status_t
Node::ReadSymlink(char* buffer, int32 bufferSize, int32* _bytesRead)
{
	if (!buffer || bufferSize < 1)
		return B_BAD_VALUE;

	// get a path
	Path path;
	status_t error = GetPath(&path);
	if (error != B_OK)
		return error;

	// read the symlink
	ssize_t bytesRead = readlink(path.GetPath(), buffer, bufferSize);
	if (bytesRead < 0)
		return bytesRead;
	if (bytesRead < bufferSize)
		buffer[bytesRead] = '\0';
	else
		buffer[bufferSize - 1] = '\0';

	if (_bytesRead)
		*_bytesRead = bytesRead;
	return B_OK;
}

// _CheckNodeHandle
status_t
Node::_CheckNodeHandle(NodeHandle* nodeHandle)
{
	if (!nodeHandle)
		return B_BAD_VALUE;

	// read the stat
	struct stat st;
	status_t error = nodeHandle->GetStat(&st);
	if (error != B_OK)
		return error;

	// check if it is the same node
	if (st.st_dev != fStat.st_dev || st.st_ino != fStat.st_ino)
		return B_ENTRY_NOT_FOUND;
	return B_OK;
}

