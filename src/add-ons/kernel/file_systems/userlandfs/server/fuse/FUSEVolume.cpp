/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "FUSEVolume.h"

#include <dirent.h>
#include <file_systems/mime_ext_table.h>

#include <algorithm>

#include <fs_info.h>
#include <NodeMonitor.h>

#include <AutoDeleter.h>

#include "FUSEFileSystem.h"

#include "../kernel_emu.h"
#include "../RequestThread.h"


// TODO: For remote/shared file systems (sshfs, nfs, etc.) we need to notice
// that entries have been added/removed, so that we can (1) update our
// FUSEEntry/FUSENode objects and (2) send out node monitoring messages.


// The maximal node tree hierarchy levels we support.
static const uint32 kMaxNodeTreeDepth = 1024;


struct FUSEVolume::DirEntryCache {
	DirEntryCache()
		:
		fEntries(NULL),
		fNames(NULL),
		fEntryCount(0),
		fEntryCapacity(0),
		fNamesSize(0),
		fNamesCapacity(0)
	{
	}

	~DirEntryCache()
	{
		free(fEntries);
		free(fNames);
	}

	status_t AddEntry(ino_t nodeID, const char* name)
	{
		// resize entries array, if full
		if (fEntryCount == fEntryCapacity) {
			// entries array full -- resize
			uint32 newCapacity = std::max(fEntryCapacity * 2, (uint32)8);
			Entry* newEntries = (Entry*)realloc(fEntries,
				newCapacity * sizeof(Entry));
			if (newEntries == NULL)
				return B_NO_MEMORY;

			fEntries = newEntries;
			fEntryCapacity = newCapacity;
		}

		// resize names buffer, if full
		size_t nameSize = strlen(name) + 1;
		if (fNamesSize + nameSize > fNamesCapacity) {
			size_t newCapacity = std::max(fNamesCapacity * 2, (size_t)256);
			while (newCapacity < fNamesSize + nameSize)
				newCapacity *= 2;

			char* names = (char*)realloc(fNames, newCapacity);
			if (names == NULL)
				return B_NO_MEMORY;

			fNames = names;
			fNamesCapacity = newCapacity;
		}

		// add the entry
		fEntries[fEntryCount].nodeID = nodeID;
		fEntries[fEntryCount].nameOffset = fNamesSize;
		fEntries[fEntryCount].nameSize = nameSize;
		fEntryCount++;

		memcpy(fNames + fNamesSize, name, nameSize);
		fNamesSize += nameSize;

		return B_OK;
	}

	uint32 CountEntries() const
	{
		return fEntryCount;
	}

	size_t DirentLength(uint32 index) const
	{
		const Entry& entry = fEntries[index];
		return sizeof(dirent) + entry.nameSize - 1;
	}

	bool ReadDirent(uint32 index, dev_t volumeID, bool align, dirent* buffer,
		size_t bufferSize) const
	{
		if (index >= fEntryCount)
			return false;

		const Entry& entry = fEntries[index];

		// get and check the size
		size_t size = sizeof(dirent) + entry.nameSize - 1;
		if (size > bufferSize)
			return false;

		// align the size, if requested
		if (align)
			size = std::min(bufferSize, (size + 7) / 8 * 8);

		// fill in the dirent
		buffer->d_dev = volumeID;
		buffer->d_ino = entry.nodeID;
		memcpy(buffer->d_name, fNames + entry.nameOffset, entry.nameSize);
		buffer->d_reclen = size;

		return true;
	}

private:
	struct Entry {
		ino_t	nodeID;
		uint32	nameOffset;
		uint32	nameSize;
	};

private:
	Entry*	fEntries;
	char*	fNames;
	uint32	fEntryCount;
	uint32	fEntryCapacity;
	size_t	fNamesSize;
	size_t	fNamesCapacity;
};


struct FUSEVolume::DirCookie : fuse_file_info, RWLockable {
	union {
		off_t		currentEntryOffset;
		uint32		currentEntryIndex;
	};
	DirEntryCache*	entryCache;
	bool			getdirInterface;

	DirCookie()
		:
		currentEntryOffset(0),
		entryCache(NULL),
		getdirInterface(false)
	{
		flags = 0;
		fh_old = 0;
		writepage = 0;
		direct_io = 0;
		keep_cache = 0;
		flush = 0;
		fh = 0;
		lock_owner = 0;
	}

	~DirCookie()
	{
		delete entryCache;
	}
};


struct FUSEVolume::FileCookie : fuse_file_info, RWLockable {
	FileCookie(int openMode)
	{
		flags = openMode;
		fh_old = 0;
		writepage = 0;
		direct_io = 0;
		keep_cache = 0;
		flush = 0;
		fh = 0;
		lock_owner = 0;
	}
};


struct FUSEVolume::AttrDirCookie : RWLockable {
	AttrDirCookie()
		:
		fAttributes(NULL),
		fAttributesSize(0),
		fCurrentOffset(0),
		fValid(false)
	{
	}

	~AttrDirCookie()
	{
		Clear();
	}

	void Clear()
	{
		free(fAttributes);
		fAttributes = NULL;
		fAttributesSize = 0;
		fCurrentOffset = 0;
		fValid = false;
	}

	status_t Allocate(size_t size)
	{
		Clear();

		if (size == 0)
			return B_OK;

		fAttributes = (char*)malloc(size);
		if (fAttributes == NULL)
			return B_NO_MEMORY;

		fAttributesSize = size;
		return B_OK;
	}

	bool IsValid() const
	{
		return fValid;
	}

	void SetValid(bool valid)
	{
		fValid = valid;
	}

	char* AttributesBuffer() const
	{
		return fAttributes;
	}

	bool ReadNextEntry(dev_t volumeID, ino_t nodeID, bool align,
		dirent* buffer, size_t bufferSize)
	{
		if (fCurrentOffset >= fAttributesSize)
			return false;

		const char* name = fAttributes + fCurrentOffset;
		size_t nameLen = strlen(name);

		// get and check the size
		size_t size = sizeof(dirent) + nameLen;
		if (size > bufferSize)
			return false;

		// align the size, if requested
		if (align)
			size = std::min(bufferSize, (size + 7) / 8 * 8);

		// fill in the dirent
		buffer->d_dev = volumeID;
		buffer->d_ino = nodeID;
		memcpy(buffer->d_name, name, nameLen + 1);
		buffer->d_reclen = size;

		fCurrentOffset += nameLen + 1;

		return true;
	}

private:
	char*	fAttributes;
	size_t	fAttributesSize;
	size_t	fCurrentOffset;
	bool	fValid;
};


struct FUSEVolume::AttrCookie : RWLockable {
public:
	AttrCookie(const char* name)
		:
		fValue(NULL),
		fSize(0),
		fType(0)
	{
		_SetType(name);
	}

	AttrCookie(const char* name, const char* value)
		:
		fValue(strdup(value)),
		fSize(strlen(value) + 1),
		fType(0)
	{
		_SetType(name);
	}

	~AttrCookie()
	{
		free(fValue);
	}

	bool IsValid() const
	{
		return fValue != NULL;
	}

	uint32 Type() const
	{
		return fType;
	}

	status_t Allocate(size_t size)
	{
		fValue = (char*)malloc(size);
		if (fValue == NULL) {
			fSize = 0;
			return B_NO_MEMORY;
		}
		fSize = size;
		return B_OK;
	}

	void Read(void* buffer, size_t bufferSize, off_t pos,
		size_t* bytesRead) const
	{
		if (pos < 0 || (uint64)pos > SIZE_MAX || (size_t)pos > fSize - 1) {
			*bytesRead = 0;
			return;
		}
		size_t copySize = fSize - pos;
		if (copySize > bufferSize)
			copySize = bufferSize;
		strlcpy((char*)buffer, &fValue[pos], bufferSize);
		*bytesRead = copySize;
	}

	char* Buffer()
	{
		return fValue;
	}

	size_t Size() const
	{
		return fSize;
	}

private:
	void _SetType(const char* name)
	{
		if (strcmp(name, kAttrMimeTypeName) == 0)
			fType = B_MIME_STRING_TYPE;
		else
			fType = B_RAW_TYPE;
	}

private:
	char*	fValue;
	size_t	fSize;
	uint32	fType;
};


struct FUSEVolume::ReadDirBuffer {
	FUSEVolume*	volume;
	FUSENode*	directory;
	DirCookie*	cookie;
	void*		buffer;
	size_t		bufferSize;
	size_t		usedSize;
	uint32		entriesRead;
	uint32		maxEntries;
	status_t	error;

	ReadDirBuffer(FUSEVolume* volume, FUSENode* directory, DirCookie* cookie,
		void* buffer, size_t bufferSize, uint32 maxEntries)
		:
		volume(volume),
		directory(directory),
		cookie(cookie),
		buffer(buffer),
		bufferSize(bufferSize),
		usedSize(0),
		entriesRead(0),
		maxEntries(maxEntries),
		error(B_OK)
	{
	}
};


struct FUSEVolume::LockIterator {
	FUSEVolume*	volume;
	FUSENode*	firstNode;
	FUSENode*	lastLockedNode;
	FUSENode*	nextNode;
	FUSENode*	stopBeforeNode;
	bool		writeLock;

	LockIterator(FUSEVolume* volume, FUSENode* node, bool writeLock,
		FUSENode* stopBeforeNode)
		:
		volume(volume),
		firstNode(node),
		lastLockedNode(NULL),
		nextNode(node),
		stopBeforeNode(stopBeforeNode),
		writeLock(writeLock)
	{
	}

	~LockIterator()
	{
		Unlock();
	}

	void SetTo(FUSEVolume* volume, FUSENode* node, bool writeLock,
		FUSENode* stopBeforeNode)
	{
		Unlock();

		this->volume = volume;
		this->firstNode = node;
		this->lastLockedNode = NULL;
		this->nextNode = node;
		this->stopBeforeNode = stopBeforeNode;
		this->writeLock = writeLock;
	}

