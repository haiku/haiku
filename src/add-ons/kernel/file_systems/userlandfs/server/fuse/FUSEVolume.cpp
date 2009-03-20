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


static inline status_t
from_fuse_error(int error)
{
	return error < 0 ? error : -error;
}


struct FUSEVolume::DirCookie : fuse_file_info {
	off_t		currentEntryOffset;

	DirCookie()
		:
		currentEntryOffset(0)
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
	off_t		lastOffset;
	status_t	error;

	ReadDirBuffer(FUSEVolume* volume, FUSENode* directory, DirCookie* cookie,
		void* buffer, size_t bufferSize, uint32 maxEntries)
		:
		directory(directory),
		cookie(cookie),
		buffer(buffer),
		bufferSize(bufferSize),
		usedSize(0),
		entriesRead(0),
		maxEntries(maxEntries),
		lastOffset(0),
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
		RETURN_ERROR(from_fuse_error(fuseError));

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
		return from_fuse_error(fuseError);

	info->flags = ((st.f_fsid & ST_RDONLY) != 0 ? B_FS_IS_READONLY : 0)
		| B_FS_IS_PERSISTENT;			// assume the FS is persistent
	info->block_size = st.f_bsize;
	info->io_size = 64 * 1024;			// some value
	info->total_blocks = st.f_blocks;
	info->free_blocks = st.f_bfree;
	info->total_nodes = st.f_files;
	info->free_nodes = st.f_favail;
	info->volume_name[0] = '\0';		// no way to get the name (if any)

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
FUSEVolume::ReadSymlink(void* node, char* buffer, size_t bufferSize,
	size_t* bytesRead)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
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
FUSEVolume::Access(void* node, int mode)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
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
		return from_fuse_error(fuseError);

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
FUSEVolume::Open(void* node, int openMode, void** cookie)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::Close(void* node, void* cookie)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::FreeCookie(void* node, void* cookie)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::Read(void* node, void* cookie, off_t pos, void* buffer,
	size_t bufferSize, size_t* bytesRead)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
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

	// open the dir
	int fuseError = fuse_fs_opendir(fFS, path, cookie);
	if (fuseError != 0)
		return from_fuse_error(fuseError);

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
		return from_fuse_error(fuseError);

	return B_OK;
}


status_t
FUSEVolume::ReadDir(void* _node, void* _cookie, void* buffer, size_t bufferSize,
	uint32 count, uint32* countRead)
{
	FUSENode* node = (FUSENode*)_node;
	DirCookie* cookie = (DirCookie*)_cookie;

	AutoLocker<Locker> locker(fLock);

	ReadDirBuffer readDirBuffer(this, node, cookie, buffer, bufferSize, count);

	// get a path for the node
	char path[B_PATH_NAME_LENGTH];
	size_t pathLen;
	status_t error = _BuildPath(node, path, pathLen);
	if (error != B_OK)
		RETURN_ERROR(error);

	off_t offset = cookie->currentEntryOffset;

	locker.Unlock();

	// read the dir
	int fuseError = fuse_fs_readdir(fFS, path, &readDirBuffer,
		&_AddReadDirEntry, offset, cookie);
	if (fuseError != 0)
		return from_fuse_error(fuseError);

	locker.Lock();

	// everything went fine
	cookie->currentEntryOffset = readDirBuffer.lastOffset;
	*countRead = readDirBuffer.entriesRead;

	return B_OK;
}


status_t
FUSEVolume::RewindDir(void* node, void* cookie)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
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
		return from_fuse_error(fuseError);

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
	FUSEVolume* self = buffer->volume;

	AutoLocker<Locker> _(self->fLock);

	if (offset == 0) {
		ERROR(("FUSEVolume::_AddReadDirEntry(): Old interface not "
			"supported!\n"));
		buffer->error = B_ERROR;
		return 1;
	}

	if (buffer->entriesRead == buffer->maxEntries)
		return 1;

	// compute the entry length and check whether the entry still fits
	size_t nameLen = strlen(name);
	dirent* dirEntry = (dirent*)((uint8*)buffer->buffer + buffer->usedSize);
	size_t entryLen = dirEntry->d_name + nameLen + 1 - (char*)dirEntry;
	if (buffer->usedSize + entryLen > buffer->bufferSize)
		return 1;

	// create a node and an entry, if necessary
	FUSEEntry* entry = self->fEntries.Lookup(
		FUSEEntryRef(buffer->directory->id, name));
	if (entry == NULL) {
		// get the node
		ino_t nodeID = st->st_ino;
		FUSENode* node = NULL;
		if (self->fUseNodeIDs)
			node = self->fNodes.Lookup(st->st_ino);
		else
			nodeID = self->_GenerateNodeID();

		if (node == NULL) {
			// no node yet -- create one
			node = new(std::nothrow) FUSENode(nodeID, st->st_mode & S_IFMT);
			if (node == NULL) {
				buffer->error = B_NO_MEMORY;
				return 1;
			}

			self->fNodes.Insert(node);
		} else {
			// get a node reference for the entry
			node->refCount++;
		}

		// create the entry
		entry = FUSEEntry::Create(buffer->directory, name, node);
		if (entry == NULL) {
			self->_PutNode(node);
			buffer->error = B_NO_MEMORY;
			return 1;
		}

		self->fEntries.Insert(entry);
		node->entries.Add(entry);
	} else {
		// TODO: Check whether the node's ID matches the one we got!
	}

	// fill in the dirent
	dirEntry->d_dev = self->fID;
	dirEntry->d_ino = entry->node->id;
	strcpy(dirEntry->d_name, name);

	if (buffer->entriesRead + 1 < buffer->maxEntries) {
		// align the entry length, so the next dirent will be aligned
		entryLen = (entryLen + 7) / 8 * 8;
		entryLen = std::min(entryLen, buffer->bufferSize - buffer->usedSize);
	}

	dirEntry->d_reclen = entryLen;

	// update the buffer
	buffer->usedSize += entryLen;
	buffer->entriesRead++;
	buffer->lastOffset = offset;

	return 0;
}
