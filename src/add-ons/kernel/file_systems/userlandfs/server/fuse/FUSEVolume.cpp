/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "FUSEVolume.h"

#include <dirent.h>

#include <algorithm>

#include <fs_info.h>

#include <AutoDeleter.h>

#include "FUSEFileSystem.h"

#include "../kernel_emu.h"


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


struct FUSEVolume::DirCookie : fuse_file_info {
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


struct FUSEVolume::FileCookie : fuse_file_info {
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
	// init entry and node tables
	status_t error = fEntries.Init();
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
	// TODO: Implement!
	// NOTE: There's no hook for sync'ing the whole FS.
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::ReadFSInfo(fs_info* info)
{
	if (fFS->ops.statfs == NULL)
		return B_UNSUPPORTED;

	struct statvfs st;
	int fuseError = fuse_fs_statfs(fFS, "/", &st);
	if (fuseError != 0)
		return fuseError;

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


status_t
FUSEVolume::WriteFSInfo(const struct fs_info* info, uint32 mask)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


// #pragma mark - vnodes


status_t
FUSEVolume::Lookup(void* _dir, const char* entryName, ino_t* vnid)
{
	FUSENode* dir = (FUSENode*)_dir;

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


// #pragma mark - asynchronous I/O


status_t
FUSEVolume::DoIO(void* node, void* cookie, const IORequestInfo& requestInfo)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::CancelIO(void* node, void* cookie, int32 ioRequestID)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::IterativeIOGetVecs(void* cookie, int32 requestID, off_t offset,
	size_t size, struct file_io_vec* vecs, size_t* _count)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::IterativeIOFinished(void* cookie, int32 requestID, status_t status,
	bool partialTransfer, size_t bytesTransferred)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


// #pragma mark - nodes


status_t
FUSEVolume::IOCtl(void* node, void* cookie, uint32 command, void *buffer,
	size_t size)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::SetFlags(void* node, void* cookie, int flags)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::Select(void* node, void* cookie, uint8 event, selectsync* sync)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::Deselect(void* node, void* cookie, uint8 event, selectsync* sync)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::FSync(void* node)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::ReadSymlink(void* _node, char* buffer, size_t bufferSize,
	size_t* _bytesRead)
{
	FUSENode* node = (FUSENode*)_node;

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
FUSEVolume::CreateSymlink(void* dir, const char* name, const char* target,
	int mode)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::Link(void* dir, const char* name, void* node)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::Unlink(void* dir, const char* name)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::Rename(void* oldDir, const char* oldName, void* newDir,
	const char* newName)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::Access(void* _node, int mode)
{
	FUSENode* node = (FUSENode*)_node;

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
PRINT(("FUSEVolume::ReadStat(%p (%lld), %p)\n", node, node->id, st));

	AutoLocker<Locker> locker(fLock);

	// get a path for the node
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen;
	status_t error = _BuildPath(node, path, pathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	locker.Unlock();

	// stat the path
	int fuseError = fuse_fs_getattr(fFS, path, st);
	if (fuseError != 0)
		return fuseError;

	return B_OK;
}


status_t
FUSEVolume::WriteStat(void* node, const struct stat *st, uint32 mask)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


// #pragma mark - files


status_t
FUSEVolume::Create(void* dir, const char* name, int openMode, int mode,
	void** cookie, ino_t* vnid)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::Open(void* _node, int openMode, void** _cookie)
{
	FUSENode* node = (FUSENode*)_node;
PRINT(("FUSEVolume::Open(%p (%lld), %#x)\n", node, node->id, openMode));

	bool truncate = (openMode & O_TRUNC) != 0;
	openMode &= ~O_TRUNC;

	// TODO: Support truncation!
	if (truncate)
		RETURN_ERROR(B_NOT_ALLOWED);

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

	// open the dir
	int fuseError = fuse_fs_open(fFS, path, cookie);
	if (fuseError != 0)
		return fuseError;

	cookieDeleter.Detach();
	*_cookie = cookie;

	return B_OK;
}


status_t
FUSEVolume::Close(void* _node, void* _cookie)
{
	FUSENode* node = (FUSENode*)_node;
	FileCookie* cookie = (FileCookie*)_cookie;

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

	*_bytesRead = 0;

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
FUSEVolume::Write(void* node, void* cookie, off_t pos, const void* buffer,
	size_t bufferSize, size_t* bytesWritten)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


// #pragma mark - directories


status_t
FUSEVolume::CreateDir(void* dir, const char* name, int mode, ino_t *newDir)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::RemoveDir(void* dir, const char* name)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::OpenDir(void* _node, void** _cookie)
{
	FUSENode* node = (FUSENode*)_node;
PRINT(("FUSEVolume::OpenDir(%p (%lld), %p)\n", node, node->id, _cookie));

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
PRINT(("FUSEVolume::ReadDir(%p, %p, %p, %lu, %ld)\n", _node, _cookie, buffer,
bufferSize, count));
	*_countRead = 0;

	FUSENode* node = (FUSENode*)_node;
	DirCookie* cookie = (DirCookie*)_cookie;

	uint32 countRead = 0;
	status_t readDirError = B_OK;

	AutoLocker<Locker> locker(fLock);

	if (cookie->entryCache == NULL) {
		// We don't have an entry cache (yet), so we need to ask the client
		// file system to read the directory.
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

	AutoLocker<Locker> locker(fLock);

	if (cookie->getdirInterface) {
		delete cookie->entryCache;
		cookie->entryCache = NULL;
		cookie->currentEntryIndex = 0;
	} else {
		cookie->currentEntryOffset = 0;
	}

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
			ERROR(("FUSEVolume::_GetNode(): cookie for node %lld changed: "
				"expected: %p, got: %p\n", nodeID, node, privateNode));
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
PRINT(("FUSEVolume::_AddReadDirEntry(%p, \"%s\", %#x, %lld, %lld\n", buffer,
name, type, nodeID, offset));

	AutoLocker<Locker> locker(fLock);

	size_t entryLen;
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
			ERROR(("FUSEVolume::_AddReadDirEntry(): dir %lld has no entry!\n",
				dirID));
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
PRINT(("  -> create node: %p, id: %lld\n", node, nodeID));

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