	status_t LockNext(bool* _done, bool* _volumeUnlocked)
	{
		// increment the ref count first
		nextNode->refCount++;

		if (volume->fLockManager.TryGenericLock(
				nextNode == firstNode && writeLock, nextNode)) {
			// got the lock
			*_volumeUnlocked = false;
		} else {
			// node is locked -- we need to unlock the volume and wait for
			// the lock
			volume->fLock.Unlock();
			status_t error = volume->fLockManager.GenericLock(
				nextNode == firstNode && writeLock, nextNode);
			volume->fLock.Lock();

			*_volumeUnlocked = false;

			if (error != B_OK) {
				volume->_PutNode(nextNode);
				return error;
			}
		}

		lastLockedNode = nextNode;

		// get the parent node
		FUSENode* parent = nextNode->Parent();
		if (parent == stopBeforeNode || parent == nextNode) {
			if (parent == nextNode)
				parent = NULL;
			*_done = true;
		} else
			*_done = false;

		nextNode = parent;

		return B_OK;
	}

	void Unlock()
	{
		if (lastLockedNode == NULL)
			return;

		volume->_UnlockNodeChainInternal(firstNode, writeLock, lastLockedNode,
			NULL);

		lastLockedNode = NULL;
		nextNode = firstNode;
	}

	void SetStopBeforeNode(FUSENode* stopBeforeNode)
	{
		this->stopBeforeNode = stopBeforeNode;
	}

	void Detach()
	{
		lastLockedNode = NULL;
		nextNode = firstNode;
	}
};


struct FUSEVolume::RWLockableReadLocking {
	RWLockableReadLocking(FUSEVolume* volume)
		:
		fLockManager(volume != NULL ? &volume->fLockManager : NULL)
	{
	}

	RWLockableReadLocking(RWLockManager* lockManager)
		:
		fLockManager(lockManager)
	{
	}

	RWLockableReadLocking(const RWLockableReadLocking& other)
		:
		fLockManager(other.fLockManager)
	{
	}

	inline bool Lock(RWLockable* lockable)
	{
		return fLockManager != NULL && fLockManager->ReadLock(lockable);
	}

	inline void Unlock(RWLockable* lockable)
	{
		if (fLockManager != NULL)
			fLockManager->ReadUnlock(lockable);
	}

private:
	RWLockManager*	fLockManager;
};


struct FUSEVolume::RWLockableWriteLocking {
	RWLockableWriteLocking(FUSEVolume* volume)
		:
		fLockManager(volume != NULL ? &volume->fLockManager : NULL)
	{
	}

	RWLockableWriteLocking(RWLockManager* lockManager)
		:
		fLockManager(lockManager)
	{
	}

	RWLockableWriteLocking(const RWLockableWriteLocking& other)
		:
		fLockManager(other.fLockManager)
	{
	}

	inline bool Lock(RWLockable* lockable)
	{
		return fLockManager != NULL && fLockManager->WriteLock(lockable);
	}

	inline void Unlock(RWLockable* lockable)
	{
		if (fLockManager != NULL)
			fLockManager->WriteUnlock(lockable);
	}

private:
	RWLockManager*	fLockManager;
};


struct FUSEVolume::RWLockableReadLocker
	: public AutoLocker<RWLockable, RWLockableReadLocking> {

	RWLockableReadLocker(FUSEVolume* volume, RWLockable* lockable)
		:
		AutoLocker<RWLockable, RWLockableReadLocking>(
			RWLockableReadLocking(volume))
	{
		SetTo(lockable, false);
	}
};


struct FUSEVolume::RWLockableWriteLocker
	: public AutoLocker<RWLockable, RWLockableWriteLocking> {

	RWLockableWriteLocker(FUSEVolume* volume, RWLockable* lockable)
		:
		AutoLocker<RWLockable, RWLockableWriteLocking>(
			RWLockableWriteLocking(volume))
	{
		SetTo(lockable, false);
	}
};


struct FUSEVolume::NodeLocker {
	NodeLocker(FUSEVolume* volume, FUSENode* node, bool parent, bool writeLock)
		:
		fVolume(volume),
		fNode(NULL),
		fParent(parent),
		fWriteLock(writeLock)
	{
		fStatus = volume->_LockNodeChain(node, parent, writeLock);
		if (fStatus == B_OK)
			fNode = node;
	}

	~NodeLocker()
	{
		if (fNode != NULL)
			fVolume->_UnlockNodeChain(fNode, fParent, fWriteLock);
	}

	status_t Status() const
	{
		return fStatus;
	}

private:
	FUSEVolume*	fVolume;
	FUSENode*	fNode;
	status_t	fStatus;
	bool		fParent;
	bool		fWriteLock;
};


struct FUSEVolume::NodeReadLocker : NodeLocker {
	NodeReadLocker(FUSEVolume* volume, FUSENode* node, bool parent)
		:
		NodeLocker(volume, node, parent, false)
	{
	}
};


struct FUSEVolume::NodeWriteLocker : NodeLocker {
	NodeWriteLocker(FUSEVolume* volume, FUSENode* node, bool parent)
		:
		NodeLocker(volume, node, parent, true)
	{
	}
};


struct FUSEVolume::MultiNodeLocker {
	MultiNodeLocker(FUSEVolume* volume, FUSENode* node1, bool lockParent1,
		bool writeLock1, FUSENode* node2, bool lockParent2, bool writeLock2)
		:
		fVolume(volume),
		fNode1(NULL),
		fNode2(NULL),
		fLockParent1(lockParent1),
		fWriteLock1(writeLock1),
		fLockParent2(lockParent2),
		fWriteLock2(writeLock2)
	{
		fStatus = volume->_LockNodeChains(node1, lockParent1, writeLock1, node2,
			lockParent2, writeLock2);
		if (fStatus == B_OK) {
			fNode1 = node1;
			fNode2 = node2;
		}
	}

	~MultiNodeLocker()
	{
		if (fNode1 != NULL) {
			fVolume->_UnlockNodeChains(fNode1, fLockParent1, fWriteLock1,
				fNode2, fLockParent2, fWriteLock2);
		}
	}

	status_t Status() const
	{
		return fStatus;
	}

private:
	FUSEVolume*	fVolume;
	FUSENode*	fNode1;
	FUSENode*	fNode2;
	bool		fLockParent1;
	bool		fWriteLock1;
	bool		fLockParent2;
	bool		fWriteLock2;
	status_t	fStatus;
};


// #pragma mark -


inline FUSEFileSystem*
FUSEVolume::_FileSystem() const
{
	return static_cast<FUSEFileSystem*>(fFileSystem);
}


FUSEVolume::FUSEVolume(FUSEFileSystem* fileSystem, dev_t id)
	:
	Volume(fileSystem, id),
	fFS(NULL),
	fRootNode(NULL),
	fNextNodeID(FUSE_ROOT_ID + 1),
	fUseNodeIDs(false)
{
}


FUSEVolume::~FUSEVolume()
{
}


status_t
FUSEVolume::Init()
{
	// init lock manager
	status_t error = fLockManager.Init();
	if (error != B_OK)
		return error;

	// init entry and node tables
	error = fEntries.Init();
	if (error != B_OK)
		return error;

	error = fNodes.Init();
	if (error != B_OK)
		return error;

	// check lock
	error = fLock.InitCheck();
	if (error != B_OK)
		return error;

	return B_OK;
}


// #pragma mark - FS


status_t
FUSEVolume::Mount(const char* device, uint32 flags, const char* parameters,
	ino_t* rootID)
{
printf("FUSEVolume::Mount()\n");
	status_t error = _FileSystem()->InitClientFS(parameters);
	if (error != B_OK)
		RETURN_ERROR(error);

	fFS = _FileSystem()->GetFS();
	_FileSystem()->GetVolumeCapabilities(fCapabilities);

	const fuse_config& config = _FileSystem()->GetFUSEConfig();
	fUseNodeIDs = config.use_ino;

	// update the fuse_context::private_data field before calling into the FS
	fuse_context* context = (fuse_context*)RequestThread::GetCurrentThread()
		->GetContext()->GetFSData();
	context->private_data = fFS->userData;

	// get the root node
	struct stat st;
	int fuseError = fuse_fs_getattr(fFS, "/", &st);
	if (fuseError != 0)
		RETURN_ERROR(fuseError);

	if (!fUseNodeIDs)
		st.st_ino = FUSE_ROOT_ID;

	// create a node and an entry object for the root node
	AutoLocker<Locker> _(fLock);
	FUSENode* node = new(std::nothrow) FUSENode(st.st_ino, st.st_mode & S_IFMT);
	FUSEEntry* entry = node != NULL ? FUSEEntry::Create(node, "/", node) : NULL;
	if (node == NULL || entry == NULL) {
		delete node;
		delete entry;
		_FileSystem()->ExitClientFS(B_NO_MEMORY);
		RETURN_ERROR(B_NO_MEMORY);
	}

	node->refCount++;	// for the entry
	node->entries.Add(entry);
	fRootNode = node;

	// insert the node and the entry
	fNodes.Insert(node);
	fEntries.Insert(entry);

	// init the volume name
	snprintf(fName, sizeof(fName), "%s Volume", _FileSystem()->GetName());

	// publish the root node
	error = UserlandFS::KernelEmu::publish_vnode(fID, node->id, node,
		node->type, 0, _FileSystem()->GetNodeCapabilities());
	if (error != B_OK) {
		_FileSystem()->ExitClientFS(B_NO_MEMORY);
		RETURN_ERROR(error);
	}

	*rootID = node->id;

	return B_OK;
}


status_t
FUSEVolume::Unmount()
{
printf("FUSEVolume::Unmount()\n");
	_FileSystem()->ExitClientFS(B_OK);
	return B_OK;
}


