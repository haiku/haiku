// VirtualVolume.cpp

#include <new>
#include <stdio.h>
#include <string.h>

#include "AutoLocker.h"
#include "Compatibility.h"
#include "Debug.h"
#include "QueryIterator.h"
#include "QueryManager.h"
#include "VirtualDir.h"
#include "VirtualVolume.h"
#include "VolumeManager.h"
#include "VolumeSupport.h"

// constructor
VirtualVolume::VirtualVolume(VolumeManager* volumeManager)
	: Volume(volumeManager),
	  fRootNode(NULL)
{
}

// destructor
VirtualVolume::~VirtualVolume()
{
	delete fRootNode;
}

// Init
status_t
VirtualVolume::Init(const char* name)
{
	status_t error = Volume::Init(name);
	if (error != B_OK)
		return error;

	// get an ID for the root node
	vnode_id rootNodeID = fVolumeManager->NewNodeID(this);
	if (rootNodeID < 0) {
		Uninit();
		return rootNodeID;
	}

	// create the root node
	fRootNode = new(nothrow) VirtualDir(this, rootNodeID);
	if (!fRootNode) {
		Uninit();
		fVolumeManager->RemoveNodeID(rootNodeID);
		return B_NO_MEMORY;
	}
	error = fRootNode->InitCheck();
	if (error != B_OK) {
		Uninit();
		return error;
	}

	return B_OK;
}

// Uninit
void
VirtualVolume::Uninit()
{
	if (fRootNode) {
		fVolumeManager->RemoveNodeID(fRootNode->GetID());
		delete fRootNode;
		fRootNode = NULL;
	}

	Volume::Uninit();
}

// GetRootNode
Node*
VirtualVolume::GetRootNode() const
{
	return fRootNode;
}

// PrepareToUnmount
void
VirtualVolume::PrepareToUnmount()
{
	Volume::PrepareToUnmount();

	// remove all child volumes

	// init a directory iterator
	fLock.Lock();
	VirtualDirIterator iterator;
	iterator.SetDirectory(fRootNode, true);

	// iterate through the directory
	const char* name;
	Node* node;
	while (iterator.GetCurrentEntry(&name, &node)) {
		iterator.NextEntry();
		Volume* volume = fVolumeManager->GetVolume(node->GetID());
		fLock.Unlock();
		if (volume) {
			RemoveChildVolume(volume);
			volume->SetUnmounting(true);
			volume->PutVolume();
		}
		fLock.Lock();
	}

	// uninit the directory iterator
	iterator.SetDirectory(NULL);

	// remove the our root node
	vnode_id rootNodeID = fRootNode->GetID();

	fLock.Unlock();

	if (GetVNode(rootNodeID, &node) == B_OK) {
		Volume::RemoveVNode(rootNodeID);
		PutVNode(rootNodeID);
	}
}

// AddChildVolume
status_t
VirtualVolume::AddChildVolume(Volume* volume)
{
	if (!volume)
		return B_BAD_VALUE;

	// don't add anything, if we are already unmounting
	AutoLocker<Locker> locker(fLock);
	if (fUnmounting)
		return B_BAD_VALUE;

	// get and check the node name
	char name[B_FILE_NAME_LENGTH];
	int32 nameLen = strlen(volume->GetName());
	if (nameLen == 0 || nameLen >= B_FILE_NAME_LENGTH)
		return B_BAD_VALUE;
	strcpy(name, volume->GetName());

	// add the volume's root node
	status_t error = fRootNode->AddEntry(name, volume->GetRootNode());
	if (error != B_OK)
		return error;

	// set the volume's parent volume
	volume->SetParentVolume(this);
	AddReference();

	// send out a notification
	vnode_id dirID = fRootNode->GetID();
	vnode_id nodeID = volume->GetRootID();
	locker.Unlock();
	NotifyListener(B_ENTRY_CREATED, fVolumeManager->GetID(), dirID, 0, nodeID,
		name);

	return B_OK;
}