status_t
FUSEVolume::Sync()
{
	PRINT(("FUSEVolume::Sync()\n"));

	// There's no FUSE hook for sync'ing the whole FS. We need to individually
	// fsync all nodes that have been marked dirty. To keep things simple, we
	// hold the volume lock the whole time. That's a concurrency killer, but
	// usually sync isn't invoked that often.

	AutoLocker<Locker> _(fLock);

	// iterate through all nodes
	FUSENodeTable::Iterator it = fNodes.GetIterator();
	while (FUSENode* node = it.Next()) {
		if (!node->dirty)
			continue;

		// node is dirty -- we have to sync it

		// get a path for the node
		char path[B_PATH_NAME_LENGTH];
		size_t pathLen;
		status_t error = _BuildPath(node, path, pathLen);
		if (error != B_OK)
			continue;

		// open, sync, and close the node
		FileCookie cookie(O_RDONLY);
		int fuseError = fuse_fs_open(fFS, path, &cookie);
		if (fuseError == 0) {
			fuseError = fuse_fs_fsync(fFS, path, 0, &cookie);
				// full sync, not only data
			fuse_fs_flush(fFS, path, &cookie);
			fuse_fs_release(fFS, path, &cookie);
		}

		if (fuseError == 0) {
			// sync'ing successful -- mark the node not dirty
			node->dirty = false;
		}
	}

	return B_OK;
}


status_t
FUSEVolume::ReadFSInfo(fs_info* info)
{
	if (gHasHaikuFuseExtensions == 1 && fFS->ops.get_fs_info != NULL) {
		int fuseError = fuse_fs_get_fs_info(fFS, info);
		if (fuseError != 0)
			return fuseError;
		return B_OK;
	}

	// No Haiku FUSE extensions, so our knowledge is limited: use some values
	// from statfs and make reasonable guesses for the rest of them.
	if (fFS->ops.statfs == NULL)
		return B_UNSUPPORTED;

	struct statvfs st;
	int fuseError = fuse_fs_statfs(fFS, "/", &st);
	if (fuseError != 0)
		return fuseError;

	memset(info, 0, sizeof(*info));
	info->flags = B_FS_IS_PERSISTENT;	// assume the FS is persistent
	info->block_size = st.f_bsize;
	info->io_size = 64 * 1024;			// some value
	info->total_blocks = st.f_blocks;
	info->free_blocks = st.f_bfree;
	info->total_nodes = st.f_files;
	info->free_nodes = 100;				// st.f_favail is ignored by statfs()
	strlcpy(info->volume_name, fName, sizeof(info->volume_name));
		// no way to get the real name (if any)

	return B_OK;
}


// #pragma mark - vnodes


status_t
FUSEVolume::Lookup(void* _dir, const char* entryName, ino_t* vnid)
{
	FUSENode* dir = (FUSENode*)_dir;

	// lock the directory
	NodeReadLocker nodeLocker(this, dir, false);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	// look the node up
	FUSENode* node;
	status_t error = _GetNode(dir, entryName, &node);
	if (error != B_OK)
		return error;

	*vnid = node->id;

	return B_OK;
}


status_t
FUSEVolume::GetVNodeName(void* _node, char* buffer, size_t bufferSize)
{
	FUSENode* node = (FUSENode*)_node;

	AutoLocker<Locker> _(fLock);

	// get one of the node's entries and return its name
	FUSEEntry* entry = node->entries.Head();
	if (entry == NULL)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);

	if (strlcpy(buffer, entry->name, bufferSize) >= bufferSize)
		RETURN_ERROR(B_NAME_TOO_LONG);

	return B_OK;
}


status_t
FUSEVolume::ReadVNode(ino_t vnid, bool reenter, void** _node, int* type,
	uint32* flags, FSVNodeCapabilities* _capabilities)
{
	AutoLocker<Locker> _(fLock);

	FUSENode* node = fNodes.Lookup(vnid);
	if (node == NULL)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);

	node->refCount++;

	*_node = node;
	*type = node->type;
	*flags = 0;
	*_capabilities = _FileSystem()->GetNodeCapabilities();

	return B_OK;
}


status_t
FUSEVolume::WriteVNode(void* _node, bool reenter)
{
	FUSENode* node = (FUSENode*)_node;

	AutoLocker<Locker> _(fLock);

	_PutNode(node);

	return B_OK;
}


status_t
FUSEVolume::RemoveVNode(void* node, bool reenter)
{
	// TODO: Implement for real!
	return WriteVNode(node, reenter);
}


// #pragma mark - nodes


status_t
FUSEVolume::SetFlags(void* _node, void* _cookie, int flags)
{
	FileCookie* cookie = (FileCookie*)_cookie;

	RWLockableWriteLocker cookieLocker(this, cookie);

	const int settableFlags = O_APPEND | O_NONBLOCK | O_SYNC | O_RSYNC
		| O_DSYNC | O_DIRECT;

	cookie->flags = (cookie->flags & ~settableFlags) | (flags & settableFlags);

	return B_OK;
}