// RemoveChildVolume
void
VirtualVolume::RemoveChildVolume(Volume* volume)
{
	if (!volume)
		return;

	// check, if the volume's root node is a child of our root node
	AutoLocker<Locker> locker(fLock);
	Node* node = fRootNode->GetChildNode(volume->GetName());
	if (!node)
		return;
	if (node != volume->GetRootNode())
		return;

	// remove it
	fRootNode->RemoveEntry(volume->GetName());
	volume->SetParentVolume(NULL);

	// get the data needed for the node monitoring notification
	vnode_id dirID = fRootNode->GetID();
	vnode_id nodeID = volume->GetRootID();
	char name[B_FILE_NAME_LENGTH];
	strcpy(name, volume->GetName());

	// surrender the child volumes reference to us
	// Since the caller of this method must have a valid reference to us,
	// this is unproblematic, even if fLock is being held, while this method
	// is invoked.
	locker.Unlock();
	PutVolume();

	// send out a notification
	locker.Unlock();
	NotifyListener(B_ENTRY_REMOVED, fVolumeManager->GetID(), dirID, 0, nodeID,
		name);
}

// GetChildVolume
Volume*
VirtualVolume::GetChildVolume(const char* name)
{
	if (!name)
		return NULL;

	// get the child node of the root node
	AutoLocker<Locker> locker(fLock);
	Node* node = fRootNode->GetChildNode(name);
	if (!node)
		return NULL;

	// get its volume, if it is the root volume
	Volume* volume = node->GetVolume();
	if (volume->GetRootNode() != node)
		return NULL;
	locker.Unlock();

	return fVolumeManager->GetVolume(node->GetID());
}

// GetUniqueEntryName
status_t
VirtualVolume::GetUniqueEntryName(const char* baseName, char* buffer)
{
	if (!baseName || !buffer)
		return B_BAD_VALUE;

	// check the base name len
	int32 baseLen = strlen(baseName);
	if (baseLen == 0 || baseLen >= B_FILE_NAME_LENGTH)
		return B_BAD_VALUE;

	strcpy(buffer, baseName);

	AutoLocker<Locker> _(fLock);

	// adjust the name, if necessary
	int32 suffixNumber = 2;
	while (fRootNode->GetChildNode(baseName)) {
		// create a suffix
		char suffix[13];
		sprintf(suffix, " %ld", suffixNumber);
		suffixNumber++;

		// check the len
		int32 suffixLen = strlen(suffix);
		if (baseLen + suffixLen >= B_FILE_NAME_LENGTH)
			return B_NAME_TOO_LONG;

		// compose the final name
		strcpy(buffer + baseLen, suffix);
	}

	return B_OK;
}


// #pragma mark -
// #pragma mark ----- FS -----

// Unmount
status_t
VirtualVolume::Unmount()
{
	return B_OK;
}

// Sync
status_t
VirtualVolume::Sync()
{
// TODO: Recursively call Sync().
	return B_BAD_VALUE;
}


// #pragma mark -
// #pragma mark ----- vnodes -----

// ReadVNode
status_t
VirtualVolume::ReadVNode(vnode_id vnid, char reenter, Node** node)
{
	if (vnid != GetRootID())
		return B_BAD_VALUE;

	AutoLocker<Locker> _(fLock);
	fRootNode->SetKnownToVFS(true);
	*node = fRootNode;

	// add a volume reference for the node
	AddReference();

	return B_OK;
}

// WriteVNode
status_t
VirtualVolume::WriteVNode(Node* node, char reenter)
{
	if (node != fRootNode)
		return B_BAD_VALUE;

	AutoLocker<Locker> locker(fLock);
	fRootNode->SetKnownToVFS(false);

	// surrender the volume reference of the node
	locker.Unlock();
	PutVolume();

	return B_OK;
}

// RemoveVNode
status_t
VirtualVolume::RemoveVNode(Node* node, char reenter)
{
	if (node != fRootNode)
		return B_BAD_VALUE;

	AutoLocker<Locker> locker(fLock);
	fRootNode->SetKnownToVFS(false);

	// surrender the volume reference of the node
	locker.Unlock();
	PutVolume();

	return B_OK;
}

// #pragma mark -
// #pragma mark ----- nodes -----

// FSync
status_t
VirtualVolume::FSync(Node* node)
{
	return B_OK;
}