status_t
FUSEVolume::FSync(void* _node)
{
	FUSENode* node = (FUSENode*)_node;

	// lock the directory
	NodeReadLocker nodeLocker(this, node, true);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	AutoLocker<Locker> locker(fLock);

	// get a path for the node
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen;
	status_t error = _BuildPath(node, path, pathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	// mark the node not dirty
	bool dirty = node->dirty;
	node->dirty = false;

	locker.Unlock();

	// open, sync, and close the node
	FileCookie cookie(O_RDONLY);
	int fuseError = fuse_fs_open(fFS, path, &cookie);
	if (fuseError == 0) {
		fuseError = fuse_fs_fsync(fFS, path, 0, &cookie);
			// full sync, not only data
		fuse_fs_flush(fFS, path, &cookie);
		fuse_fs_release(fFS, path, &cookie);
	}

	if (fuseError != 0) {
		// sync'ing failed -- mark the node dirty again
		locker.Lock();
		node->dirty |= dirty;
		RETURN_ERROR(fuseError);
	}

	return B_OK;
}


status_t
FUSEVolume::ReadSymlink(void* _node, char* buffer, size_t bufferSize,
	size_t* _bytesRead)
{
	FUSENode* node = (FUSENode*)_node;

	// lock the directory
	NodeReadLocker nodeLocker(this, node, true);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	AutoLocker<Locker> locker(fLock);

	// get a path for the node
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen;
	status_t error = _BuildPath(node, path, pathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	locker.Unlock();

	// read the symlink
	int fuseError = fuse_fs_readlink(fFS, path, buffer, bufferSize);
	if (fuseError != 0) {
		*_bytesRead = 0;
		return fuseError;
	}

	// fuse_fs_readlink() is supposed to return a NULL-terminated string, which
	// the Haiku interface doesn't require. We have to return the string length,
	// though.
	*_bytesRead = strnlen(buffer, bufferSize);

	return B_OK;
}


status_t
FUSEVolume::CreateSymlink(void* _dir, const char* name, const char* target,
	int mode)
{
	FUSENode* dir = (FUSENode*)_dir;
	PRINT(("FUSEVolume::CreateSymlink(%p (%" B_PRId64 "), \"%s\" -> \"%s\", "
		"%#x)\n", dir, dir->id, name, target, mode));

	// lock the directory
	NodeWriteLocker nodeLocker(this, dir, false);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	AutoLocker<Locker> locker(fLock);

	// get a path for the entry
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen;
	status_t error = _BuildPath(dir, name, path, pathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	locker.Unlock();

	// create the symlink
	int fuseError = fuse_fs_symlink(fFS, target, path);
	if (fuseError != 0)
		RETURN_ERROR(fuseError);

	// TODO: Set the mode?!

	// mark the dir dirty
	locker.Lock();
	dir->dirty = true;
	locker.Unlock();

	// send node monitoring message
	ino_t nodeID;
	if (_GetNodeID(dir, name, &nodeID)) {
		UserlandFS::KernelEmu::notify_listener(B_ENTRY_CREATED, 0, fID, 0,
			dir->id, nodeID, NULL, name);
	}

	return B_OK;
}


status_t
FUSEVolume::Link(void* _dir, const char* name, void* _node)
{
	FUSENode* dir = (FUSENode*)_dir;
	FUSENode* node = (FUSENode*)_node;
	PRINT(("FUSEVolume::Link(%p (%" B_PRId64 "), \"%s\" -> %p (%" B_PRId64
		"))\n", dir, dir->id, name, node, node->id));

	// lock the directories -- the target directory for writing, the node's
	// parent for reading
	MultiNodeLocker nodeLocker(this, dir, false, true, node, true, false);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	AutoLocker<Locker> locker(fLock);

	// get a path for the entries
	char oldPath[B_PATH_NAME_LENGTH];
	size_t oldPathLen;
	status_t error = _BuildPath(node, oldPath, oldPathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	char newPath[B_PATH_NAME_LENGTH];
	size_t newPathLen;
	error = _BuildPath(dir, name, newPath, newPathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	locker.Unlock();

	// link
	int fuseError = fuse_fs_link(fFS, oldPath, newPath);
	if (fuseError != 0)
		RETURN_ERROR(fuseError);

	// mark the dir and the node dirty
	locker.Lock();
	dir->dirty = true;
	node->dirty = true;
	locker.Unlock();

	// send node monitoring message
	UserlandFS::KernelEmu::notify_listener(B_ENTRY_CREATED, 0, fID, 0, dir->id,
		node->id, NULL, name);

	return B_OK;
}


status_t
FUSEVolume::Unlink(void* _dir, const char* name)
{
	FUSENode* dir = (FUSENode*)_dir;
	PRINT(("FUSEVolume::Unlink(%p (%" B_PRId64 "), \"%s\")\n", dir, dir->id,
		name));

	// lock the directory
	NodeWriteLocker nodeLocker(this, dir, false);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	// get the node ID (for the node monitoring message)
	ino_t nodeID;
	bool doNodeMonitoring = _GetNodeID(dir, name, &nodeID);

	AutoLocker<Locker> locker(fLock);

	// get a path for the entry
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen;
	status_t error = _BuildPath(dir, name, path, pathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	locker.Unlock();

	// unlink
	int fuseError = fuse_fs_unlink(fFS, path);
	if (fuseError != 0)
		RETURN_ERROR(fuseError);

	// remove the entry
	locker.Lock();
	_RemoveEntry(dir, name);

	// mark the dir dirty
	dir->dirty = true;
	locker.Unlock();

	// send node monitoring message
	if (doNodeMonitoring) {
		UserlandFS::KernelEmu::notify_listener(B_ENTRY_REMOVED, 0, fID, 0,
			dir->id, nodeID, NULL, name);
	}

	return B_OK;
}


status_t
FUSEVolume::Rename(void* _oldDir, const char* oldName, void* _newDir,
	const char* newName)
{
	FUSENode* oldDir = (FUSENode*)_oldDir;
	FUSENode* newDir = (FUSENode*)_newDir;
	PRINT(("FUSEVolume::Rename(%p (%" B_PRId64 "), \"%s\", %p (%" B_PRId64
		"), \"%s\")\n", oldDir, oldDir->id, oldName, newDir, newDir->id,
		newName));

	// lock the directories
	MultiNodeLocker nodeLocker(this, oldDir, false, true, newDir, false, true);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	AutoLocker<Locker> locker(fLock);

	// get a path for the entries
	char oldPath[B_PATH_NAME_LENGTH];
	size_t oldPathLen;
	status_t error = _BuildPath(oldDir, oldName, oldPath, oldPathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	char newPath[B_PATH_NAME_LENGTH];
	size_t newPathLen;
	error = _BuildPath(newDir, newName, newPath, newPathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	locker.Unlock();

	// rename
	int fuseError = fuse_fs_rename(fFS, oldPath, newPath);
	if (fuseError != 0)
		RETURN_ERROR(fuseError);

	// rename the entry
	locker.Lock();
	_RenameEntry(oldDir, oldName, newDir, newName);

	// mark the dirs dirty
	oldDir->dirty = true;
	newDir->dirty = true;

	// send node monitoring message
	ino_t nodeID;
	if (_GetNodeID(newDir, newName, &nodeID)) {
		UserlandFS::KernelEmu::notify_listener(B_ENTRY_MOVED, 0, fID,
			oldDir->id, newDir->id, nodeID, oldName, newName);
	}

	return B_OK;
}


status_t
FUSEVolume::Access(void* _node, int mode)
{
	FUSENode* node = (FUSENode*)_node;

	// lock the directory
	NodeReadLocker nodeLocker(this, node, true);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	AutoLocker<Locker> locker(fLock);

	// get a path for the node
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen;
	status_t error = _BuildPath(node, path, pathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	locker.Unlock();

	// call the access hook on the path
	int fuseError = fuse_fs_access(fFS, path, mode);
	if (fuseError != 0)
		return fuseError;

	return B_OK;
}


status_t
FUSEVolume::ReadStat(void* _node, struct stat* st)
{
	FUSENode* node = (FUSENode*)_node;
	PRINT(("FUSEVolume::ReadStat(%p (%" B_PRId64 "), %p)\n", node, node->id,
		st));

	// lock the directory
	NodeReadLocker nodeLocker(this, node, true);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	AutoLocker<Locker> locker(fLock);

	// get a path for the node
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen;
	status_t error = _BuildPath(node, path, pathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	locker.Unlock();

	st->st_dev = GetID();
	st->st_ino = node->id;
	st->st_blksize = 2048;
	st->st_type = 0;

	// stat the path
	int fuseError = fuse_fs_getattr(fFS, path, st);
	if (fuseError != 0)
		return fuseError;

	return B_OK;
}


status_t
FUSEVolume::WriteStat(void* _node, const struct stat* st, uint32 mask)
{
	FUSENode* node = (FUSENode*)_node;
	PRINT(("FUSEVolume::WriteStat(%p (%" B_PRId64 "), %p, %#" B_PRIx32 ")\n",
		node, node->id, st, mask));

	// lock the directory
	NodeReadLocker nodeLocker(this, node, true);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	AutoLocker<Locker> locker(fLock);

	// get a path for the node
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen;
	status_t error = _BuildPath(node, path, pathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	locker.Unlock();

	// permissions
	if ((mask & B_STAT_MODE) != 0) {
		int fuseError = fuse_fs_chmod(fFS, path, st->st_mode);
		if (fuseError != 0)
			RETURN_ERROR(fuseError);
	}

	// owner
	if ((mask & (B_STAT_UID | B_STAT_GID)) != 0) {
		uid_t uid = (mask & B_STAT_UID) != 0 ? st->st_uid : (uid_t)-1;
		gid_t gid = (mask & B_STAT_GID) != 0 ? st->st_gid : (gid_t)-1;
		int fuseError = fuse_fs_chown(fFS, path, uid, gid);
		if (fuseError != 0)
			RETURN_ERROR(fuseError);
	}

	// size
	if ((mask & B_STAT_SIZE) != 0) {
		// truncate
		int fuseError = fuse_fs_truncate(fFS, path, st->st_size);
		if (fuseError != 0)
			RETURN_ERROR(fuseError);
	}

	// access/modification time
	if ((mask & (B_STAT_ACCESS_TIME | B_STAT_MODIFICATION_TIME)) != 0) {
		timespec tv[2] = {
			{st->st_atime, 0},
			{st->st_mtime, 0}
		};

		// If either time is not specified, we need to stat the file to get the
		// current value.
		if ((mask & (B_STAT_ACCESS_TIME | B_STAT_MODIFICATION_TIME))
				!= (B_STAT_ACCESS_TIME | B_STAT_MODIFICATION_TIME)) {
			struct stat currentStat;
			int fuseError = fuse_fs_getattr(fFS, path, &currentStat);
			if (fuseError != 0)
				RETURN_ERROR(fuseError);

			if ((mask & B_STAT_ACCESS_TIME) == 0)
				tv[0].tv_sec = currentStat.st_atime;
			else
				tv[1].tv_sec = currentStat.st_mtime;
		}

		int fuseError = fuse_fs_utimens(fFS, path, tv);
		if (fuseError != 0)
			RETURN_ERROR(fuseError);
	}

	// mark the node dirty
	locker.Lock();
	node->dirty = true;

	// send node monitoring message
	uint32 changedFields = mask &
		(B_STAT_MODE | B_STAT_UID | B_STAT_GID | B_STAT_SIZE
		| B_STAT_ACCESS_TIME | B_STAT_MODIFICATION_TIME);

	if (changedFields != 0) {
		UserlandFS::KernelEmu::notify_listener(B_STAT_CHANGED, changedFields,
			fID, 0, 0, node->id, NULL, NULL);
	}

	return B_OK;
}


// #pragma mark - files


status_t
FUSEVolume::Create(void* _dir, const char* name, int openMode, int mode,
	void** _cookie, ino_t* _vnid)
{
	FUSENode* dir = (FUSENode*)_dir;
	PRINT(("FUSEVolume::Create(%p (%" B_PRId64 "), \"%s\", %#x, %#x)\n", dir,
		dir->id, name, openMode, mode));

	// lock the directory
	NodeWriteLocker nodeLocker(this, dir, false);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	// allocate a file cookie
	FileCookie* cookie = new(std::nothrow) FileCookie(openMode);
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<FileCookie> cookieDeleter(cookie);

	AutoLocker<Locker> locker(fLock);

	// get a path for the node
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen;
	status_t error = _BuildPath(dir, name, path, pathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	locker.Unlock();

	// create the file
	int fuseError = fuse_fs_create(fFS, path, mode, cookie);
	if (fuseError != 0)
		RETURN_ERROR(fuseError);

	// get the node
	FUSENode* node;
	error = _GetNode(dir, name, &node);
	if (error != B_OK) {
		// This is bad. We've create the file successfully, but couldn't get
		// the node. Close the file and delete the entry.
		fuse_fs_flush(fFS, path, cookie);
		fuse_fs_release(fFS, path, cookie);
		fuse_fs_unlink(fFS, path);
		RETURN_ERROR(error);
	}

	// mark the dir and the node dirty
	locker.Lock();
	dir->dirty = true;
	node->dirty = true;
	locker.Unlock();

	// send node monitoring message
	UserlandFS::KernelEmu::notify_listener(B_ENTRY_CREATED, 0, fID, 0, dir->id,
		node->id, NULL, name);

	cookieDeleter.Detach();
	*_cookie = cookie;
	*_vnid = node->id;

	return B_OK;
}


status_t
FUSEVolume::Open(void* _node, int openMode, void** _cookie)
{
	FUSENode* node = (FUSENode*)_node;
	PRINT(("FUSEVolume::Open(%p (%" B_PRId64 "), %#x)\n", node, node->id,
		openMode));

	// lock the directory
	NodeReadLocker nodeLocker(this, node, true);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	bool truncate = (openMode & O_TRUNC) != 0;
	openMode &= ~O_TRUNC;

	// allocate a file cookie
	FileCookie* cookie = new(std::nothrow) FileCookie(openMode);
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<FileCookie> cookieDeleter(cookie);

	AutoLocker<Locker> locker(fLock);

	// get a path for the node
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen;
	status_t error = _BuildPath(node, path, pathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	locker.Unlock();

	// open the file
	int fuseError = fuse_fs_open(fFS, path, cookie);
	if (fuseError != 0)
		RETURN_ERROR(fuseError);

	// truncate the file, if requested
	if (truncate) {
		fuseError = fuse_fs_ftruncate(fFS, path, 0, cookie);
		if (fuseError == ENOSYS) {
			// Fallback to truncate if ftruncate is not implemented
			fuseError = fuse_fs_truncate(fFS, path, 0);
		}
		if (fuseError != 0) {
			fuse_fs_flush(fFS, path, cookie);
			fuse_fs_release(fFS, path, cookie);
			RETURN_ERROR(fuseError);
		}

		// mark the node dirty
		locker.Lock();
		node->dirty = true;

		// send node monitoring message
		UserlandFS::KernelEmu::notify_listener(B_STAT_CHANGED,
			B_STAT_SIZE | B_STAT_MODIFICATION_TIME, fID, 0, 0, node->id, NULL,
			NULL);
	}

	cookieDeleter.Detach();
	*_cookie = cookie;

	return B_OK;
}


status_t
FUSEVolume::Close(void* _node, void* _cookie)
{
	FUSENode* node = (FUSENode*)_node;
	FileCookie* cookie = (FileCookie*)_cookie;

	RWLockableReadLocker cookieLocker(this, cookie);

	// lock the directory
	NodeReadLocker nodeLocker(this, node, true);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	AutoLocker<Locker> locker(fLock);

	// get a path for the node
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen;
	status_t error = _BuildPath(node, path, pathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	locker.Unlock();

	// flush the file
	int fuseError = fuse_fs_flush(fFS, path, cookie);
	if (fuseError != 0)
		return fuseError;

	return B_OK;
}


status_t
FUSEVolume::FreeCookie(void* _node, void* _cookie)
{
	FUSENode* node = (FUSENode*)_node;
	FileCookie* cookie = (FileCookie*)_cookie;

	// no need to lock the cookie here, as no-one else uses it anymore

	// lock the directory
	NodeReadLocker nodeLocker(this, node, true);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	ObjectDeleter<FileCookie> cookieDeleter(cookie);

	AutoLocker<Locker> locker(fLock);

	// get a path for the node
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen;
	status_t error = _BuildPath(node, path, pathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	locker.Unlock();

	// release the file
	int fuseError = fuse_fs_release(fFS, path, cookie);
	if (fuseError != 0)
		return fuseError;

	return B_OK;
}


status_t
FUSEVolume::Read(void* _node, void* _cookie, off_t pos, void* buffer,
	size_t bufferSize, size_t* _bytesRead)
{
	FUSENode* node = (FUSENode*)_node;
	FileCookie* cookie = (FileCookie*)_cookie;

	RWLockableReadLocker cookieLocker(this, cookie);

	*_bytesRead = 0;

	// lock the directory
	NodeReadLocker nodeLocker(this, node, true);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	AutoLocker<Locker> locker(fLock);

	// get a path for the node
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen;
	status_t error = _BuildPath(node, path, pathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	locker.Unlock();

	// read the file
	int bytesRead = fuse_fs_read(fFS, path, (char*)buffer, bufferSize, pos,
		cookie);
	if (bytesRead < 0)
		return bytesRead;

	*_bytesRead = bytesRead;
	return B_OK;
}


status_t
FUSEVolume::Write(void* _node, void* _cookie, off_t pos, const void* buffer,
	size_t bufferSize, size_t* _bytesWritten)
{
	FUSENode* node = (FUSENode*)_node;
	FileCookie* cookie = (FileCookie*)_cookie;

	RWLockableReadLocker cookieLocker(this, cookie);

	*_bytesWritten = 0;

	// lock the directory
	NodeReadLocker nodeLocker(this, node, true);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	AutoLocker<Locker> locker(fLock);

	// get a path for the node
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen;
	status_t error = _BuildPath(node, path, pathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	locker.Unlock();

	// write the file
	int bytesWritten = fuse_fs_write(fFS, path, (const char*)buffer, bufferSize,
		pos, cookie);
	if (bytesWritten < 0)
		return bytesWritten;

	// mark the node dirty
	locker.Lock();
	node->dirty = true;

	// send node monitoring message
	UserlandFS::KernelEmu::notify_listener(B_STAT_CHANGED,
		B_STAT_SIZE | B_STAT_MODIFICATION_TIME, fID, 0, 0, node->id, NULL,
		NULL);
		// TODO: The size possibly doesn't change.
		// TODO: Avoid message flooding -- use a timeout and set the
		// B_STAT_INTERIM_UPDATE flag.

	*_bytesWritten = bytesWritten;
	return B_OK;
}


// #pragma mark - directories


status_t
FUSEVolume::CreateDir(void* _dir, const char* name, int mode)
{
	FUSENode* dir = (FUSENode*)_dir;
	PRINT(("FUSEVolume::CreateDir(%p (%" B_PRId64 "), \"%s\", %#x)\n", dir,
		dir->id, name, mode));

	// lock the directory
	NodeWriteLocker nodeLocker(this, dir, false);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	AutoLocker<Locker> locker(fLock);

	// get a path for the entry
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen;
	status_t error = _BuildPath(dir, name, path, pathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	locker.Unlock();

	// create the dir
	int fuseError = fuse_fs_mkdir(fFS, path, mode);
	if (fuseError != 0)
		RETURN_ERROR(fuseError);

	// mark the dir dirty
	locker.Lock();
	dir->dirty = true;

	// send node monitoring message
	ino_t nodeID;
	if (_GetNodeID(dir, name, &nodeID)) {
		UserlandFS::KernelEmu::notify_listener(B_ENTRY_CREATED, 0, fID, 0,
			dir->id, nodeID, NULL, name);
	}

	return B_OK;
}


status_t
FUSEVolume::RemoveDir(void* _dir, const char* name)
{
	FUSENode* dir = (FUSENode*)_dir;
	PRINT(("FUSEVolume::RemoveDir(%p (%" B_PRId64 "), \"%s\")\n", dir, dir->id,
		name));

	// lock the directory
	NodeWriteLocker nodeLocker(this, dir, false);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	// get the node ID (for the node monitoring message)
	ino_t nodeID;
	bool doNodeMonitoring = _GetNodeID(dir, name, &nodeID);

	AutoLocker<Locker> locker(fLock);

	// get a path for the entry
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen;
	status_t error = _BuildPath(dir, name, path, pathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	locker.Unlock();

	// remove the dir
	int fuseError = fuse_fs_rmdir(fFS, path);
	if (fuseError != 0)
		RETURN_ERROR(fuseError);

	// remove the entry
	locker.Lock();
	_RemoveEntry(dir, name);

	// mark the parent dir dirty
	dir->dirty = true;

	// send node monitoring message
	if (doNodeMonitoring) {
		UserlandFS::KernelEmu::notify_listener(B_ENTRY_REMOVED, 0, fID, 0,
			dir->id, nodeID, NULL, name);
	}

	return B_OK;
}


status_t
FUSEVolume::OpenDir(void* _node, void** _cookie)
{
	FUSENode* node = (FUSENode*)_node;
	PRINT(("FUSEVolume::OpenDir(%p (%" B_PRId64 "), %p)\n", node, node->id,
		_cookie));

	// lock the parent directory
	NodeReadLocker nodeLocker(this, node, true);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	// allocate a dir cookie
	DirCookie* cookie = new(std::nothrow) DirCookie;
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<DirCookie> cookieDeleter(cookie);

	AutoLocker<Locker> locker(fLock);

	// get a path for the node
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen;
	status_t error = _BuildPath(node, path, pathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	locker.Unlock();

	if (fFS->ops.readdir == NULL && fFS->ops.getdir != NULL) {
		// no open call -- the FS only supports the deprecated getdir()
		// interface
		cookie->getdirInterface = true;
	} else {
		// open the dir
		int fuseError = fuse_fs_opendir(fFS, path, cookie);
		if (fuseError != 0)
			return fuseError;
	}

	cookieDeleter.Detach();
	*_cookie = cookie;

	return B_OK;
}


status_t
FUSEVolume::CloseDir(void* node, void* _cookie)
{
	return B_OK;
}


status_t
FUSEVolume::FreeDirCookie(void* _node, void* _cookie)
{
	FUSENode* node = (FUSENode*)_node;
	DirCookie* cookie = (DirCookie*)_cookie;

	// lock the parent directory
	NodeReadLocker nodeLocker(this, node, true);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	ObjectDeleter<DirCookie> cookieDeleter(cookie);

	if (cookie->getdirInterface)
		return B_OK;

	AutoLocker<Locker> locker(fLock);

	// get a path for the node
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen;
	status_t error = _BuildPath(node, path, pathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	locker.Unlock();

	// release the dir
	int fuseError = fuse_fs_releasedir(fFS, path, cookie);
	if (fuseError != 0)
		return fuseError;

	return B_OK;
}


status_t
FUSEVolume::ReadDir(void* _node, void* _cookie, void* buffer, size_t bufferSize,
	uint32 count, uint32* _countRead)
{
	PRINT(("FUSEVolume::ReadDir(%p, %p, %p, %" B_PRIuSIZE ", %" B_PRId32 ")\n",
		_node, _cookie, buffer, bufferSize, count));
	*_countRead = 0;

	FUSENode* node = (FUSENode*)_node;
	DirCookie* cookie = (DirCookie*)_cookie;

	RWLockableWriteLocker cookieLocker(this, cookie);

	uint32 countRead = 0;
	status_t readDirError = B_OK;

	AutoLocker<Locker> locker(fLock);

	if (cookie->entryCache == NULL) {
		// We don't have an entry cache (yet), so we need to ask the client
		// file system to read the directory.

		locker.Unlock();

		// lock the directory
		NodeReadLocker nodeLocker(this, node, false);
		if (nodeLocker.Status() != B_OK)
			RETURN_ERROR(nodeLocker.Status());

		locker.Lock();

		ReadDirBuffer readDirBuffer(this, node, cookie, buffer, bufferSize,
			count);

		// get a path for the node
		char path[B_PATH_NAME_LENGTH];
		size_t pathLen;
		status_t error = _BuildPath(node, path, pathLen);
		if (error != B_OK)
			RETURN_ERROR(error);

		off_t offset = cookie->currentEntryOffset;

		locker.Unlock();

		// read the dir
		int fuseError;
		if (cookie->getdirInterface) {
PRINT(("  using getdir() interface\n"));
			fuseError = fFS->ops.getdir(path, (fuse_dirh_t)&readDirBuffer,
				&_AddReadDirEntryGetDir);
		} else {
PRINT(("  using readdir() interface\n"));
			fuseError = fuse_fs_readdir(fFS, path, &readDirBuffer,
				&_AddReadDirEntry, offset, cookie);
		}
		if (fuseError != 0)
			return fuseError;

		locker.Lock();

		countRead = readDirBuffer.entriesRead;
		readDirError = readDirBuffer.error;
	}

	if (cookie->entryCache != NULL) {
		// we're using an entry cache -- read into the buffer what we can
		dirent* entryBuffer = (dirent*)buffer;
		while (countRead < count
			&& cookie->entryCache->ReadDirent(cookie->currentEntryIndex, fID,
				countRead + 1 < count, entryBuffer, bufferSize)) {
			countRead++;
			cookie->currentEntryIndex++;
			bufferSize -= entryBuffer->d_reclen;
			entryBuffer
				= (dirent*)((uint8*)entryBuffer + entryBuffer->d_reclen);
		}
	}

	*_countRead = countRead;
	return countRead > 0 ? B_OK : readDirError;
}


status_t
FUSEVolume::RewindDir(void* _node, void* _cookie)
{
PRINT(("FUSEVolume::RewindDir(%p, %p)\n", _node, _cookie));
	DirCookie* cookie = (DirCookie*)_cookie;

	RWLockableWriteLocker cookieLocker(this, cookie);

	if (cookie->getdirInterface) {
		delete cookie->entryCache;
		cookie->entryCache = NULL;
		cookie->currentEntryIndex = 0;
	} else {
		cookie->currentEntryOffset = 0;
	}

	return B_OK;
}


// #pragma mark - attribute directories


// OpenAttrDir
status_t
FUSEVolume::OpenAttrDir(void* _node, void** _cookie)
{
	// allocate an attribute directory cookie
	AttrDirCookie* cookie = new(std::nothrow) AttrDirCookie;
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	*_cookie = cookie;

	return B_OK;
}


// CloseAttrDir
status_t
FUSEVolume::CloseAttrDir(void* node, void* cookie)
{
	return B_OK;
}


// FreeAttrDirCookie
status_t
FUSEVolume::FreeAttrDirCookie(void* _node, void* _cookie)
{
	delete (AttrDirCookie*)_cookie;
	return B_OK;
}


// ReadAttrDir
status_t
FUSEVolume::ReadAttrDir(void* _node, void* _cookie, void* buffer,
	size_t bufferSize, uint32 count, uint32* _countRead)
{
	FUSENode* node = (FUSENode*)_node;
	AttrDirCookie* cookie = (AttrDirCookie*)_cookie;

	RWLockableWriteLocker cookieLocker(this, cookie);

	*_countRead = 0;

	// lock the directory
	NodeReadLocker nodeLocker(this, node, true);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	AutoLocker<Locker> locker(fLock);

	// get a path for the node
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen;
	status_t error = _BuildPath(node, path, pathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	locker.Unlock();

	if (!cookie->IsValid()) {
		// cookie not yet valid -- get the length of the list
		int listSize = fuse_fs_listxattr(fFS, path, NULL, 0);
		if (listSize < 0)
			RETURN_ERROR(listSize);

		while (true) {
			// allocate space for the listing
			error = cookie->Allocate(listSize);
			if (error != B_OK)
				RETURN_ERROR(error);

			// read the listing
			int bytesRead = fuse_fs_listxattr(fFS, path,
				cookie->AttributesBuffer(), listSize);
			if (bytesRead < 0)
				RETURN_ERROR(bytesRead);

			if (bytesRead == listSize)
				break;

			// attributes listing changed -- reread it
			listSize = bytesRead;
		}

		cookie->SetValid(true);
	}

	// we have a valid cookie now -- get the next entries from the cookie
	uint32 countRead = 0;
	dirent* entryBuffer = (dirent*)buffer;
	while (countRead < count
		&& cookie->ReadNextEntry(fID, node->id, countRead + 1 < count,
			entryBuffer, bufferSize)) {
		countRead++;
		bufferSize -= entryBuffer->d_reclen;
		entryBuffer = (dirent*)((uint8*)entryBuffer + entryBuffer->d_reclen);
	}

	*_countRead = countRead;
	return B_OK;
}


// RewindAttrDir
status_t
FUSEVolume::RewindAttrDir(void* _node, void* _cookie)
{
	AttrDirCookie* cookie = (AttrDirCookie*)_cookie;

	RWLockableWriteLocker cookieLocker(this, cookie);

	cookie->Clear();

	return B_OK;
}


// #pragma mark - attributes


status_t
FUSEVolume::OpenAttr(void* _node, const char* name, int openMode,
	void** _cookie)
{
	FUSENode* node = (FUSENode*)_node;

	// lock the node
	NodeReadLocker nodeLocker(this, node, true);
	if (nodeLocker.Status() != B_OK)
		RETURN_ERROR(nodeLocker.Status());

	if (openMode != O_RDONLY) {
		// Write support currently not implemented
		RETURN_ERROR(B_UNSUPPORTED);
	}

	AutoLocker<Locker> locker(fLock);

	// get a path for the node
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen;
	status_t error = _BuildPath(node, path, pathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	locker.Unlock();

	int attrSize = fuse_fs_getxattr(fFS, path, name, NULL, 0);
	if (attrSize < 0) {
		if (strcmp(name, kAttrMimeTypeName) == 0) {
			// Return a fake MIME type attribute based on the file extension
			const char* mimeType = NULL;
			error = set_mime(&mimeType, S_ISDIR(node->type) ? NULL : &path[0]);
			if (error != B_OK)
				return error;
			*_cookie = new(std::nothrow)AttrCookie(name, mimeType);
			return B_OK;
		}

		// Reading attribute failed
		return attrSize;
	}

	AttrCookie* cookie = new(std::nothrow)AttrCookie(name);
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	error = cookie->Allocate(attrSize);
	if (error != B_OK) {
		delete cookie;
		RETURN_ERROR(error);
	}

	int bytesRead = fuse_fs_getxattr(fFS, path, name, cookie->Buffer(),
		attrSize);
	if (bytesRead < 0)
		return bytesRead;

	*_cookie = cookie;

	return B_OK;
}


status_t
FUSEVolume::CloseAttr(void* _node, void* _cookie)
{
	return B_OK;
}


status_t
FUSEVolume::FreeAttrCookie(void* _node, void* _cookie)
{
	delete (AttrCookie*)_cookie;
	return B_OK;
}


status_t
FUSEVolume::ReadAttr(void* _node, void* _cookie, off_t pos, void* buffer,
	size_t bufferSize, size_t* bytesRead)
{
	AttrCookie* cookie = (AttrCookie*)_cookie;

	RWLockableWriteLocker cookieLocker(this, cookie);

	if (!cookie->IsValid())
		RETURN_ERROR(B_BAD_VALUE);

	cookie->Read(buffer, bufferSize, pos, bytesRead);

	return B_OK;
}


status_t
FUSEVolume::ReadAttrStat(void* _node, void* _cookie, struct stat* st)
{
	AttrCookie* cookie = (AttrCookie*)_cookie;

	RWLockableWriteLocker cookieLocker(this, cookie);

	if (!cookie->IsValid())
		RETURN_ERROR(B_BAD_VALUE);

	st->st_size = cookie->Size();
	st->st_type = cookie->Type();

	return B_OK;
}


// #pragma mark -


ino_t
FUSEVolume::_GenerateNodeID()
{
	ino_t id;
	do {
		id = fNextNodeID++;
	} while (fNodes.Lookup(id) != NULL);

	return id;
}


/*!	Gets the ID of the node the entry specified by \a dir and \a entryName
	refers to. The ID is returned via \a _nodeID. The caller doesn't get a
	reference to the node.
*/
bool
FUSEVolume::_GetNodeID(FUSENode* dir, const char* entryName, ino_t* _nodeID)
{
	while (true) {
		AutoLocker<Locker> locker(fLock);

		FUSENode* node;
		status_t error = _InternalGetNode(dir, entryName, &node, locker);
		if (error != B_OK)
			return false;

		if (node == NULL)
			continue;

		*_nodeID = node->id;
		_PutNode(node);

		return true;
	}
}


/*!	Gets the node the entry specified by \a dir and \a entryName refers to. The
	found node is returned via \a _node. The caller gets a reference to the node
	as well as a vnode reference.
*/
status_t
FUSEVolume::_GetNode(FUSENode* dir, const char* entryName, FUSENode** _node)
{
	while (true) {
		AutoLocker<Locker> locker(fLock);

		FUSENode* node;
		status_t error = _InternalGetNode(dir, entryName, &node, locker);
		if (error != B_OK)
			return error;

		if (node == NULL)
			continue;

		ino_t nodeID = node->id;

		locker.Unlock();

		// get a reference for the caller
		void* privateNode;
		error = UserlandFS::KernelEmu::get_vnode(fID, nodeID, &privateNode);
		if (error != B_OK)
			RETURN_ERROR(error);

		locker.Lock();

		if (privateNode != node) {
			// weird, the node changed!
			ERROR(("FUSEVolume::_GetNode(): cookie for node %" B_PRId64
				" changed: expected: %p, got: %p\n", nodeID, node,
				privateNode));
			UserlandFS::KernelEmu::put_vnode(fID, nodeID);
			_PutNode(node);
			continue;
		}

		// Put the node reference we got from _InternalGetNode. We've now got
		// a reference from get_vnode().
		_PutNode(node);

		*_node = node;
		return B_OK;
	}
}


status_t
FUSEVolume::_InternalGetNode(FUSENode* dir, const char* entryName,
	FUSENode** _node, AutoLocker<Locker>& locker)
{
	// handle special cases
	if (strcmp(entryName, ".") == 0) {
		// same directory
		if (!S_ISDIR(dir->type))
			RETURN_ERROR(B_NOT_A_DIRECTORY);

		dir->refCount++;
		*_node = dir;
		return B_OK;
	}

	if (strcmp(entryName, "..") == 0) {
		// parent directory
		if (!S_ISDIR(dir->type))
			RETURN_ERROR(B_NOT_A_DIRECTORY);

		FUSEEntry* entry = dir->entries.Head();
		if (entry == NULL)
			RETURN_ERROR(B_ENTRY_NOT_FOUND);

		entry->parent->refCount++;
		*_node = entry->parent;
		return B_OK;
	}

	// lookup the entry in the table
	FUSEEntry* entry = fEntries.Lookup(FUSEEntryRef(dir->id, entryName));
	if (entry != NULL) {
		entry->node->refCount++;
		*_node = entry->node;
		return B_OK;
	}

	// construct a path for the entry
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen = 0;
	status_t error = _BuildPath(dir, entryName, path, pathLen);
	if (error != B_OK)
		return error;

	locker.Unlock();

	// stat the path
	struct stat st;
	int fuseError = fuse_fs_getattr(fFS, path, &st);
	if (fuseError != 0)
		return fuseError;

	locker.Lock();

	// lookup the entry in the table again
	entry = fEntries.Lookup(FUSEEntryRef(dir->id, entryName));
	if (entry != NULL) {
		// check whether the node still matches
		if (entry->node->id == st.st_ino) {
			entry->node->refCount++;
			*_node = entry->node;
		} else {
			// nope, something changed -- return a NULL node and let the caller
			// call us again
			*_node = NULL;
		}

		return B_OK;
	}

	// lookup the node in the table
	FUSENode* node = NULL;
	if (fUseNodeIDs)
		node = fNodes.Lookup(st.st_ino);
	else
		st.st_ino = _GenerateNodeID();

	if (node == NULL) {
		// no node yet -- create one
		node = new(std::nothrow) FUSENode(st.st_ino, st.st_mode & S_IFMT);
		if (node == NULL)
			RETURN_ERROR(B_NO_MEMORY);

		fNodes.Insert(node);
	} else {
		// get a node reference for the entry
		node->refCount++;
	}

	// create the entry
	entry = FUSEEntry::Create(dir, entryName, node);
	if (entry == NULL) {
		_PutNode(node);
		RETURN_ERROR(B_NO_MEMORY);
	}

	dir->refCount++;
		// dir reference for the entry

	fEntries.Insert(entry);
	node->entries.Add(entry);

	locker.Unlock();

	// get a reference for the caller
	node->refCount++;

	*_node = node;
	return B_OK;
}


void
FUSEVolume::_PutNode(FUSENode* node)
{
	if (--node->refCount == 0) {
		fNodes.Remove(node);
		delete node;
	}
}


void
FUSEVolume::_PutNodes(FUSENode* const* nodes, int32 count)
{
	for (int32 i = 0; i < count; i++)
		_PutNode(nodes[i]);
}


/*!	Volume must be locked. The entry's directory must be write locked.
 */
void
FUSEVolume::_RemoveEntry(FUSEEntry* entry)
{
	fEntries.Remove(entry);
	entry->node->entries.Remove(entry);
	_PutNode(entry->node);
	_PutNode(entry->parent);
	delete entry;
}


/*!	Volume must be locked. The directory must be write locked.
 */
status_t
FUSEVolume::_RemoveEntry(FUSENode* dir, const char* name)
{
	FUSEEntry* entry = fEntries.Lookup(FUSEEntryRef(dir->id, name));
	if (entry == NULL)
		return B_ENTRY_NOT_FOUND;

	_RemoveEntry(entry);
	return B_OK;
}


/*!	Volume must be locked. The directories must be write locked.
 */
status_t
FUSEVolume::_RenameEntry(FUSENode* oldDir, const char* oldName,
	FUSENode* newDir, const char* newName)
{
	FUSEEntry* entry = fEntries.Lookup(FUSEEntryRef(oldDir->id, oldName));
	if (entry == NULL)
		return B_ENTRY_NOT_FOUND;

	// get a node reference for the new entry
	FUSENode* node = entry->node;
	node->refCount++;

	// remove the old entry
	_RemoveEntry(entry);

	// make sure there's no entry in our way
	_RemoveEntry(newDir, newName);

	// create a new entry
	entry = FUSEEntry::Create(newDir, newName, node);
	if (entry == NULL) {
		_PutNode(node);
		RETURN_ERROR(B_NO_MEMORY);
	}

	newDir->refCount++;
		// dir reference for the entry

	fEntries.Insert(entry);
	node->entries.Add(entry);

	return B_OK;
}


/*!	Locks the given node and all of its ancestors up to the root. The given
	node is write-locked, if \a writeLock is \c true, read-locked otherwise. All
	ancestors are always read-locked in either case.

	If \a lockParent is \c true, the given node itself is ignored, but locking
	starts with the parent node of the given node (\a writeLock applies to the
	parent node then).

	If the method fails, none of the nodes is locked.

	The volume lock must not be held.
*/
status_t
FUSEVolume::_LockNodeChain(FUSENode* node, bool lockParent, bool writeLock)
{
	AutoLocker<Locker> locker(fLock);

	FUSENode* originalNode = node;

	if (lockParent && node != NULL)
		node = node->Parent();

	if (node == NULL)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);

	LockIterator iterator(this, node, writeLock, NULL);

	bool done;
	do {
		bool volumeUnlocked;
		status_t error = iterator.LockNext(&done, &volumeUnlocked);
		if (error != B_OK)
			RETURN_ERROR(error);

		if (volumeUnlocked) {
			// check whether we're still locking the right node
			if (lockParent && originalNode->Parent() != node) {
				// We don't -- unlock everything and try again.
				node = originalNode->Parent();
				iterator.SetTo(this, node, writeLock, NULL);
			}
		}
	} while (!done);

	// Fail, if we couldn't lock all nodes up to the root.
	if (iterator.lastLockedNode != fRootNode)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);

	iterator.Detach();
	return B_OK;
}


void
FUSEVolume::_UnlockNodeChain(FUSENode* node, bool parent, bool writeLock)
{
	AutoLocker<Locker> locker(fLock);

	if (parent && node != NULL)
		node = node->Parent();

	_UnlockNodeChainInternal(node, writeLock, NULL, NULL);
}


/*!	Unlocks all nodes from \a node up to (and including) \a stopNode (if
	\c NULL, it is ignored). If \a stopBeforeNode is given, the method stops
	before unlocking that node.
	The volume lock must be held.
 */
void
FUSEVolume::_UnlockNodeChainInternal(FUSENode* node, bool writeLock,
	FUSENode* stopNode, FUSENode* stopBeforeNode)
{
	FUSENode* originalNode = node;

	while (node != NULL && node != stopBeforeNode) {
		FUSENode* parent = node->Parent();

		fLockManager.GenericUnlock(node == originalNode && writeLock, node);
		_PutNode(node);

		if (node == stopNode || parent == node)
			break;

		node = parent;
	}
}


status_t
FUSEVolume::_LockNodeChains(FUSENode* node1, bool lockParent1, bool writeLock1,
	FUSENode* node2, bool lockParent2, bool writeLock2)
{
	// Since in this case locking is more complicated, we use a helper method.
	// It does the main work, but simply returns telling us to retry when the
	// node hierarchy changes.
	bool retry;
	do {
		status_t error = _LockNodeChainsInternal(node1, lockParent1, writeLock1,
			node2, lockParent2, writeLock2, &retry);
		if (error != B_OK)
			return error;
	} while (retry);

	return B_OK;
}


status_t
FUSEVolume::_LockNodeChainsInternal(FUSENode* node1, bool lockParent1,
	bool writeLock1, FUSENode* node2, bool lockParent2, bool writeLock2,
	bool* _retry)
{
	// Locking order:
	// * A child of a node has to be locked before its parent.
	// * Sibling nodes have to be locked in ascending node ID order.
	//
	// This implies the following locking algorithm:
	// * We find the closest common ancestor of the two given nodes (might even
	//   be one of the given nodes).
	// * We lock all ancestors on one branch (the one with the lower common
	//   ancestor child node ID), but not including the common ancestor.
	// * We lock all ancestors on the other branch, not including the common
	//   ancestor.
	// * We lock the common ancestor and all of its ancestors up to the root
	//   node.
	//
	// When the hierarchy changes while we're waiting for a lock, we recheck the
	// conditions and in doubt have to be restarted.

	AutoLocker<Locker> locker(fLock);

	FUSENode* originalNode1 = node1;
	FUSENode* originalNode2 = node2;

	if (lockParent1 && node1 != NULL)
		node1 = node1->Parent();

	if (lockParent2 && node2 != NULL)
		node2 = node2->Parent();

	if (node1 == NULL || node2 == NULL)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);

	// find the first common ancestor
	FUSENode* commonAncestor;
	bool inverseLockingOrder;
	if (!_FindCommonAncestor(node1, node2, &commonAncestor,
			&inverseLockingOrder)) {
		RETURN_ERROR(B_ENTRY_NOT_FOUND);
	}

	// lock the both node chains up to (but not including) the common ancestor
	LockIterator iterator1(this, node1, writeLock1, commonAncestor);
	LockIterator iterator2(this, node2, writeLock2, commonAncestor);

	for (int i = 0; i < 2; i++) {
		LockIterator& iterator = (i == 0) != inverseLockingOrder
			? iterator1 : iterator2;

		// If the node is the common ancestor, don't enter the "do" loop, since
		// we don't have to lock anything here.
		if (iterator.firstNode == commonAncestor)
			continue;

		bool done;
		do {
			bool volumeUnlocked;
			status_t error = iterator.LockNext(&done, &volumeUnlocked);
			if (error != B_OK)
				RETURN_ERROR(error);

			if (volumeUnlocked) {
				// check whether we're still locking the right nodes
				if ((lockParent1 && originalNode1->Parent() != node1)
					|| (lockParent2 && originalNode2->Parent() != node2)) {
					// We don't -- unlock everything and retry.
					*_retry = true;
					return B_OK;
				}

				// also recheck the common ancestor
				FUSENode* newCommonParent;
				bool newInverseLockingOrder;
				if (!_FindCommonAncestor(node1, node2, &newCommonParent,
						&newInverseLockingOrder)) {
					RETURN_ERROR(B_ENTRY_NOT_FOUND);
				}

				if (newCommonParent != commonAncestor
					|| inverseLockingOrder != newInverseLockingOrder) {
					// Something changed -- unlock everything and retry.
					*_retry = true;
					return B_OK;
				}
			}
		} while (!done);
	}

	// Continue locking from the common ancestor to the root. If one of the
	// given nodes is the common ancestor and shall be write locked, we need to
	// use the respective iterator.
	LockIterator& iterator = node2 == commonAncestor && writeLock2
		? iterator2 : iterator1;
	iterator.SetStopBeforeNode(NULL);

	bool done;
	do {
		bool volumeUnlocked;
		status_t error = iterator.LockNext(&done, &volumeUnlocked);
		if (error != B_OK)
			RETURN_ERROR(error);

		if (volumeUnlocked) {
			// check whether we're still locking the right nodes
			if ((lockParent1 && originalNode1->Parent() != node1)
				|| (lockParent2 && originalNode2->Parent() != node2)) {
				// We don't -- unlock everything and retry.
				*_retry = true;
				return B_OK;
			}

			// Also recheck the common ancestor, if we have just locked it.
			// Otherwise we can just continue to lock, since nothing below the
			// previously locked node can have changed.
			if (iterator.lastLockedNode == commonAncestor) {
				FUSENode* newCommonParent;
				bool newInverseLockingOrder;
				if (!_FindCommonAncestor(node1, node2, &newCommonParent,
						&newInverseLockingOrder)) {
					RETURN_ERROR(B_ENTRY_NOT_FOUND);
				}

				if (newCommonParent != commonAncestor
					|| inverseLockingOrder != newInverseLockingOrder) {
					// Something changed -- unlock everything and retry.
					*_retry = true;
					return B_OK;
				}
			}
		}
	} while (!done);

	// Fail, if we couldn't lock all nodes up to the root.
	if (iterator.lastLockedNode != fRootNode)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);

	// everything went fine
	iterator1.Detach();
	iterator2.Detach();

	*_retry = false;
	return B_OK;
}


void
FUSEVolume::_UnlockNodeChains(FUSENode* node1, bool lockParent1,
	bool writeLock1, FUSENode* node2, bool lockParent2, bool writeLock2)
{
	AutoLocker<Locker> locker(fLock);

	if (lockParent1 && node1 != NULL)
		node1 = node1->Parent();

	if (lockParent2 && node2 != NULL)
		node2 = node2->Parent();

	if (node1 == NULL || node2 == NULL)
		return;

	// find the common ancestor
	FUSENode* commonAncestor;
	bool inverseLockingOrder;
	if (!_FindCommonAncestor(node1, node2, &commonAncestor,
			&inverseLockingOrder)) {
		return;
	}

	// Unlock one branch up to the common ancestor and then the complete other
	// branch up to the root. If one of the given nodes is the common ancestor,
	// we need to make sure, we write-unlock it, if requested.
	if (node2 == commonAncestor && writeLock2) {
		_UnlockNodeChainInternal(node1, writeLock1, NULL, commonAncestor);
		_UnlockNodeChainInternal(node2, writeLock2, NULL, NULL);
	} else {
		_UnlockNodeChainInternal(node2, writeLock2, NULL, commonAncestor);
		_UnlockNodeChainInternal(node1, writeLock1, NULL, NULL);
	}
}


bool
FUSEVolume::_FindCommonAncestor(FUSENode* node1, FUSENode* node2,
	FUSENode** _commonAncestor, bool* _inverseLockingOrder)
{
	// handle trivial special case -- both nodes are the same
	if (node1 == node2) {
		*_commonAncestor = node1;
		*_inverseLockingOrder = false;
		return true;
	}

	// get the ancestors of both nodes
	FUSENode* ancestors1[kMaxNodeTreeDepth];
	FUSENode* ancestors2[kMaxNodeTreeDepth];
	uint32 count1;
	uint32 count2;

	if (!_GetNodeAncestors(node1, ancestors1, &count1)
		|| !_GetNodeAncestors(node2, ancestors2, &count2)) {
		return false;
	}

	// find the first ancestor not common to both nodes
	uint32 index = 0;
	for (; index < count1 && index < count2; index++) {
		FUSENode* ancestor1 = ancestors1[count1 - index - 1];
		FUSENode* ancestor2 = ancestors2[count2 - index - 1];
		if (ancestor1 != ancestor2) {
			*_commonAncestor = ancestors1[count1 - index];
			*_inverseLockingOrder = ancestor1->id > ancestor2->id;
			return true;
		}
	}

	// one node is an ancestor of the other
	*_commonAncestor = ancestors1[count1 - index];
	*_inverseLockingOrder = index == count1;
	return true;
}


bool
FUSEVolume::_GetNodeAncestors(FUSENode* node, FUSENode** ancestors,
	uint32* _count)
{
	uint32 count = 0;
	while (node != NULL && count < kMaxNodeTreeDepth) {
		ancestors[count++] = node;

		if (node == fRootNode) {
			*_count = count;
			return true;
		}

		node = node->Parent();
	}

	// Either the node is not in the tree or we hit the array limit.
	return false;
}


status_t
FUSEVolume::_BuildPath(FUSENode* dir, const char* entryName, char* path,
	size_t& pathLen)
{
	// get the directory path
	status_t error = _BuildPath(dir, path, pathLen);
	if (error != B_OK)
		return error;

	if (path[pathLen - 1] != '/') {
		path[pathLen++] = '/';
		if (pathLen == B_PATH_NAME_LENGTH)
			RETURN_ERROR(B_NAME_TOO_LONG);
	}

	// append the entry name
	size_t len = strlen(entryName);
	if (pathLen + len >= B_PATH_NAME_LENGTH)
		RETURN_ERROR(B_NAME_TOO_LONG);

	memcpy(path + pathLen, entryName, len + 1);
	pathLen += len;

	return B_OK;
}


status_t
FUSEVolume::_BuildPath(FUSENode* node, char* path, size_t& pathLen)
{
	if (node == fRootNode) {
		// we hit the root
		strcpy(path, "/");
		pathLen = 1;
		return B_OK;
	}

	// get an entry for the node and get its path
	FUSEEntry* entry = node->entries.Head();
	if (entry == NULL)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);

	return _BuildPath(entry->parent, entry->name, path, pathLen);
}


/*static*/ int
FUSEVolume::_AddReadDirEntry(void* _buffer, const char* name,
	const struct stat* st, off_t offset)
{
	ReadDirBuffer* buffer = (ReadDirBuffer*)_buffer;

	ino_t nodeID = st != NULL ? st->st_ino : 0;
	int type = st != NULL ? st->st_mode & S_IFMT : 0;
	return buffer->volume->_AddReadDirEntry(buffer, name, type, nodeID, offset);
}


/*static*/ int
FUSEVolume::_AddReadDirEntryGetDir(fuse_dirh_t handle, const char* name,
	int type, ino_t nodeID)
{
	ReadDirBuffer* buffer = (ReadDirBuffer*)handle;
	return buffer->volume->_AddReadDirEntry(buffer, name, type << 12, nodeID,
		0);
}


int
FUSEVolume::_AddReadDirEntry(ReadDirBuffer* buffer, const char* name, int type,
	ino_t nodeID, off_t offset)
{
	PRINT(("FUSEVolume::_AddReadDirEntry(%p, \"%s\", %#x, %" B_PRId64 ", %"
		B_PRId64 "\n", buffer, name, type, nodeID, offset));

	AutoLocker<Locker> locker(fLock);

	size_t entryLen = 0;
	if (offset != 0) {
		// does the caller want more entries?
		if (buffer->entriesRead == buffer->maxEntries)
			return 1;

		// compute the entry length and check whether the entry still fits
		entryLen = sizeof(dirent) + strlen(name);
		if (buffer->usedSize + entryLen > buffer->bufferSize)
			return 1;
	}

	// create a node and an entry, if necessary
	ino_t dirID = buffer->directory->id;
	FUSEEntry* entry;
	if (strcmp(name, ".") == 0) {
		// current dir entry
		nodeID = dirID;
		type = S_IFDIR;
	} else if (strcmp(name, "..") == 0) {
		// parent dir entry
		FUSEEntry* parentEntry = buffer->directory->entries.Head();
		if (parentEntry == NULL) {
			ERROR(("FUSEVolume::_AddReadDirEntry(): dir %" B_PRId64
				" has no entry!\n", dirID));
			return 0;
		}
		nodeID = parentEntry->parent->id;
		type = S_IFDIR;
	} else if ((entry = fEntries.Lookup(FUSEEntryRef(dirID, name))) == NULL) {
		// get the node
		FUSENode* node = NULL;
		if (fUseNodeIDs)
			node = fNodes.Lookup(nodeID);
		else
			nodeID = _GenerateNodeID();

		if (node == NULL) {
			// no node yet -- create one

			// If we don't have a valid type, we need to stat the node first.
			if (type == 0) {
				char path[B_PATH_NAME_LENGTH];
				size_t pathLen;
				status_t error = _BuildPath(buffer->directory, name, path,
					pathLen);
				if (error != B_OK) {
					buffer->error = error;
					return 0;
				}

				locker.Unlock();

				// stat the path
				struct stat st;
				int fuseError = fuse_fs_getattr(fFS, path, &st);

				locker.Lock();

				if (fuseError != 0) {
					buffer->error = fuseError;
					return 0;
				}

				type = st.st_mode & S_IFMT;
			}

			node = new(std::nothrow) FUSENode(nodeID, type);
			if (node == NULL) {
				buffer->error = B_NO_MEMORY;
				return 1;
			}
			PRINT(("  -> create node: %p, id: %" B_PRId64 "\n", node, nodeID));

			fNodes.Insert(node);
		} else {
			// get a node reference for the entry
			node->refCount++;
		}

		// create the entry
		entry = FUSEEntry::Create(buffer->directory, name, node);
		if (entry == NULL) {
			_PutNode(node);
			buffer->error = B_NO_MEMORY;
			return 1;
		}

		buffer->directory->refCount++;
			// dir reference for the entry

		fEntries.Insert(entry);
		node->entries.Add(entry);
	} else {
		// TODO: Check whether the node's ID matches the one we got (if any)!
		nodeID = entry->node->id;
		type = entry->node->type;
	}

	if (offset == 0) {
		// cache the entry
		if (buffer->cookie->entryCache == NULL) {
			// no cache yet -- create it
			buffer->cookie->entryCache = new(std::nothrow) DirEntryCache;
			if (buffer->cookie->entryCache == NULL) {
				buffer->error = B_NO_MEMORY;
				return 1;
			}
		}

		status_t error = buffer->cookie->entryCache->AddEntry(nodeID, name);
		if (error != B_OK) {
			buffer->error = error;
			return 1;
		}
	} else {
		// fill in the dirent
		dirent* dirEntry = (dirent*)((uint8*)buffer->buffer + buffer->usedSize);
		dirEntry->d_dev = fID;
		dirEntry->d_ino = nodeID;
		strcpy(dirEntry->d_name, name);

		if (buffer->entriesRead + 1 < buffer->maxEntries) {
			// align the entry length, so the next dirent will be aligned
			entryLen = (entryLen + 7) / 8 * 8;
			entryLen = std::min(entryLen,
				buffer->bufferSize - buffer->usedSize);
		}

		dirEntry->d_reclen = entryLen;

		// update the buffer
		buffer->usedSize += entryLen;
		buffer->entriesRead++;
		buffer->cookie->currentEntryOffset = offset;
	}

	return 0;
}