// ReadStat
status_t
VirtualVolume::ReadStat(Node* node, struct stat* st)
{
	if (node != fRootNode)
		return B_BAD_VALUE;

	AutoLocker<Locker> _(fLock);
	st->st_ino = node->GetID();
	st->st_mode = S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH
		| S_IXOTH;
	st->st_nlink = 1;
	st->st_size = 1;
	st->st_blksize = 1024;
	st->st_crtime = fRootNode->GetCreationTime();
	st->st_ctime = st->st_mtime = st->st_atime = st->st_crtime;
	st->st_dev = fVolumeManager->GetID();
	// we set the UID/GID fields to the one who mounted the FS
	st->st_uid = fVolumeManager->GetMountUID();
	st->st_gid = fVolumeManager->GetMountGID();
	return B_OK;
}

// WriteStat
status_t
VirtualVolume::WriteStat(Node* node, struct stat *st, uint32 mask)
{
	return B_NOT_ALLOWED;
}

// Access
status_t
VirtualVolume::Access(Node* node, int mode)
{
	// TODO: Implement!
	return B_OK;
}

// #pragma mark -
// #pragma mark ----- files -----

// Create
status_t
VirtualVolume::Create(Node* dir, const char* name, int openMode, int mode,
	vnode_id* vnid, void** cookie)
{
	return B_NOT_ALLOWED;
}

// Open
status_t
VirtualVolume::Open(Node* node, int openMode, void** cookie)
{
	if (node != fRootNode)
		return B_BAD_VALUE;

	// we're read-only
	if ((openMode & O_RWMASK) == O_WRONLY)
		return B_PERMISSION_DENIED;
	if (openMode & O_TRUNC)
		return B_PERMISSION_DENIED;
	if ((openMode & O_RWMASK) == O_RDWR)
		openMode = openMode & ~O_RWMASK | O_RDONLY;

	// set the result
	*cookie = NULL;
	return B_OK;
}

// Close
status_t
VirtualVolume::Close(Node* node, void* cookie)
{
	// no-op: FreeCookie() does the job
	return B_OK;
}

// FreeCookie
status_t
VirtualVolume::FreeCookie(Node* node, void* cookie)
{
	// nothing to do: we didn't allocate anything
	return B_OK;
}

// Read
status_t
VirtualVolume::Read(Node* node, void* cookie, off_t pos, void* _buffer,
	size_t bufferSize, size_t* _bytesRead)
{
	// can't read() directories
	return B_NOT_ALLOWED;
}

// Write
status_t
VirtualVolume::Write(Node* node, void* cookie, off_t pos, const void* _buffer,
	size_t bufferSize, size_t* bytesWritten)
{
	// can't write() directories
	return B_NOT_ALLOWED;
}

// IOCtl
status_t
VirtualVolume::IOCtl(Node* node, void* cookie, int cmd, void* buffer,
	size_t bufferSize)
{
	return B_BAD_VALUE;
}

// SetFlags
status_t
VirtualVolume::SetFlags(Node* node, void* cookie, int flags)
{
	return B_BAD_VALUE;
}

// #pragma mark -
// #pragma mark ----- hard links / symlinks -----

// Link
status_t
VirtualVolume::Link(Node* dir, const char* name, Node* node)
{
	// we're read-only
	return B_NOT_ALLOWED;
}

// Unlink
status_t
VirtualVolume::Unlink(Node* dir, const char* name)
{
	// we're read-only
	return B_NOT_ALLOWED;
}

// Symlink
status_t
VirtualVolume::Symlink(Node* dir, const char* name, const char* target)
{
	// we're read-only
	return B_NOT_ALLOWED;
}

// ReadLink
status_t
VirtualVolume::ReadLink(Node* node, char* buffer, size_t bufferSize,
	size_t* bytesRead)
{
	// there are no symlinks
	return B_BAD_VALUE;
}

// Rename
status_t
VirtualVolume::Rename(Node* oldDir, const char* oldName, Node* newDir,
	const char* newName)
{
	// we're read-only
	return B_NOT_ALLOWED;
}

// #pragma mark -
// #pragma mark ----- directories -----

// MkDir
status_t
VirtualVolume::MkDir(Node* dir, const char* name, int mode)
{
	// we're read-only
	return B_NOT_ALLOWED;
}

// RmDir
status_t
VirtualVolume::RmDir(Node* dir, const char* name)
{
	// we're read-only
	return B_NOT_ALLOWED;
}

// OpenDir
status_t
VirtualVolume::OpenDir(Node* node, void** cookie)
{
	if (node != fRootNode)
		return B_BAD_VALUE;

	// allocate an iterator
	VirtualDirIterator* iterator = new(nothrow) VirtualDirIterator;
	if (!iterator)
		return B_NO_MEMORY;

	AutoLocker<Locker> locker(fLock);
	iterator->SetDirectory(fRootNode);
	*cookie = iterator;
	return B_OK;
}

// CloseDir
status_t
VirtualVolume::CloseDir(Node* node, void* cookie)
{
	// no-op: FreeDirCookie() does the job
	return B_OK;
}

// FreeDirCookie
status_t
VirtualVolume::FreeDirCookie(Node* node, void* cookie)
{
	if (node != fRootNode)
		return B_BAD_VALUE;

	// delete the iterator
	AutoLocker<Locker> locker(fLock);
	VirtualDirIterator* iterator = (VirtualDirIterator*)cookie;
	delete iterator;
	return B_OK;
}

// ReadDir
status_t
VirtualVolume::ReadDir(Node* node, void* cookie, struct dirent* buffer,
	size_t bufferSize, int32 count, int32* countRead)
{
	if (node != fRootNode)
		return B_BAD_VALUE;

	*countRead = 0;

	AutoLocker<Locker> locker(fLock);
	VirtualDirIterator* iterator = (VirtualDirIterator*)cookie;

	// get the current entry
	const char* name;
	Node* child;
	if (!iterator->GetCurrentEntry(&name, &child))
		return B_OK;

	// set the name: this also checks the size of the buffer
	status_t error = set_dirent_name(buffer, bufferSize, name, strlen(name));
	if (error != B_OK)
		RETURN_ERROR(error);

	// fill in the other fields
	buffer->d_pdev = fVolumeManager->GetID();
	buffer->d_pino = fRootNode->GetID();
	buffer->d_dev = fVolumeManager->GetID();
	buffer->d_ino = child->GetID();
	*countRead = 1;

	// fix d_ino, if this is the parent of the root node
	if (strcmp(name, "..") == 0) {
		if (Volume* parentVolume = GetParentVolume())
			buffer->d_ino = parentVolume->GetRootID();
	}

	iterator->NextEntry();
	return B_OK;
}

// RewindDir
status_t
VirtualVolume::RewindDir(Node* node, void* cookie)
{
	if (node != fRootNode)
		return B_BAD_VALUE;

	AutoLocker<Locker> locker(fLock);
	VirtualDirIterator* iterator = (VirtualDirIterator*)cookie;
	iterator->Rewind();

	return B_OK;
}

// Walk
status_t
VirtualVolume::Walk(Node* dir, const char* entryName, char** resolvedPath,
	vnode_id* vnid)
{
	if (dir != fRootNode)
		return B_BAD_VALUE;

	// get the referred to node ID
	AutoLocker<Locker> locker(fLock);
	if (strcmp(entryName, ".") == 0) {
		*vnid = dir->GetID();
	} else if (strcmp(entryName, "..") == 0) {
		if (Volume* parentVolume = GetParentVolume())
			*vnid = parentVolume->GetRootID();
		else 
			*vnid = dir->GetID();
	} else {
		Node* node = fRootNode->GetChildNode(entryName);
		if (!node)
			return B_ENTRY_NOT_FOUND;
		*vnid = node->GetID();
	}
	locker.Unlock();

	// get a VFS node reference
	Node* dummyNode;
	status_t error = GetVNode(*vnid, &dummyNode);
	if (error != B_OK)
		return error;
	return B_OK;
}

// #pragma mark -
// #pragma mark ----- attributes -----

// OpenAttrDir
status_t
VirtualVolume::OpenAttrDir(Node* node, void** cookie)
{
	if (node != fRootNode)
		return B_BAD_VALUE;

	// we support no attributes at this time
	*cookie = NULL;
	return B_OK;
}

// CloseAttrDir
status_t
VirtualVolume::CloseAttrDir(Node* node, void* cookie)
{
	// no-op: FreeAttrDirCookie() does the job
	return B_OK;
}

// FreeAttrDirCookie
status_t
VirtualVolume::FreeAttrDirCookie(Node* node, void* _cookie)
{
	// nothing to do: we didn't allocate anything
	return B_OK;
}

// ReadAttrDir
status_t
VirtualVolume::ReadAttrDir(Node* node, void* _cookie, struct dirent* buffer,
	size_t bufferSize, int32 count, int32* countRead)
{
	// no attributes for the time being
	*countRead = 0;
	return B_OK;
}

// RewindAttrDir
status_t
VirtualVolume::RewindAttrDir(Node* node, void* _cookie)
{
	return B_OK;
}

// ReadAttr
status_t
VirtualVolume::ReadAttr(Node* node, const char* name, int type, off_t pos,
	void* _buffer, size_t bufferSize, size_t* bytesRead)
{
	// no attributes for the time being
	*bytesRead = 0;
	return B_ENTRY_NOT_FOUND;
}

// WriteAttr
status_t
VirtualVolume::WriteAttr(Node* node, const char* name, int type, off_t pos,
	const void* _buffer, size_t bufferSize, size_t* bytesWritten)
{
	// no attributes for the time being
	*bytesWritten = 0;
	return B_NOT_ALLOWED;
}

// RemoveAttr
status_t
VirtualVolume::RemoveAttr(Node* node, const char* name)
{
	return B_NOT_ALLOWED;
}

// RenameAttr
status_t
VirtualVolume::RenameAttr(Node* node, const char* oldName, const char* newName)
{
	// no attributes for the time being
	return B_ENTRY_NOT_FOUND;
}

// StatAttr
status_t
VirtualVolume::StatAttr(Node* node, const char* name, struct attr_info* attrInfo)
{
	// no attributes for the time being
	return B_ENTRY_NOT_FOUND;
}

// #pragma mark -
// #pragma mark ----- queries -----

// OpenQuery
status_t
VirtualVolume::OpenQuery(const char* queryString, uint32 flags, port_id port,
	int32 token, QueryIterator** _iterator)
{
	QueryManager* queryManager = fVolumeManager->GetQueryManager();

	// allocate a hierarchical iterator
	HierarchicalQueryIterator* iterator
		= new(nothrow) HierarchicalQueryIterator(this);
	if (!iterator)
		return B_NO_MEMORY;

	// add it to the query manager
	status_t error = queryManager->AddIterator(iterator);
	if (error != B_OK) {
		delete iterator;
		return error;
	}
	QueryIteratorPutter iteratorPutter(queryManager, iterator);

	// iterate through the child volumes and open subqueries for them
	// init a directory iterator
	fLock.Lock();
	VirtualDirIterator dirIterator;
	dirIterator.SetDirectory(fRootNode, true);

	// iterate through the directory
	const char* name;
	Node* node;
	while (dirIterator.GetCurrentEntry(&name, &node)) {
		dirIterator.NextEntry();
		Volume* volume = fVolumeManager->GetVolume(node->GetID());
		fLock.Unlock();

		// open the subquery
		QueryIterator* subIterator;
		if (volume->OpenQuery(queryString, flags, port, token,
			&subIterator) == B_OK) {
			// add the subiterator
			if (queryManager->AddSubIterator(iterator, subIterator) != B_OK)
				queryManager->PutIterator(subIterator);
		}
		volume->PutVolume();

		fLock.Lock();
	}

	// uninit the directory iterator
	dirIterator.SetDirectory(NULL);
	fLock.Unlock();

	// return the result
	*_iterator = iterator;
	iteratorPutter.Detach();

	return B_OK;
}

// FreeQueryIterator
void
VirtualVolume::FreeQueryIterator(QueryIterator* iterator)
{
	delete iterator;
}

// ReadQuery
status_t
VirtualVolume::ReadQuery(QueryIterator* _iterator, struct dirent* buffer,
	size_t bufferSize, int32 count, int32* countRead)
{
	HierarchicalQueryIterator* iterator
		= dynamic_cast<HierarchicalQueryIterator*>(_iterator);

	QueryManager* queryManager = fVolumeManager->GetQueryManager();
	while (QueryIterator* subIterator
			= queryManager->GetCurrentSubIterator(iterator)) {
		QueryIteratorPutter _(queryManager, subIterator);

		status_t error = subIterator->GetVolume()->ReadQuery(subIterator,
			buffer, bufferSize, count, countRead);
		if (error != B_OK)
			return error;

		if (*countRead > 0)
			return B_OK;

		queryManager->NextSubIterator(iterator, subIterator);
	}

	*countRead = 0;
	return B_OK;
}

