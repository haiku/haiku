/*
 * Copyright 2001-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Volume.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <algorithm>

#include <fs_cache.h>

#include <util/AutoLock.h>
#include <util/OpenHashTable.h>

#include <fs/fd.h>	// kernel private
#include <io_requests.h>
#include <thread.h>

#include "IORequest.h"	// kernel internal

#include "Compatibility.h"
#include "Debug.h"
#include "FileSystem.h"
#include "IOCtlInfo.h"
#include "kernel_interface.h"
#include "KernelRequestHandler.h"
#include "PortReleaser.h"
#include "RequestAllocator.h"
#include "RequestPort.h"
#include "Requests.h"
#include "userlandfs_ioctl.h"

// missing ioctl()s
// TODO: Place somewhere else.
#define		IOCTL_FILE_UNCACHED_IO	10000
#define		IOCTL_CREATE_TIME		10002
#define		IOCTL_MODIFIED_TIME		10003

// If a thread of the userland server enters userland FS kernel code and
// is sending a request, this is the time after which it shall time out
// waiting for a reply.
static const bigtime_t kUserlandServerlandPortTimeout = 10000000;	// 10s


// VNode
struct Volume::VNode {
	ino_t		id;
	void*		clientNode;
	void*		fileCache;
	VNodeOps*	ops;
	int32		useCount;
	bool		valid;
	bool		published;
	VNode*		hash_link;

	VNode(ino_t id, void* clientNode, VNodeOps* ops)
		:
		id(id),
		clientNode(clientNode),
		fileCache(NULL),
		ops(ops),
		useCount(0),
		valid(true),
		published(true)
	{
	}

	void Delete(Volume* volume)
	{
		if (ops != NULL)
			volume->GetFileSystem()->PutVNodeOps(ops);
		delete this;
	}

protected:	// should be private, but gcc 2.95.3 issues a warning
	~VNode()
	{
		if (fileCache != NULL) {
			ERROR(("VNode %" B_PRId64 " still has a file cache!\n", id));
			file_cache_delete(fileCache);
		}
	}
};


// VNodeHashDefinition
struct Volume::VNodeHashDefinition {
	typedef ino_t	KeyType;
	typedef	VNode	ValueType;

	size_t HashKey(ino_t key) const
		{ return (uint32)key ^ (uint32)(key >> 32); }
	size_t Hash(const VNode* value) const
		{ return HashKey(value->id); }
	bool Compare(ino_t key, const VNode* value) const
		{ return value->id == key; }
	VNode*& GetLink(VNode* value) const
		{ return value->hash_link; }
};


// VNodeMap
struct Volume::VNodeMap
	: public BOpenHashTable<VNodeHashDefinition> {
};


// IORequestInfo
struct Volume::IORequestInfo {
	io_request*						request;
	int32							id;

	IORequestInfo*					idLink;
	IORequestInfo*					structLink;

	IORequestInfo(io_request* request, int32 id)
		:
		request(request),
		id(id)
	{
	}
};


// IORequestIDHashDefinition
struct Volume::IORequestIDHashDefinition {
	typedef int32			KeyType;
	typedef	IORequestInfo	ValueType;

	size_t HashKey(int32 key) const
		{ return key; }
	size_t Hash(const IORequestInfo* value) const
		{ return HashKey(value->id); }
	bool Compare(int32 key, const IORequestInfo* value) const
		{ return value->id == key; }
	IORequestInfo*& GetLink(IORequestInfo* value) const
		{ return value->idLink; }
};


// IORequestStructHashDefinition
struct Volume::IORequestStructHashDefinition {
	typedef io_request*		KeyType;
	typedef	IORequestInfo	ValueType;

	size_t HashKey(io_request* key) const
		{ return (size_t)(addr_t)key; }
	size_t Hash(const IORequestInfo* value) const
		{ return HashKey(value->request); }
	bool Compare(io_request* key, const IORequestInfo* value) const
		{ return value->request == key; }
	IORequestInfo*& GetLink(IORequestInfo* value) const
		{ return value->structLink; }
};


// IORequestIDMap
struct Volume::IORequestIDMap
	: public BOpenHashTable<IORequestIDHashDefinition> {
};


// IORequestStructMap
struct Volume::IORequestStructMap
	: public BOpenHashTable<IORequestStructHashDefinition> {
};


// IterativeFDIOCookie
struct Volume::IterativeFDIOCookie : public BReferenceable {
	Volume*				volume;
	int					fd;
	int32				requestID;
	void*				clientCookie;
	off_t				offset;
	const file_io_vec*	vecs;
	uint32				vecCount;

	IterativeFDIOCookie(Volume* volume, int fd, int32 requestID,
		void* clientCookie, off_t offset, const file_io_vec* vecs,
		uint32 vecCount)
		:
		volume(volume),
		fd(fd),
		requestID(requestID),
		clientCookie(clientCookie),
		offset(offset),
		vecs(vecs),
		vecCount(vecCount)
	{
	}

	~IterativeFDIOCookie()
	{
		if (fd >= 0)
			close(fd);
	}
};


// AutoIncrementer
class Volume::AutoIncrementer {
public:
	AutoIncrementer(int32* variable)
		: fVariable(variable)
	{
		if (fVariable)
			atomic_add(fVariable, 1);
	}

	~AutoIncrementer()
	{
		if (fVariable)
			atomic_add(fVariable, -1);
	}

	void Keep()
	{
		fVariable = NULL;
	}

private:
	int32*	fVariable;
};


// IORequestRemover
class Volume::IORequestRemover {
public:
	IORequestRemover(Volume* volume, int32 requestID)
		:
		fVolume(volume),
		fRequestID(requestID)
	{
	}

	~IORequestRemover()
	{
		if (fVolume != NULL)
			fVolume->_UnregisterIORequest(fRequestID);
	}

	void Detach()
	{
		fVolume = NULL;
	}

private:
	Volume*	fVolume;
	int32	fRequestID;
};


// VNodeRemover
class Volume::VNodeRemover {
public:
	VNodeRemover(Volume* volume, VNode* node)
		:
		fVolume(volume),
		fNode(node)
	{
	}

	~VNodeRemover()
	{
		if (fNode != NULL) {
			MutexLocker locker(fVolume->fLock);
			fVolume->fVNodes->Remove(fNode);
			locker.Unlock();

			fNode->Delete(fVolume);
		}
	}

private:
	Volume*	fVolume;
	VNode*	fNode;
};


// HasVNodeCapability
inline bool
Volume::HasVNodeCapability(VNode* vnode, int capability) const
{
	return vnode->ops->capabilities.Get(capability);
}


// constructor
Volume::Volume(FileSystem* fileSystem, fs_volume* fsVolume)
	:
	BReferenceable(),
	fFileSystem(fileSystem),
	fFSVolume(fsVolume),
	fUserlandVolume(NULL),
	fRootID(0),
	fRootNode(NULL),
	fOpenFiles(0),
	fOpenDirectories(0),
	fOpenAttributeDirectories(0),
	fOpenAttributes(0),
	fOpenIndexDirectories(0),
	fOpenQueries(0),
	fVNodes(NULL),
	fIORequestInfosByID(NULL),
	fIORequestInfosByStruct(NULL),
	fLastIORequestID(0),
	fVNodeCountingEnabled(false)
{
	mutex_init(&fLock, "userlandfs volume");
}


// destructor
Volume::~Volume()
{
	mutex_destroy(&fLock);

	delete fIORequestInfosByID;
	delete fIORequestInfosByStruct;
	delete fVNodes;
}


// #pragma mark - client methods

// GetVNode
status_t
Volume::GetVNode(ino_t vnid, void** _node)
{
	PRINT(("get_vnode(%" B_PRId32 ", %" B_PRId64 ")\n", GetID(), vnid));
	void* vnode;
	status_t error = get_vnode(fFSVolume, vnid, &vnode);
	if (error == B_OK) {
		_IncrementVNodeCount(vnid);
		*_node = ((VNode*)vnode)->clientNode;
	}

	return error;
}

// PutVNode
status_t
Volume::PutVNode(ino_t vnid)
{
	PRINT(("put_vnode(%" B_PRId32 ", %" B_PRId64 ")\n", GetID(), vnid));
	// Decrement the count first. We might not have another chance, since
	// put_vnode() could put the last reference, thus causing the node to be
	// removed from our map. This is all not very dramatic, but this way we
	// avoid an erroneous error message from _DecrementVNodeCount().
	_DecrementVNodeCount(vnid);

	return put_vnode(fFSVolume, vnid);
}


// AcquireVNode
status_t
Volume::AcquireVNode(ino_t vnid)
{
	PRINT(("acquire_vnode(%" B_PRId32 ", %" B_PRId64 ")\n", GetID(), vnid));
	status_t error = acquire_vnode(fFSVolume, vnid);
	if (error == B_OK)
		_IncrementVNodeCount(vnid);
	return error;
}


// NewVNode
status_t
Volume::NewVNode(ino_t vnid, void* clientNode,
	const FSVNodeCapabilities& capabilities)
{
	PRINT(("new_vnode(%" B_PRId32 ", %" B_PRId64 ")\n", GetID(), vnid));
	// lookup the node
	MutexLocker locker(fLock);
	VNode* node = fVNodes->Lookup(vnid);
	if (node != NULL) {
		// The node is already known -- this is an error.
		RETURN_ERROR(B_BAD_VALUE);
	}

	// get the ops vector for the node
	VNodeOps* ops = fFileSystem->GetVNodeOps(capabilities);
	if (ops == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	// create the node
	node = new(std::nothrow) VNode(vnid, clientNode, ops);
	if (node == NULL) {
		fFileSystem->PutVNodeOps(ops);
		RETURN_ERROR(B_NO_MEMORY);
	}

	node->published = false;

	locker.Unlock();

	// tell the VFS
	status_t error = new_vnode(fFSVolume, vnid, node, node->ops->ops);
	if (error != B_OK) {
		node->Delete(this);
		RETURN_ERROR(error);
	}

	// add the node to our map
	locker.Lock();
	fVNodes->Insert(node);

	// Increment its use count. After new_vnode() the caller has a reference,
	// but a subsequent publish_vnode() won't get another one. We handle that
	// there.
	if (fVNodeCountingEnabled)
		node->useCount++;

	return B_OK;
}

// PublishVNode
status_t
Volume::PublishVNode(ino_t vnid, void* clientNode, int type, uint32 flags,
	const FSVNodeCapabilities& capabilities)
{
	PRINT(("publish_vnode(%" B_PRId32 ", %" B_PRId64 ", %p)\n", GetID(), vnid,
		clientNode));

	// lookup the node
	MutexLocker locker(fLock);
	VNode* node = fVNodes->Lookup(vnid);
	bool nodeKnown = node != NULL;

	if (nodeKnown) {
		if (node->published) {
			WARN(("publish_vnode(): vnode (%" B_PRId32 ", %" B_PRId64
				") already published!\n", GetID(), vnid));
			RETURN_ERROR(B_BAD_VALUE);
		}
	} else if (!nodeKnown) {
		// The node is not yet known -- create it.

		// get the ops vector for the node
		VNodeOps* ops = fFileSystem->GetVNodeOps(capabilities);
		if (ops == NULL)
			RETURN_ERROR(B_NO_MEMORY);

		// create the node
		node = new(std::nothrow) VNode(vnid, clientNode, ops);
		if (node == NULL) {
			fFileSystem->PutVNodeOps(ops);
			RETURN_ERROR(B_NO_MEMORY);
		}
	}

	locker.Unlock();

	// tell the VFS
	status_t error = publish_vnode(fFSVolume, vnid, node, node->ops->ops,
		type, flags);
	if (error != B_OK) {
		if (nodeKnown) {
			// The node was known, i.e. it had been made known via new_vnode()
			// and thus already had a use count of 1. Decrement that use count
			// and remove the node completely.
			_DecrementVNodeCount(vnid);
			_RemoveInvalidVNode(vnid);
		} else
			node->Delete(this);
		RETURN_ERROR(error);
	}

	// add the node to our map, if not known yet
	locker.Lock();
	if (nodeKnown) {
		// The node is now published. Don't increment its use count. It already
		// has 1 from new_vnode() and this publish_vnode() didn't increment it.
		node->published = true;
	} else {
		// New node: increment its use count and add it to the map.
		if (fVNodeCountingEnabled)
			node->useCount++;
		fVNodes->Insert(node);
	}

	return B_OK;
}

// RemoveVNode
status_t
Volume::RemoveVNode(ino_t vnid)
{
	PRINT(("remove_vnode(%" B_PRId32 ", %" B_PRId64 ")\n", GetID(), vnid));
	return remove_vnode(fFSVolume, vnid);
}

// UnremoveVNode
status_t
Volume::UnremoveVNode(ino_t vnid)
{
	PRINT(("unremove_vnode(%" B_PRId32 ", %" B_PRId64 ")\n", GetID(), vnid));
	return unremove_vnode(fFSVolume, vnid);
}

// GetVNodeRemoved
status_t
Volume::GetVNodeRemoved(ino_t vnid, bool* removed)
{
	PRINT(("get_vnode_removed(%" B_PRId32 ", %" B_PRId64 ", %p)\n", GetID(),
		vnid, removed));
	return get_vnode_removed(fFSVolume, vnid, removed);
}


// CreateFileCache
status_t
Volume::CreateFileCache(ino_t vnodeID, off_t size)
{
	// lookup the node
	MutexLocker locker(fLock);
	VNode* vnode = fVNodes->Lookup(vnodeID);
	if (vnode == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	// does the node already have a file cache?
	if (vnode->fileCache != NULL)
		RETURN_ERROR(B_BAD_VALUE);

	// create the file cache
	locker.Unlock();
	void* fileCache = file_cache_create(GetID(), vnodeID, size);
	locker.Lock();

	// re-check whether the node still lives
	vnode = fVNodes->Lookup(vnodeID);
	if (vnode == NULL) {
		file_cache_delete(fileCache);
		RETURN_ERROR(B_BAD_VALUE);
	}

	vnode->fileCache = fileCache;

	return B_OK;
}


// DeleteFileCache
status_t
Volume::DeleteFileCache(ino_t vnodeID)
{
	// lookup the node
	MutexLocker locker(fLock);
	VNode* vnode = fVNodes->Lookup(vnodeID);
	if (vnode == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	// does the node have a file cache
	if (vnode->fileCache == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	void* fileCache = vnode->fileCache;
	vnode->fileCache = NULL;

	locker.Unlock();

	file_cache_delete(fileCache);

	return B_OK;
}


// SetFileCacheEnabled
status_t
Volume::SetFileCacheEnabled(ino_t vnodeID, bool enabled)
{
	// lookup the node
	MutexLocker locker(fLock);
	VNode* vnode = fVNodes->Lookup(vnodeID);
	if (vnode == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	// does the node have a file cache
	if (vnode->fileCache == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	void* fileCache = vnode->fileCache;
	locker.Unlock();
// TODO: We should use some kind of ref counting to avoid that another thread
// deletes the file cache now that we have dropped the lock. Applies to the
// other file cache operations as well.

	// enable/disable the file cache
	if (enabled) {
		file_cache_enable(fileCache);
		return B_OK;
	}

	return file_cache_disable(fileCache);
}


// SetFileCacheSize
status_t
Volume::SetFileCacheSize(ino_t vnodeID, off_t size)
{
	// lookup the node
	MutexLocker locker(fLock);
	VNode* vnode = fVNodes->Lookup(vnodeID);
	if (vnode == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	// does the node have a file cache
	if (vnode->fileCache == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	void* fileCache = vnode->fileCache;
	locker.Unlock();

	// set the size
	return file_cache_set_size(fileCache, size);
}


// SyncFileCache
status_t
Volume::SyncFileCache(ino_t vnodeID)
{
	// lookup the node
	MutexLocker locker(fLock);
	VNode* vnode = fVNodes->Lookup(vnodeID);
	if (vnode == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	// does the node have a file cache
	if (vnode->fileCache == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	void* fileCache = vnode->fileCache;
	locker.Unlock();

	// sync
	return file_cache_sync(fileCache);
}


// ReadFileCache
status_t
Volume::ReadFileCache(ino_t vnodeID, void* cookie,
	off_t offset, void* buffer, size_t* _size)
{
	// lookup the node
	MutexLocker locker(fLock);
	VNode* vnode = fVNodes->Lookup(vnodeID);
	if (vnode == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	// does the node have a file cache
	if (vnode->fileCache == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	void* fileCache = vnode->fileCache;
	locker.Unlock();

	// read
	return file_cache_read(fileCache, cookie, offset, buffer, _size);
}


// WriteFileCache
status_t
Volume::WriteFileCache(ino_t vnodeID, void* cookie,
	off_t offset, const void* buffer, size_t* _size)
{
	// lookup the node
	MutexLocker locker(fLock);
	VNode* vnode = fVNodes->Lookup(vnodeID);
	if (vnode == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	// does the node have a file cache
	if (vnode->fileCache == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	void* fileCache = vnode->fileCache;
	locker.Unlock();

	// read
	return file_cache_write(fileCache, cookie, offset, buffer, _size);
}


status_t
Volume::ReadFromIORequest(int32 requestID, void* buffer, size_t size)
{
	// get the request
	io_request* request;
	status_t error = _FindIORequest(requestID, &request);
	if (error != B_OK)
		RETURN_ERROR(error);

	return read_from_io_request(request, buffer, size);
}


status_t
Volume::WriteToIORequest(int32 requestID, const void* buffer, size_t size)
{
	// get the request
	io_request* request;
	status_t error = _FindIORequest(requestID, &request);
	if (error != B_OK)
		RETURN_ERROR(error);

	return write_to_io_request(request, buffer, size);
}


// DoIterativeFDIO
status_t
Volume::DoIterativeFDIO(int fd, int32 requestID, void* clientCookie,
	const file_io_vec* vecs, uint32 vecCount)
{
	// get the request
	io_request* request;
	status_t error = _FindIORequest(requestID, &request);
	if (error != B_OK)
		RETURN_ERROR(error);

	// copy the FD into the kernel
	fd = dup_foreign_fd(fFileSystem->GetTeam(), fd, true);
	if (fd < 0)
		RETURN_ERROR(fd);

	// create a cookie
	IterativeFDIOCookie* cookie = new(std::nothrow) IterativeFDIOCookie(
		this, fd, requestID, clientCookie, request->Offset(), vecs, vecCount);
	if (cookie == NULL) {
		close(fd);
		RETURN_ERROR(B_NO_MEMORY);
	}

	// we need another reference, so we can still access the cookie below
	cookie->AcquireReference();

// TODO: Up to this point we're responsible for calling the finished hook on
// error!
	// call the kernel function
	error = do_iterative_fd_io(fd, request, &_IterativeFDIOGetVecs,
		&_IterativeFDIOFinished, cookie);

	// unset the vecs -- they are on the stack an will become invalid when we
	// return
	MutexLocker _(fLock);
	cookie->vecs = NULL;
	cookie->vecCount = 0;
	cookie->ReleaseReference();

	return error;
}


status_t
Volume::NotifyIORequest(int32 requestID, status_t status)
{
	// get the request
	io_request* request;
	status_t error = _FindIORequest(requestID, &request);
	if (error != B_OK)
		RETURN_ERROR(error);

	notify_io_request(request, status);
	return B_OK;
}


// #pragma mark - FS


// Mount
status_t
Volume::Mount(const char* device, uint32 flags, const char* parameters)
{
	// create the maps
	fVNodes = new(std::nothrow) VNodeMap;
	fIORequestInfosByID = new(std::nothrow) IORequestIDMap;
	fIORequestInfosByStruct = new(std::nothrow) IORequestStructMap;

	if (fVNodes == NULL || fIORequestInfosByID == NULL
		|| fIORequestInfosByStruct == NULL
		|| fVNodes->Init() != B_OK
		|| fIORequestInfosByID->Init() != B_OK
		|| fIORequestInfosByStruct->Init() != B_OK) {
		return B_NO_MEMORY;
	}

	// enable vnode counting
	fVNodeCountingEnabled = true;

	// init IORequest ID's
	fLastIORequestID = 0;

	// mount
	status_t error = _Mount(device, flags, parameters);
	if (error != B_OK)
		RETURN_ERROR(error);

	MutexLocker locker(fLock);
	// fetch the root node, so that we can serve Walk() requests on it,
	// after the connection to the userland server is gone
	fRootNode = fVNodes->Lookup(fRootID);
	if (fRootNode == NULL) {
		// The root node was not added while mounting. That's a serious
		// problem -- not only because we don't have it, but also because
		// the VFS requires publish_vnode() to be invoked for the root node.
		ERROR(("Volume::Mount(): new_vnode() was not called for root node! "
			"Unmounting...\n"));
		locker.Unlock();
		Unmount();
		return B_ERROR;
	}

	// Decrement the root node use count. The publish_vnode() the client FS
	// did will be balanced by the VFS.
	if (fVNodeCountingEnabled)
		fRootNode->useCount--;

	// init the volume ops vector we'll give the VFS
	_InitVolumeOps();

	return B_OK;
}

// Unmount
status_t
Volume::Unmount()
{
	status_t error = _Unmount();

	// free the memory associated with the maps
	{
		// vnodes
		MutexLocker _(fLock);
		if (fVNodes != NULL) {
			VNode* node = fVNodes->Clear(true);
			while (node != NULL) {
				VNode* nextNode = node->hash_link;
				node->Delete(this);
				node = nextNode;
			}
			delete fVNodes;
			fVNodes = NULL;
		}

		// io request infos
		if (fIORequestInfosByID != NULL) {
			fIORequestInfosByID->Clear();
			delete fIORequestInfosByID;
			fIORequestInfosByID = NULL;
		}

		if (fIORequestInfosByStruct != NULL) {
			IORequestInfo* info = fIORequestInfosByStruct->Clear(true);
			while (info != NULL) {
				IORequestInfo* nextInfo = info->structLink;
				delete info;
				// TODO: We should probably also notify the request, if that
				// hasn't happened yet.
				info = nextInfo;
			}
			delete fIORequestInfosByStruct;
			fIORequestInfosByStruct = NULL;
		}
	}

	fFileSystem->VolumeUnmounted(this);
	return error;
}

// Sync
status_t
Volume::Sync()
{
	// check capability
	if (!HasCapability(FS_VOLUME_CAPABILITY_SYNC))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	SyncVolumeRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;

	// send the request
	KernelRequestHandler handler(this, SYNC_VOLUME_REPLY);
	SyncVolumeReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// ReadFSInfo
status_t
Volume::ReadFSInfo(fs_info* info)
{
	// When the connection to the userland server is lost, we serve
	// read_fs_info() requests manually.
	status_t error = _ReadFSInfo(info);
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		WARN(("Volume::ReadFSInfo(): connection lost, emulating lookup `.'\n"));

		info->flags = B_FS_IS_PERSISTENT | B_FS_IS_READONLY;
		info->block_size = 512;
		info->io_size = 512;
		info->total_blocks = 0;
		info->free_blocks = 0;
		strlcpy(info->volume_name, fFileSystem->GetName(),
			sizeof(info->volume_name));
		strlcat(info->volume_name, ":disconnected", sizeof(info->volume_name));

		error = B_OK;
	}
	return error;
}

// WriteFSInfo
status_t
Volume::WriteFSInfo(const struct fs_info *info, uint32 mask)
{
	// check capability
	if (!HasCapability(FS_VOLUME_CAPABILITY_WRITE_FS_INFO))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	WriteFSInfoRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->info = *info;
	request->mask = mask;

	// send the request
	KernelRequestHandler handler(this, WRITE_FS_INFO_REPLY);
	WriteFSInfoReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}


// #pragma mark - vnodes


// Lookup
status_t
Volume::Lookup(void* dir, const char* entryName, ino_t* vnid)
{
	// When the connection to the userland server is lost, we serve
	// lookup(fRootNode, `.') requests manually to allow clean unmounting.
	status_t error = _Lookup(dir, entryName, vnid);
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()
		&& dir == fRootNode && strcmp(entryName, ".") == 0) {
		WARN(("Volume::Lookup(): connection lost, emulating lookup `.'\n"));
		void* entryNode;
		if (GetVNode(fRootID, &entryNode) != B_OK)
			RETURN_ERROR(B_BAD_VALUE);
		*vnid = fRootID;
		// The VFS will balance the get_vnode() call for the FS.
		_DecrementVNodeCount(*vnid);
		return B_OK;
	}
	return error;
}

// GetVNodeName
status_t
Volume::GetVNodeName(void* _node, char* buffer, size_t bufferSize)
{
	// We don't check the capability -- if not implemented by the client FS,
	// the functionality is emulated in userland.

	VNode* vnode = (VNode*)_node;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	GetVNodeNameRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->size = bufferSize;

	// send the request
	KernelRequestHandler handler(this, GET_VNODE_NAME_REPLY);
	GetVNodeNameReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;

	char* readBuffer = (char*)reply->buffer.GetData();
	size_t nameLen = reply->buffer.GetSize();
	nameLen = strnlen(readBuffer, nameLen);
	if (nameLen <= 1 || nameLen >= bufferSize)
		RETURN_ERROR(B_BAD_DATA);

	memcpy(buffer, readBuffer, nameLen);
	buffer[nameLen] = '\0';

	_SendReceiptAck(port);
	return error;
}

// ReadVNode
status_t
Volume::ReadVNode(ino_t vnid, bool reenter, void** _node, fs_vnode_ops** _ops,
	int* type, uint32* flags)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadVNodeRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->vnid = vnid;
	request->reenter = reenter;

	// add the uninitialized node to our map
	VNode* vnode = new(std::nothrow) VNode(vnid, NULL, NULL);
	if (vnode == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	vnode->valid = false;

	MutexLocker locker(fLock);
	fVNodes->Insert(vnode);
	locker.Unlock();

	// send the request
	KernelRequestHandler handler(this, READ_VNODE_REPLY);
	ReadVNodeReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK) {
		_RemoveInvalidVNode(vnid);
		return error;
	}
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK) {
		_RemoveInvalidVNode(vnid);
		return reply->error;
	}

	// get the ops vector for the node
	VNodeOps* ops = fFileSystem->GetVNodeOps(reply->capabilities);
	if (ops == NULL) {
		_RemoveInvalidVNode(vnid);
		RETURN_ERROR(B_NO_MEMORY);
	}

	// everything went fine -- mark the node valid
	locker.Lock();
	vnode->clientNode = reply->node;
	vnode->ops = ops;
	vnode->valid = true;

	*_node = vnode;
	*type = reply->type;
	*flags = reply->flags;
	*_ops = ops->ops;
	return B_OK;
}

// WriteVNode
status_t
Volume::WriteVNode(void* node, bool reenter)
{
	status_t error = _WriteVNode(node, reenter);
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, since the VFS basically ignores the
		// return value -- at least Haiku. The fshell panic()s; didn't check
		// BeOS. It doesn't harm to appear to behave nicely. :-)
		WARN(("Volume::WriteVNode(): connection lost, forcing write vnode\n"));
		return B_OK;
	}
	return error;
}

// RemoveVNode
status_t
Volume::RemoveVNode(void* _node, bool reenter)
{
	VNode* vnode = (VNode*)_node;

	// At any rate remove the vnode from our map and delete it. We don't do that
	// right now, though, since we might still need to serve file cache requests
	// from the client FS.
	VNodeRemover nodeRemover(this, vnode);

	void* clientNode = vnode->clientNode;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FSRemoveVNodeRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = clientNode;
	request->reenter = reenter;

	// send the request
	KernelRequestHandler handler(this, FS_REMOVE_VNODE_REPLY);
	FSRemoveVNodeReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}


// #pragma mark - asynchronous I/O


// DoIO
status_t
Volume::DoIO(void* _node, void* cookie, io_request* ioRequest)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_IO))
		return B_UNSUPPORTED;

	// register the IO request
	int32 requestID;
	status_t error = _RegisterIORequest(ioRequest, &requestID);
	if (error != B_OK) {
		notify_io_request(ioRequest, error);
		return error;
	}

	IORequestRemover requestRemover(this, requestID);

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port) {
		notify_io_request(ioRequest, B_ERROR);
		return B_ERROR;
	}
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	DoIORequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK) {
		notify_io_request(ioRequest, error);
		return error;
	}

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->fileCookie = cookie;
	request->request = requestID;
	request->offset = ioRequest->Offset();
	request->length = ioRequest->Length();
	request->isWrite = ioRequest->IsWrite();
	request->isVIP = (ioRequest->Flags() & B_VIP_IO_REQUEST) != 0;

	// send the request
	KernelRequestHandler handler(this, DO_IO_REPLY);
	DoIOReply* reply;

	// TODO: when to notify the io_request?
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;

	requestRemover.Detach();

	return B_OK;
}


// CancelIO
status_t
Volume::CancelIO(void* _node, void* cookie, io_request* ioRequest)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_CANCEL_IO))
		return B_BAD_VALUE;

	// find the request
	int32 requestID;
	status_t error = _FindIORequest(ioRequest, &requestID);
	if (error != B_OK)
		return error;

	IORequestRemover requestRemover(this, requestID);

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	CancelIORequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->fileCookie = cookie;
	request->request = requestID;

	// send the request
	KernelRequestHandler handler(this, CANCEL_IO_REPLY);
	CancelIOReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK) {
		_UnregisterIORequest(requestID);
		return error;
	}
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK) {
		_UnregisterIORequest(requestID);
		return reply->error;
	}

	return B_OK;
}


// #pragma mark - nodes


// IOCtl
status_t
Volume::IOCtl(void* _node, void* cookie, uint32 command, void *buffer,
	size_t len)
{
	VNode* vnode = (VNode*)_node;

	// check the command and its parameters
	bool isBuffer = false;
	int32 bufferSize = 0;
	int32 writeSize = 0;
	switch (command) {
		case IOCTL_FILE_UNCACHED_IO:
			buffer = NULL;
			break;
		case IOCTL_CREATE_TIME:
		case IOCTL_MODIFIED_TIME:
			isBuffer = 0;
			bufferSize = 0;
			writeSize = sizeof(bigtime_t);
			break;
		case USERLANDFS_IOCTL:
			area_id area;
			area_info info;
			PRINT(("Volume::IOCtl(): USERLANDFS_IOCTL\n"));
			if ((area = area_for(buffer)) >= 0) {
				if (get_area_info(area, &info) == B_OK) {
					if ((uint8*)buffer - (uint8*)info.address
							+ sizeof(userlandfs_ioctl) <= info.size) {
						if (strncmp(((userlandfs_ioctl*)buffer)->magic,
								kUserlandFSIOCtlMagic,
								USERLAND_IOCTL_MAGIC_LENGTH) == 0) {
							return _InternalIOCtl((userlandfs_ioctl*)buffer,
								bufferSize);
						} else
							PRINT(("Volume::IOCtl(): bad magic\n"));
					} else
						PRINT(("Volume::IOCtl(): bad buffer size\n"));
				} else
					PRINT(("Volume::IOCtl(): failed to get area info\n"));
			} else
				PRINT(("Volume::IOCtl(): bad area\n"));
			// fall through...
		default:
		{
			// We don't know the command. Check whether the FileSystem knows
			// about it.
			const IOCtlInfo* info = fFileSystem->GetIOCtlInfo(command);
			if (!info) {
				PRINT(("Volume::IOCtl(): unknown command\n"));
				return B_BAD_VALUE;
			}

			isBuffer = info->isBuffer;
			bufferSize = info->bufferSize;
			writeSize = info->writeBufferSize;

			// If the buffer shall indeed specify a buffer, check it.
			if (info->isBuffer) {
				if (!buffer) {
					PRINT(("Volume::IOCtl(): buffer is NULL\n"));
					return B_BAD_VALUE;
				}

				area_id area = area_for(buffer);
				if (area < 0) {
					PRINT(("Volume::IOCtl(): bad area\n"));
					return B_BAD_VALUE;
				}

				area_info info;
				if (get_area_info(area, &info) != B_OK) {
					PRINT(("Volume::IOCtl(): failed to get area info\n"));
					return B_BAD_VALUE;
				}

				int32 areaSize = info.size - ((uint8*)buffer
					- (uint8*)info.address);
				if (bufferSize > areaSize || writeSize > areaSize) {
					PRINT(("Volume::IOCtl(): bad buffer size\n"));
					return B_BAD_VALUE;
				}

				if (writeSize > 0 && !(info.protection & B_WRITE_AREA)) {
					PRINT(("Volume::IOCtl(): buffer not writable\n"));
					return B_BAD_VALUE;
				}
			}
			break;
		}
	}

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_IOCTL))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	IOCtlRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->fileCookie = cookie;
	request->command = command;
	request->bufferParameter = buffer;
	request->isBuffer = isBuffer;
	request->lenParameter = len;
	request->writeSize = writeSize;

	if (isBuffer && bufferSize > 0) {
		error = allocator.AllocateData(request->buffer, buffer, bufferSize, 8);
		if (error != B_OK)
			return error;
	}

	// send the request
	KernelRequestHandler handler(this, IOCTL_REPLY);
	IOCtlReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;

	// Copy back the buffer even if the result is not B_OK. The protocol
	// is defined by the FS developer and may include writing data into
	// the buffer in some error cases.
	if (isBuffer && writeSize > 0 && reply->buffer.GetData()) {
		if (writeSize > reply->buffer.GetSize())
			writeSize = reply->buffer.GetSize();
		memcpy(buffer, reply->buffer.GetData(), writeSize);
		_SendReceiptAck(port);
	}
	return reply->ioctlError;
}

// SetFlags
status_t
Volume::SetFlags(void* _node, void* cookie, int flags)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_SET_FLAGS))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	SetFlagsRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->fileCookie = cookie;
	request->flags = flags;

	// send the request
	KernelRequestHandler handler(this, SET_FLAGS_REPLY);
	SetFlagsReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// Select
status_t
Volume::Select(void* _node, void* cookie, uint8 event, selectsync* sync)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_SELECT)) {
		notify_select_event(sync, event);
		return B_OK;
	}

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	SelectRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->fileCookie = cookie;
	request->event = event;
	request->sync = sync;

	// add a selectsync entry
	error = fFileSystem->AddSelectSyncEntry(sync);
	if (error != B_OK)
		return error;

	// send the request
	KernelRequestHandler handler(this, SELECT_REPLY);
	SelectReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK) {
		fFileSystem->RemoveSelectSyncEntry(sync);
		return error;
	}
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK) {
		fFileSystem->RemoveSelectSyncEntry(sync);
		return reply->error;
	}
	return error;
}

// Deselect
status_t
Volume::Deselect(void* _node, void* cookie, uint8 event, selectsync* sync)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_DESELECT))
		return B_OK;

	struct SyncRemover {
		SyncRemover(FileSystem* fs, selectsync* sync)
			: fs(fs), sync(sync) {}
		~SyncRemover() { fs->RemoveSelectSyncEntry(sync); }

		FileSystem*	fs;
		selectsync*	sync;
	} syncRemover(fFileSystem, sync);

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	DeselectRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->fileCookie = cookie;
	request->event = event;
	request->sync = sync;

	// send the request
	KernelRequestHandler handler(this, DESELECT_REPLY);
	DeselectReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// FSync
status_t
Volume::FSync(void* _node)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_FSYNC))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FSyncRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;

	// send the request
	KernelRequestHandler handler(this, FSYNC_REPLY);
	FSyncReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// ReadSymlink
status_t
Volume::ReadSymlink(void* _node, char* buffer, size_t bufferSize,
	size_t* bytesRead)
{
	VNode* vnode = (VNode*)_node;

	*bytesRead = 0;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_READ_SYMLINK))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadSymlinkRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->size = bufferSize;

	// send the request
	KernelRequestHandler handler(this, READ_SYMLINK_REPLY);
	ReadSymlinkReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	void* readBuffer = reply->buffer.GetData();
	if (reply->bytesRead > (uint32)reply->buffer.GetSize()
		|| reply->bytesRead > bufferSize) {
		return B_BAD_DATA;
	}
	if (reply->bytesRead > 0)
		memcpy(buffer, readBuffer, reply->bytesRead);
	*bytesRead = reply->bytesRead;
	_SendReceiptAck(port);
	return error;
}

// CreateSymlink
status_t
Volume::CreateSymlink(void* _dir, const char* name, const char* target,
	int mode)
{
	VNode* vnode = (VNode*)_dir;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_CREATE_SYMLINK))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	CreateSymlinkRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	error = allocator.AllocateString(request->name, name);
	if (error == B_OK)
		error = allocator.AllocateString(request->target, target);
	if (error != B_OK)
		return error;
	request->mode = mode;

	// send the request
	KernelRequestHandler handler(this, CREATE_SYMLINK_REPLY);
	CreateSymlinkReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// Link
status_t
Volume::Link(void* _dir, const char* name, void* node)
{
	VNode* vnode = (VNode*)_dir;
	VNode* targetVnode = (VNode*)node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_LINK))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	LinkRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	error = allocator.AllocateString(request->name, name);
	request->target = targetVnode->clientNode;
	if (error != B_OK)
		return error;

	// send the request
	KernelRequestHandler handler(this, LINK_REPLY);
	LinkReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// Unlink
status_t
Volume::Unlink(void* _dir, const char* name)
{
	VNode* vnode = (VNode*)_dir;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_UNLINK))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	UnlinkRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	error = allocator.AllocateString(request->name, name);
	if (error != B_OK)
		return error;

	// send the request
	KernelRequestHandler handler(this, UNLINK_REPLY);
	UnlinkReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// Rename
status_t
Volume::Rename(void* _oldDir, const char* oldName, void* _newDir,
	const char* newName)
{
	VNode* oldVNode = (VNode*)_oldDir;
	VNode* newVNode = (VNode*)_newDir;

	// check capability
	if (!HasVNodeCapability(oldVNode, FS_VNODE_CAPABILITY_RENAME))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	RenameRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->oldDir = oldVNode->clientNode;
	request->newDir = newVNode->clientNode;
	error = allocator.AllocateString(request->oldName, oldName);
	if (error == B_OK)
		error = allocator.AllocateString(request->newName, newName);
	if (error != B_OK)
		return error;

	// send the request
	KernelRequestHandler handler(this, RENAME_REPLY);
	RenameReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// Access
status_t
Volume::Access(void* _node, int mode)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_ACCESS))
		return B_OK;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	AccessRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->mode = mode;

	// send the request
	KernelRequestHandler handler(this, ACCESS_REPLY);
	AccessReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// ReadStat
status_t
Volume::ReadStat(void* node, struct stat* st)
{
	// When the connection to the userland server is lost, we serve
	// read_stat(fRootNode) requests manually to allow clean unmounting.
	status_t error = _ReadStat(node, st);
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()
		&& node == fRootNode) {
		WARN(("Volume::ReadStat(): connection lost, emulating stat for the "
			"root node\n"));

		st->st_dev = GetID();
		st->st_ino = fRootID;
		st->st_mode = ACCESSPERMS;
		st->st_nlink = 1;
		st->st_uid = 0;
		st->st_gid = 0;
		st->st_size = 512;
		st->st_blksize = 512;
		st->st_atime = 0;
		st->st_mtime = 0;
		st->st_ctime = 0;
		st->st_crtime = 0;

		error = B_OK;
	}
	return error;
}

// WriteStat
status_t
Volume::WriteStat(void* _node, const struct stat* st, uint32 mask)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_WRITE_STAT))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	WriteStatRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->st = *st;
	request->mask = mask;

	// send the request
	KernelRequestHandler handler(this, WRITE_STAT_REPLY);
	WriteStatReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}


// #pragma mark - files

// Create
status_t
Volume::Create(void* _dir, const char* name, int openMode, int mode,
	void** cookie, ino_t* vnid)
{
	VNode* vnode = (VNode*)_dir;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_CREATE))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	AutoIncrementer incrementer(&fOpenFiles);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	CreateRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	error = allocator.AllocateString(request->name, name);
	request->openMode = openMode;
	request->mode = mode;
	if (error != B_OK)
		return error;

	// send the request
	KernelRequestHandler handler(this, CREATE_REPLY);
	CreateReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	incrementer.Keep();
	*vnid = reply->vnid;
	*cookie = reply->fileCookie;

	// The VFS will balance the publish_vnode() call for the FS.
	if (error == B_OK)
		_DecrementVNodeCount(*vnid);
	return error;
}

// Open
status_t
Volume::Open(void* _node, int openMode, void** cookie)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_OPEN))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	AutoIncrementer incrementer(&fOpenFiles);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	OpenRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->openMode = openMode;

	// send the request
	KernelRequestHandler handler(this, OPEN_REPLY);
	OpenReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	incrementer.Keep();
	*cookie = reply->fileCookie;
	return error;
}

// Close
status_t
Volume::Close(void* node, void* cookie)
{
	status_t error = _Close(node, cookie);
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. Haiku ignores it completely. The fsshell returns it to the
		// userland, but considers the node closed anyway.
		WARN(("Volume::Close(): connection lost, forcing close\n"));
		return B_OK;
	}
	return error;
}

// FreeCookie
status_t
Volume::FreeCookie(void* node, void* cookie)
{
	status_t error = _FreeCookie(node, cookie);
	bool disconnected = false;
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. It's completely ignored by Haiku as well as by the fsshell.
		WARN(("Volume::FreeCookie(): connection lost, forcing free cookie\n"));
		error = B_OK;
		disconnected = true;
	}

	int32 openFiles = atomic_add(&fOpenFiles, -1);
	if (openFiles <= 1 && disconnected)
		_PutAllPendingVNodes();
	return error;
}

// Read
status_t
Volume::Read(void* _node, void* cookie, off_t pos, void* buffer,
	size_t bufferSize, size_t* bytesRead)
{
	VNode* vnode = (VNode*)_node;

	*bytesRead = 0;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_READ))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->fileCookie = cookie;
	request->pos = pos;
	request->size = bufferSize;

	// send the request
	KernelRequestHandler handler(this, READ_REPLY);
	ReadReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	void* readBuffer = reply->buffer.GetData();
	if (reply->bytesRead > (uint32)reply->buffer.GetSize()
		|| reply->bytesRead > bufferSize) {
		return B_BAD_DATA;
	}
	if (reply->bytesRead > 0
		&& user_memcpy(buffer, readBuffer, reply->bytesRead) < B_OK) {
		return B_BAD_ADDRESS;
	}

	*bytesRead = reply->bytesRead;
	_SendReceiptAck(port);
	return error;
}

// Write
status_t
Volume::Write(void* _node, void* cookie, off_t pos, const void* buffer,
	size_t size, size_t* bytesWritten)
{
	VNode* vnode = (VNode*)_node;

	*bytesWritten = 0;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_WRITE))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	WriteRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->fileCookie = cookie;
	request->pos = pos;
	error = allocator.AllocateData(request->buffer, buffer, size, 1);
	if (error != B_OK)
		return error;

	// send the request
	KernelRequestHandler handler(this, WRITE_REPLY);
	WriteReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	*bytesWritten = reply->bytesWritten;
	return error;
}


// #pragma mark - directories

// CreateDir
status_t
Volume::CreateDir(void* _dir, const char* name, int mode)
{
	VNode* vnode = (VNode*)_dir;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_CREATE_DIR))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	CreateDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	error = allocator.AllocateString(request->name, name);
	request->mode = mode;
	if (error != B_OK)
		return error;

	// send the request
	KernelRequestHandler handler(this, CREATE_DIR_REPLY);
	CreateDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// RemoveDir
status_t
Volume::RemoveDir(void* _dir, const char* name)
{
	VNode* vnode = (VNode*)_dir;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_REMOVE_DIR))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	RemoveDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	error = allocator.AllocateString(request->name, name);
	if (error != B_OK)
		return error;

	// send the request
	KernelRequestHandler handler(this, REMOVE_DIR_REPLY);
	RemoveDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// OpenDir
status_t
Volume::OpenDir(void* _node, void** cookie)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_OPEN_DIR))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	AutoIncrementer incrementer(&fOpenDirectories);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	OpenDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;

	// send the request
	KernelRequestHandler handler(this, OPEN_DIR_REPLY);
	OpenDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	incrementer.Keep();
	*cookie = reply->dirCookie;
	return error;
}

// CloseDir
status_t
Volume::CloseDir(void* node, void* cookie)
{
	status_t error = _CloseDir(node, cookie);
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. Haiku ignores it completely. The fsshell returns it to the
		// userland, but considers the node closed anyway.
		WARN(("Volume::CloseDir(): connection lost, forcing close dir\n"));
		return B_OK;
	}
	return error;
}

// FreeDirCookie
status_t
Volume::FreeDirCookie(void* node, void* cookie)
{
	status_t error = _FreeDirCookie(node, cookie);
	bool disconnected = false;
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. It's completely ignored by Haiku as well as by the fsshell.
		WARN(("Volume::FreeDirCookie(): connection lost, forcing free dir "
			"cookie\n"));
		error = B_OK;
		disconnected = true;
	}
	int32 openDirs = atomic_add(&fOpenDirectories, -1);
	if (openDirs <= 1 && disconnected)
		_PutAllPendingVNodes();
	return error;
}

// ReadDir
status_t
Volume::ReadDir(void* _node, void* cookie, void* buffer, size_t bufferSize,
	uint32 count, uint32* countRead)
{
	VNode* vnode = (VNode*)_node;

	*countRead = 0;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_READ_DIR))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->dirCookie = cookie;
	request->bufferSize = bufferSize;
	request->count = count;

	// send the request
	KernelRequestHandler handler(this, READ_DIR_REPLY);
	ReadDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	if (reply->count < 0 || reply->count > count)
		return B_BAD_DATA;
	if ((int32)bufferSize < reply->buffer.GetSize())
		return B_BAD_DATA;

	PRINT(("Volume::ReadDir(): buffer returned: %" B_PRId32 " bytes\n",
		reply->buffer.GetSize()));

	*countRead = reply->count;
	if (*countRead > 0) {
		// copy the buffer -- limit the number of bytes to copy
		uint32 maxBytes = *countRead
			* (sizeof(struct dirent) + B_FILE_NAME_LENGTH);
		uint32 copyBytes = reply->buffer.GetSize();
		if (copyBytes > maxBytes)
			copyBytes = maxBytes;
		memcpy(buffer, reply->buffer.GetData(), copyBytes);
	}
	_SendReceiptAck(port);
	return error;
}

// RewindDir
status_t
Volume::RewindDir(void* _node, void* cookie)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_REWIND_DIR))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	RewindDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->dirCookie = cookie;

	// send the request
	KernelRequestHandler handler(this, REWIND_DIR_REPLY);
	RewindDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}


// #pragma mark - attribute directories


// OpenAttrDir
status_t
Volume::OpenAttrDir(void* _node, void** cookie)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_OPEN_ATTR_DIR))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	AutoIncrementer incrementer(&fOpenAttributeDirectories);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	OpenAttrDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;

	// send the request
	KernelRequestHandler handler(this, OPEN_ATTR_DIR_REPLY);
	OpenAttrDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	incrementer.Keep();
	*cookie = reply->attrDirCookie;
	return error;
}

// CloseAttrDir
status_t
Volume::CloseAttrDir(void* node, void* cookie)
{
	status_t error = _CloseAttrDir(node, cookie);
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. Haiku ignores it completely. The fsshell returns it to the
		// userland, but considers the node closed anyway.
		WARN(("Volume::CloseAttrDir(): connection lost, forcing close attr "
			"dir\n"));
		return B_OK;
	}
	return error;
}

// FreeAttrDirCookie
status_t
Volume::FreeAttrDirCookie(void* node, void* cookie)
{
	status_t error = _FreeAttrDirCookie(node, cookie);
	bool disconnected = false;
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. It's completely ignored by Haiku as well as by the fsshell.
		WARN(("Volume::FreeAttrDirCookie(): connection lost, forcing free attr "
			"dir cookie\n"));
		error = B_OK;
		disconnected = true;
	}

	int32 openAttrDirs = atomic_add(&fOpenAttributeDirectories, -1);
	if (openAttrDirs <= 1 && disconnected)
		_PutAllPendingVNodes();
	return error;
}

// ReadAttrDir
status_t
Volume::ReadAttrDir(void* _node, void* cookie, void* buffer,
	size_t bufferSize, uint32 count, uint32* countRead)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_READ_ATTR_DIR))
		return B_BAD_VALUE;

	*countRead = 0;
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadAttrDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->attrDirCookie = cookie;
	request->bufferSize = bufferSize;
	request->count = count;

	// send the request
	KernelRequestHandler handler(this, READ_ATTR_DIR_REPLY);
	ReadAttrDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	if (reply->count < 0 || reply->count > count)
		return B_BAD_DATA;
	if ((int32)bufferSize < reply->buffer.GetSize())
		return B_BAD_DATA;

	*countRead = reply->count;
	if (*countRead > 0) {
		// copy the buffer -- limit the number of bytes to copy
		uint32 maxBytes = *countRead
			* (sizeof(struct dirent) + B_ATTR_NAME_LENGTH);
		uint32 copyBytes = reply->buffer.GetSize();
		if (copyBytes > maxBytes)
			copyBytes = maxBytes;
		memcpy(buffer, reply->buffer.GetData(), copyBytes);
	}
	_SendReceiptAck(port);
	return error;
}

// RewindAttrDir
status_t
Volume::RewindAttrDir(void* _node, void* cookie)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_REWIND_ATTR_DIR))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	RewindAttrDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->attrDirCookie = cookie;

	// send the request
	KernelRequestHandler handler(this, REWIND_ATTR_DIR_REPLY);
	RewindAttrDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}


// #pragma mark - attributes

// CreateAttr
status_t
Volume::CreateAttr(void* _node, const char* name, uint32 type, int openMode,
	void** cookie)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_CREATE_ATTR))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	AutoIncrementer incrementer(&fOpenAttributes);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	CreateAttrRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	error = allocator.AllocateString(request->name, name);
	request->type = type;
	request->openMode = openMode;
	if (error != B_OK)
		return error;

	// send the request
	KernelRequestHandler handler(this, CREATE_ATTR_REPLY);
	CreateAttrReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	incrementer.Keep();
	*cookie = reply->attrCookie;
	return error;
}

// OpenAttr
status_t
Volume::OpenAttr(void* _node, const char* name, int openMode,
	void** cookie)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_OPEN_ATTR))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	AutoIncrementer incrementer(&fOpenAttributes);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	OpenAttrRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	error = allocator.AllocateString(request->name, name);
	request->openMode = openMode;
	if (error != B_OK)
		return error;

	// send the request
	KernelRequestHandler handler(this, OPEN_ATTR_REPLY);
	OpenAttrReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	incrementer.Keep();
	*cookie = reply->attrCookie;
	return error;
}

// CloseAttr
status_t
Volume::CloseAttr(void* node, void* cookie)
{
	status_t error = _CloseAttr(node, cookie);
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. Haiku ignores it completely. The fsshell returns it to the
		// userland, but considers the node closed anyway.
		WARN(("Volume::CloseAttr(): connection lost, forcing close attr\n"));
		return B_OK;
	}
	return error;
}

// FreeAttrCookie
status_t
Volume::FreeAttrCookie(void* node, void* cookie)
{
	status_t error = _FreeAttrCookie(node, cookie);
	bool disconnected = false;
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. It's completely ignored by Haiku as well as by the fsshell.
		WARN(("Volume::FreeAttrCookie(): connection lost, forcing free attr "
			"cookie\n"));
		error = B_OK;
		disconnected = true;
	}

	int32 openAttributes = atomic_add(&fOpenAttributes, -1);
	if (openAttributes <= 1 && disconnected)
		_PutAllPendingVNodes();
	return error;
}

// ReadAttr
status_t
Volume::ReadAttr(void* _node, void* cookie, off_t pos,
	void* buffer, size_t bufferSize, size_t* bytesRead)
{
	VNode* vnode = (VNode*)_node;

	*bytesRead = 0;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_READ_ATTR))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadAttrRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->attrCookie = cookie;
	request->pos = pos;
	request->size = bufferSize;

	// send the request
	KernelRequestHandler handler(this, READ_ATTR_REPLY);
	ReadAttrReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	void* readBuffer = reply->buffer.GetData();
	if (reply->bytesRead > (uint32)reply->buffer.GetSize()
		|| reply->bytesRead > bufferSize) {
		return B_BAD_DATA;
	}
	if (reply->bytesRead > 0
		&& user_memcpy(buffer, readBuffer, reply->bytesRead) < B_OK) {
		return B_BAD_ADDRESS;
	}
	*bytesRead = reply->bytesRead;
	_SendReceiptAck(port);
	return error;
}

// WriteAttr
status_t
Volume::WriteAttr(void* _node, void* cookie, off_t pos,
	const void* buffer, size_t bufferSize, size_t* bytesWritten)
{
	VNode* vnode = (VNode*)_node;

	*bytesWritten = 0;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_WRITE_ATTR))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	WriteAttrRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->attrCookie = cookie;
	request->pos = pos;
	error = allocator.AllocateData(request->buffer, buffer, bufferSize, 1);
	if (error != B_OK)
		return error;

	// send the request
	KernelRequestHandler handler(this, WRITE_ATTR_REPLY);
	WriteAttrReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	*bytesWritten = reply->bytesWritten;
	return error;
}

// ReadAttrStat
status_t
Volume::ReadAttrStat(void* _node, void* cookie, struct stat *st)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_READ_ATTR_STAT))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadAttrStatRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->attrCookie = cookie;

	// send the request
	KernelRequestHandler handler(this, READ_ATTR_STAT_REPLY);
	ReadAttrStatReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	*st = reply->st;
	return error;
}

// WriteAttrStat
status_t
Volume::WriteAttrStat(void* _node, void* cookie, const struct stat *st,
	int statMask)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_WRITE_ATTR_STAT))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	WriteAttrStatRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->attrCookie = cookie;
	request->st = *st;
	request->mask = statMask;

	// send the request
	KernelRequestHandler handler(this, WRITE_ATTR_STAT_REPLY);
	WriteAttrStatReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// RenameAttr
status_t
Volume::RenameAttr(void* _oldNode, const char* oldName, void* _newNode,
	const char* newName)
{
	VNode* oldVNode = (VNode*)_oldNode;
	VNode* newVNode = (VNode*)_newNode;

	// check capability
	if (!HasVNodeCapability(oldVNode, FS_VNODE_CAPABILITY_RENAME_ATTR))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	RenameAttrRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->oldNode = oldVNode->clientNode;
	request->newNode = newVNode->clientNode;
	error = allocator.AllocateString(request->oldName, oldName);
	if (error == B_OK)
		error = allocator.AllocateString(request->newName, newName);
	if (error != B_OK)
		return error;

	// send the request
	KernelRequestHandler handler(this, RENAME_ATTR_REPLY);
	RenameAttrReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// RemoveAttr
status_t
Volume::RemoveAttr(void* _node, const char* name)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_REMOVE_ATTR))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	RemoveAttrRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	error = allocator.AllocateString(request->name, name);
	if (error != B_OK)
		return error;

	// send the request
	KernelRequestHandler handler(this, REMOVE_ATTR_REPLY);
	RemoveAttrReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}


// #pragma mark - indices


// OpenIndexDir
status_t
Volume::OpenIndexDir(void** cookie)
{
	// check capability
	if (!HasCapability(FS_VOLUME_CAPABILITY_OPEN_INDEX_DIR))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	AutoIncrementer incrementer(&fOpenIndexDirectories);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	OpenIndexDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;

	// send the request
	KernelRequestHandler handler(this, OPEN_INDEX_DIR_REPLY);
	OpenIndexDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	incrementer.Keep();
	*cookie = reply->indexDirCookie;
	return error;
}

// CloseIndexDir
status_t
Volume::CloseIndexDir(void* cookie)
{
	status_t error = _CloseIndexDir(cookie);
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. Haiku ignores it completely. The fsshell returns it to the
		// userland, but considers the node closed anyway.
		WARN(("Volume::CloseIndexDir(): connection lost, forcing close "
			"index dir\n"));
		return B_OK;
	}
	return error;
}

// FreeIndexDirCookie
status_t
Volume::FreeIndexDirCookie(void* cookie)
{
	status_t error = _FreeIndexDirCookie(cookie);
	bool disconnected = false;
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. It's completely ignored by Haiku as well as by the fsshell.
		WARN(("Volume::FreeIndexDirCookie(): connection lost, forcing free "
			"index dir cookie\n"));
		error = B_OK;
		disconnected = true;
	}

	int32 openIndexDirs = atomic_add(&fOpenIndexDirectories, -1);
	if (openIndexDirs <= 1 && disconnected)
		_PutAllPendingVNodes();
	return error;
}

// ReadIndexDir
status_t
Volume::ReadIndexDir(void* cookie, void* buffer, size_t bufferSize,
	uint32 count, uint32* countRead)
{
	*countRead = 0;

	// check capability
	if (!HasCapability(FS_VOLUME_CAPABILITY_READ_INDEX_DIR))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadIndexDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->indexDirCookie = cookie;
	request->bufferSize = bufferSize;
	request->count = count;

	// send the request
	KernelRequestHandler handler(this, READ_INDEX_DIR_REPLY);
	ReadIndexDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	if (reply->count < 0 || reply->count > count)
		return B_BAD_DATA;
	if ((int32)bufferSize < reply->buffer.GetSize())
		return B_BAD_DATA;

	*countRead = reply->count;
	if (*countRead > 0) {
		// copy the buffer -- limit the number of bytes to copy
		uint32 maxBytes = *countRead
			* (sizeof(struct dirent) + B_FILE_NAME_LENGTH);
		uint32 copyBytes = reply->buffer.GetSize();
		if (copyBytes > maxBytes)
			copyBytes = maxBytes;
		memcpy(buffer, reply->buffer.GetData(), copyBytes);
	}
	_SendReceiptAck(port);
	return error;
}

// RewindIndexDir
status_t
Volume::RewindIndexDir(void* cookie)
{
	// check capability
	if (!HasCapability(FS_VOLUME_CAPABILITY_REWIND_INDEX_DIR))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	RewindIndexDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->indexDirCookie = cookie;

	// send the request
	KernelRequestHandler handler(this, REWIND_INDEX_DIR_REPLY);
	RewindIndexDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// CreateIndex
status_t
Volume::CreateIndex(const char* name, uint32 type, uint32 flags)
{
	// check capability
	if (!HasCapability(FS_VOLUME_CAPABILITY_CREATE_INDEX))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	CreateIndexRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	error = allocator.AllocateString(request->name, name);
	request->type = type;
	request->flags = flags;
	if (error != B_OK)
		return error;

	// send the request
	KernelRequestHandler handler(this, CREATE_INDEX_REPLY);
	CreateIndexReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// RemoveIndex
status_t
Volume::RemoveIndex(const char* name)
{
	// check capability
	if (!HasCapability(FS_VOLUME_CAPABILITY_REMOVE_INDEX))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	RemoveIndexRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	error = allocator.AllocateString(request->name, name);
	if (error != B_OK)
		return error;

	// send the request
	KernelRequestHandler handler(this, REMOVE_INDEX_REPLY);
	RemoveIndexReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// ReadIndexStat
status_t
Volume::ReadIndexStat(const char* name, struct stat *st)
{
	// check capability
	if (!HasCapability(FS_VOLUME_CAPABILITY_READ_INDEX_STAT))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadIndexStatRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	error = allocator.AllocateString(request->name, name);
	if (error != B_OK)
		return error;

	// send the request
	KernelRequestHandler handler(this, READ_INDEX_STAT_REPLY);
	ReadIndexStatReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	*st = reply->st;
	return error;
}


// #pragma mark - queries


// OpenQuery
status_t
Volume::OpenQuery(const char* queryString, uint32 flags, port_id targetPort,
	uint32 token, void** cookie)
{
	// check capability
	if (!HasCapability(FS_VOLUME_CAPABILITY_OPEN_QUERY))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	AutoIncrementer incrementer(&fOpenQueries);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	OpenQueryRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	error = allocator.AllocateString(request->queryString, queryString);
	if (error != B_OK)
		return error;
	request->flags = flags;
	request->port = targetPort;
	request->token = token;

	// send the request
	KernelRequestHandler handler(this, OPEN_QUERY_REPLY);
	OpenQueryReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	incrementer.Keep();
	*cookie = reply->queryCookie;
	return error;
}

// CloseQuery
status_t
Volume::CloseQuery(void* cookie)
{
	status_t error = _CloseQuery(cookie);
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. Haiku ignores it completely. The fsshell returns it to the
		// userland, but considers the node closed anyway.
		WARN(("Volume::CloseQuery(): connection lost, forcing close query\n"));
		return B_OK;
	}
	return error;
}

// FreeQueryCookie
status_t
Volume::FreeQueryCookie(void* cookie)
{
	status_t error = _FreeQueryCookie(cookie);
	bool disconnected = false;
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. It's completely ignored by Haiku as well as by the fsshell.
		WARN(("Volume::FreeQueryCookie(): connection lost, forcing free "
			"query cookie\n"));
		error = B_OK;
		disconnected = true;
	}

	int32 openQueries = atomic_add(&fOpenQueries, -1);
	if (openQueries <= 1 && disconnected)
		_PutAllPendingVNodes();
	return error;
}

// ReadQuery
status_t
Volume::ReadQuery(void* cookie, void* buffer, size_t bufferSize,
	uint32 count, uint32* countRead)
{
	*countRead = 0;

	// check capability
	if (!HasCapability(FS_VOLUME_CAPABILITY_READ_QUERY))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadQueryRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->queryCookie = cookie;
	request->bufferSize = bufferSize;
	request->count = count;

	// send the request
	KernelRequestHandler handler(this, READ_QUERY_REPLY);
	ReadQueryReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	if (reply->count < 0 || reply->count > count)
		return B_BAD_DATA;
	if ((int32)bufferSize < reply->buffer.GetSize())
		return B_BAD_DATA;

	*countRead = reply->count;
	if (*countRead > 0) {
		// copy the buffer -- limit the number of bytes to copy
		uint32 maxBytes = *countRead
			* (sizeof(struct dirent) + B_FILE_NAME_LENGTH);
		uint32 copyBytes = reply->buffer.GetSize();
		if (copyBytes > maxBytes)
			copyBytes = maxBytes;
		memcpy(buffer, reply->buffer.GetData(), copyBytes);
	}
	_SendReceiptAck(port);
	return error;
}

// RewindQuery
status_t
Volume::RewindQuery(void* cookie)
{
	// check capability
	if (!HasCapability(FS_VOLUME_CAPABILITY_REWIND_QUERY))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	RewindQueryRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->queryCookie = cookie;

	// send the request
	KernelRequestHandler handler(this, REWIND_QUERY_REPLY);
	RewindQueryReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// #pragma mark -
// #pragma mark ----- private implementations -----


// _InitVolumeOps
void
Volume::_InitVolumeOps()
{
	memcpy(&fVolumeOps, &gUserlandFSVolumeOps, sizeof(fs_volume_ops));

	#undef CLEAR_UNSUPPORTED
	#define CLEAR_UNSUPPORTED(capability, op) 	\
		if (!fCapabilities.Get(capability))				\
			fVolumeOps.op = NULL

	// FS operations
	// FS_VOLUME_CAPABILITY_UNMOUNT: unmount
		// always needed

	// FS_VOLUME_CAPABILITY_READ_FS_INFO: read_fs_info
		// always needed
	CLEAR_UNSUPPORTED(FS_VOLUME_CAPABILITY_WRITE_FS_INFO, write_fs_info);
	CLEAR_UNSUPPORTED(FS_VOLUME_CAPABILITY_SYNC, sync);

	// vnode operations
	// FS_VOLUME_CAPABILITY_GET_VNODE: get_vnode
		// always needed

	// index directory & index operations
	CLEAR_UNSUPPORTED(FS_VOLUME_CAPABILITY_OPEN_INDEX_DIR, open_index_dir);
	// FS_VOLUME_CAPABILITY_CLOSE_INDEX_DIR: close_index_dir
		// always needed
	// FS_VOLUME_CAPABILITY_FREE_INDEX_DIR_COOKIE: free_index_dir_cookie
		// always needed
	CLEAR_UNSUPPORTED(FS_VOLUME_CAPABILITY_READ_INDEX_DIR, read_index_dir);
	CLEAR_UNSUPPORTED(FS_VOLUME_CAPABILITY_REWIND_INDEX_DIR, rewind_index_dir);

	CLEAR_UNSUPPORTED(FS_VOLUME_CAPABILITY_CREATE_INDEX, create_index);
	CLEAR_UNSUPPORTED(FS_VOLUME_CAPABILITY_REMOVE_INDEX, remove_index);
	CLEAR_UNSUPPORTED(FS_VOLUME_CAPABILITY_READ_INDEX_STAT, read_index_stat);

	// query operations
	CLEAR_UNSUPPORTED(FS_VOLUME_CAPABILITY_OPEN_QUERY, open_query);
	// FS_VOLUME_CAPABILITY_CLOSE_QUERY: close_query
		// always needed
	// FS_VOLUME_CAPABILITY_FREE_QUERY_COOKIE: free_query_cookie
		// always needed
	CLEAR_UNSUPPORTED(FS_VOLUME_CAPABILITY_READ_QUERY, read_query);
	CLEAR_UNSUPPORTED(FS_VOLUME_CAPABILITY_REWIND_QUERY, rewind_query);

	#undef CLEAR_UNSUPPORTED
}


// #pragma mark -


// _Mount
status_t
Volume::_Mount(const char* device, uint32 flags, const char* parameters)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// get the current working directory
	char cwd[B_PATH_NAME_LENGTH];
	if (!getcwd(cwd, sizeof(cwd)))
		return errno;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	MountVolumeRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->nsid = GetID();
	error = allocator.AllocateString(request->cwd, cwd);
	if (error == B_OK)
		error = allocator.AllocateString(request->device, device);
	request->flags = flags;
	if (error == B_OK)
		error = allocator.AllocateString(request->parameters, parameters);
	if (error != B_OK)
		return error;

	// send the request
	KernelRequestHandler handler(this, MOUNT_VOLUME_REPLY);
	MountVolumeReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	fRootID = reply->rootID;
	fUserlandVolume = reply->volume;
	fCapabilities = reply->capabilities;

	return error;
}

// _Unmount
status_t
Volume::_Unmount()
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	UnmountVolumeRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;

	// send the request
	KernelRequestHandler handler(this, UNMOUNT_VOLUME_REPLY);
	UnmountVolumeReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _ReadFSInfo
status_t
Volume::_ReadFSInfo(fs_info* info)
{
	// check capability
	if (!HasCapability(FS_VOLUME_CAPABILITY_READ_FS_INFO))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadFSInfoRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;

	// send the request
	KernelRequestHandler handler(this, READ_FS_INFO_REPLY);
	ReadFSInfoReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	*info = reply->info;
	return error;
}

// _Lookup
status_t
Volume::_Lookup(void* _dir, const char* entryName, ino_t* vnid)
{
	VNode* vnode = (VNode*)_dir;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	LookupRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	error = allocator.AllocateString(request->entryName, entryName);
	if (error != B_OK)
		return error;

	// send the request
	KernelRequestHandler handler(this, LOOKUP_REPLY);
	LookupReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	*vnid = reply->vnid;

	// The VFS will balance the get_vnode() call for the FS.
	_DecrementVNodeCount(*vnid);
	return error;
}

// _WriteVNode
status_t
Volume::_WriteVNode(void* _node, bool reenter)
{
	VNode* vnode = (VNode*)_node;

	// At any rate remove the vnode from our map and delete it. We don't do that
	// right now, though, since we might still need to serve file cache requests
	// from the client FS.
	VNodeRemover nodeRemover(this, vnode);

	void* clientNode = vnode->clientNode;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	WriteVNodeRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = clientNode;
	request->reenter = reenter;

	// send the request
	KernelRequestHandler handler(this, WRITE_VNODE_REPLY);
	WriteVNodeReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _ReadStat
status_t
Volume::_ReadStat(void* _node, struct stat* st)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_READ_STAT))
		return B_BAD_VALUE;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadStatRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;

	// send the request
	KernelRequestHandler handler(this, READ_STAT_REPLY);
	ReadStatReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	*st = reply->st;
	return error;
}

// _Close
status_t
Volume::_Close(void* _node, void* cookie)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_CLOSE))
		return B_OK;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	CloseRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->fileCookie = cookie;

	// send the request
	KernelRequestHandler handler(this, CLOSE_REPLY);
	CloseReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _FreeCookie
status_t
Volume::_FreeCookie(void* _node, void* cookie)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_FREE_COOKIE))
		return B_OK;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FreeCookieRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->fileCookie = cookie;

	// send the request
	KernelRequestHandler handler(this, FREE_COOKIE_REPLY);
	FreeCookieReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _CloseDir
status_t
Volume::_CloseDir(void* _node, void* cookie)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_CLOSE_DIR))
		return B_OK;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	CloseDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->dirCookie = cookie;

	// send the request
	KernelRequestHandler handler(this, CLOSE_DIR_REPLY);
	CloseDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _FreeDirCookie
status_t
Volume::_FreeDirCookie(void* _node, void* cookie)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_FREE_DIR_COOKIE))
		return B_OK;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FreeDirCookieRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->dirCookie = cookie;

	// send the request
	KernelRequestHandler handler(this, FREE_DIR_COOKIE_REPLY);
	FreeDirCookieReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _CloseAttrDir
status_t
Volume::_CloseAttrDir(void* _node, void* cookie)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_CLOSE_ATTR_DIR))
		return B_OK;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	CloseAttrDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->attrDirCookie = cookie;

	// send the request
	KernelRequestHandler handler(this, CLOSE_ATTR_DIR_REPLY);
	CloseAttrDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _FreeAttrDirCookie
status_t
Volume::_FreeAttrDirCookie(void* _node, void* cookie)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_FREE_ATTR_DIR_COOKIE))
		return B_OK;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FreeAttrDirCookieRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->attrDirCookie = cookie;

	// send the request
	KernelRequestHandler handler(this, FREE_ATTR_DIR_COOKIE_REPLY);
	FreeAttrDirCookieReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _CloseAttr
status_t
Volume::_CloseAttr(void* _node, void* cookie)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_CLOSE_ATTR))
		return B_OK;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	CloseAttrRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->attrCookie = cookie;

	// send the request
	KernelRequestHandler handler(this, CLOSE_ATTR_REPLY);
	CloseAttrReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _FreeAttrCookie
status_t
Volume::_FreeAttrCookie(void* _node, void* cookie)
{
	VNode* vnode = (VNode*)_node;

	// check capability
	if (!HasVNodeCapability(vnode, FS_VNODE_CAPABILITY_FREE_ATTR_COOKIE))
		return B_OK;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FreeAttrCookieRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->node = vnode->clientNode;
	request->attrCookie = cookie;

	// send the request
	KernelRequestHandler handler(this, FREE_ATTR_COOKIE_REPLY);
	FreeAttrCookieReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _CloseIndexDir
status_t
Volume::_CloseIndexDir(void* cookie)
{
	// check capability
	if (!HasCapability(FS_VOLUME_CAPABILITY_CLOSE_INDEX_DIR))
		return B_OK;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	CloseIndexDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->indexDirCookie = cookie;

	// send the request
	KernelRequestHandler handler(this, CLOSE_INDEX_DIR_REPLY);
	CloseIndexDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _FreeIndexDirCookie
status_t
Volume::_FreeIndexDirCookie(void* cookie)
{
	// check capability
	if (!HasCapability(FS_VOLUME_CAPABILITY_FREE_INDEX_DIR_COOKIE))
		return B_OK;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FreeIndexDirCookieRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->indexDirCookie = cookie;

	// send the request
	KernelRequestHandler handler(this, FREE_INDEX_DIR_COOKIE_REPLY);
	FreeIndexDirCookieReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _CloseQuery
status_t
Volume::_CloseQuery(void* cookie)
{
	// check capability
	if (!HasCapability(FS_VOLUME_CAPABILITY_CLOSE_QUERY))
		return B_OK;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	CloseQueryRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->queryCookie = cookie;

	// send the request
	KernelRequestHandler handler(this, CLOSE_QUERY_REPLY);
	CloseQueryReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _FreeQueryCookie
status_t
Volume::_FreeQueryCookie(void* cookie)
{
	// check capability
	if (!HasCapability(FS_VOLUME_CAPABILITY_FREE_QUERY_COOKIE))
		return B_OK;

	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FreeQueryCookieRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = fUserlandVolume;
	request->queryCookie = cookie;

	// send the request
	KernelRequestHandler handler(this, FREE_QUERY_COOKIE_REPLY);
	FreeQueryCookieReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _SendRequest
status_t
Volume::_SendRequest(RequestPort* port, RequestAllocator* allocator,
	RequestHandler* handler, Request** reply)
{
	// fill in the caller info
	KernelRequest* request = static_cast<KernelRequest*>(
		allocator->GetRequest());
	Thread* thread = thread_get_current_thread();
	request->team = thread->team->id;
	request->thread = thread->id;
	request->user = geteuid();
	request->group = getegid();

	if (!fFileSystem->IsUserlandServerThread())
		return port->SendRequest(allocator, handler, reply);
	// Here it gets dangerous: a thread of the userland server team being here
	// calls for trouble. We try receiving the request with a timeout, and
	// close the port -- which will disconnect the whole FS.
	status_t error = port->SendRequest(allocator, handler, reply,
		kUserlandServerlandPortTimeout);
	if (error == B_TIMED_OUT || error == B_WOULD_BLOCK)
		port->Close();
	return error;
}

// _SendReceiptAck
status_t
Volume::_SendReceiptAck(RequestPort* port)
{
	RequestAllocator allocator(port->GetPort());
	ReceiptAckReply* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	return port->SendRequest(&allocator);
}

// _IncrementVNodeCount
void
Volume::_IncrementVNodeCount(ino_t vnid)
{
	MutexLocker _(fLock);

	if (!fVNodeCountingEnabled)
		return;

	VNode* vnode = fVNodes->Lookup(vnid);
	if (vnode == NULL) {
		ERROR(("Volume::_IncrementVNodeCount(): Node with ID %" B_PRId64
			" not known!\n", vnid));
		return;
	}

	vnode->useCount++;
//PRINT(("_IncrementVNodeCount(%Ld): count: %ld, fVNodeCountMap size: %ld\n", vnid, *count, fVNodeCountMap->Size()));
}


// _DecrementVNodeCount
void
Volume::_DecrementVNodeCount(ino_t vnid)
{
	MutexLocker _(fLock);

	if (!fVNodeCountingEnabled)
		return;

	VNode* vnode = fVNodes->Lookup(vnid);
	if (vnode == NULL) {
		ERROR(("Volume::_DecrementVNodeCount(): Node with ID %" B_PRId64 " not "
			"known!\n", vnid));
		return;
	}

	vnode->useCount--;
//PRINT(("_DecrementVNodeCount(%Ld): count: %ld, fVNodeCountMap size: %ld\n", vnid, tmpCount, fVNodeCountMap->Size()));
}


// _RemoveInvalidVNode
void
Volume::_RemoveInvalidVNode(ino_t vnid)
{
	MutexLocker locker(fLock);

	VNode* vnode = fVNodes->Lookup(vnid);
	if (vnode == NULL) {
		ERROR(("Volume::_RemoveInvalidVNode(): Node with ID %" B_PRId64
			" not known!\n", vnid));
		return;
	}

	fVNodes->Remove(vnode);
	locker.Unlock();

	// release all references acquired so far
	if (fVNodeCountingEnabled) {
		for (; vnode->useCount > 0; vnode->useCount--)
			put_vnode(fFSVolume, vnid);
	}

	vnode->Delete(this);
}


// _InternalIOCtl
status_t
Volume::_InternalIOCtl(userlandfs_ioctl* buffer, int32 bufferSize)
{
	if (buffer->version != USERLAND_IOCTL_CURRENT_VERSION)
		return B_BAD_VALUE;
	status_t result = B_OK;
	switch (buffer->command) {
		case USERLAND_IOCTL_PUT_ALL_PENDING_VNODES:
			result = _PutAllPendingVNodes();
			break;
		default:
			return B_BAD_VALUE;
	}
	buffer->error = result;
	return B_OK;
}

// _PutAllPendingVNodes
status_t
Volume::_PutAllPendingVNodes()
{
PRINT(("Volume::_PutAllPendingVNodes()\n"));
	if (!fFileSystem->GetPortPool()->IsDisconnected()) {
		PRINT(("Volume::_PutAllPendingVNodes() failed: still connected\n"));
		return USERLAND_IOCTL_STILL_CONNECTED;
	}

	MutexLocker locker(fLock);

	if (!fVNodeCountingEnabled) {
		PRINT(("Volume::_PutAllPendingVNodes() failed: vnode counting "
			"disabled\n"));
		return USERLAND_IOCTL_VNODE_COUNTING_DISABLED;
	}
	// Check whether there are open entities at the moment.
	if (atomic_get(&fOpenFiles) > 0) {
		PRINT(("Volume::_PutAllPendingVNodes() failed: open files\n"));
		return USERLAND_IOCTL_OPEN_FILES;
	}
	if (atomic_get(&fOpenDirectories) > 0) {
		PRINT(("Volume::_PutAllPendingVNodes() failed: open dirs\n"));
		return USERLAND_IOCTL_OPEN_DIRECTORIES;
	}
	if (atomic_get(&fOpenAttributeDirectories) > 0) {
		PRINT(("Volume::_PutAllPendingVNodes() failed: open attr dirs\n"));
		return USERLAND_IOCTL_OPEN_ATTRIBUTE_DIRECTORIES;
	}
	if (atomic_get(&fOpenAttributes) > 0) {
		PRINT(("Volume::_PutAllPendingVNodes() failed: open attributes\n"));
		return USERLAND_IOCTL_OPEN_ATTRIBUTES;
	}
	if (atomic_get(&fOpenIndexDirectories) > 0) {
		PRINT(("Volume::_PutAllPendingVNodes() failed: open index dirs\n"));
		return USERLAND_IOCTL_OPEN_INDEX_DIRECTORIES;
	}
	if (atomic_get(&fOpenQueries) > 0) {
		PRINT(("Volume::_PutAllPendingVNodes() failed: open queries\n"));
		return USERLAND_IOCTL_OPEN_QUERIES;
	}
	// No open entities. Since the port pool is disconnected, no new
	// entities can be opened. Disable node counting and put all pending
	// vnodes.
	fVNodeCountingEnabled = false;

	int32 putVNodeCount = 0;

	// Since the vnode map can still change, we need to iterate to the first
	// node we need to put, drop the lock, put the node, and restart from the
	// beginning.
	// TODO: Optimize by extracting batches of relevant nodes to an on-stack
	// array.
	bool nodeFound;
	do {
		nodeFound = false;

		// get the next node to put
		for (VNodeMap::Iterator it = fVNodes->GetIterator();
				VNode* vnode = it.Next();) {
			if (vnode->useCount > 0) {
				ino_t vnid = vnode->id;
				int32 count = vnode->useCount;
				vnode->useCount = 0;
				fs_vnode_ops* ops = vnode->ops->ops;
				bool published = vnode->published;

				locker.Unlock();

				// If the node has not yet been published, we have to do that
				// before putting otherwise the VFS will complain that the node
				// is busy when the last reference is gone.
				if (!published)
					publish_vnode(fFSVolume, vnid, vnode, ops, S_IFDIR, 0);

				for (int32 i = 0; i < count; i++) {
					PutVNode(vnid);
					putVNodeCount++;
				}

				locker.Lock();

				nodeFound = true;
				break;
			}
		}
	} while (nodeFound);

	PRINT(("Volume::_PutAllPendingVNodes() successful: Put %" B_PRId32
		" vnodes\n", putVNodeCount));

	return B_OK;
}


// _RegisterIORequest
status_t
Volume::_RegisterIORequest(io_request* request, int32* requestID)
{
	MutexLocker _(fLock);

	// get the next free ID
	while (fIORequestInfosByID->Lookup(++fLastIORequestID) != NULL) {
	}

	// allocate the info
	IORequestInfo* info = new(std::nothrow) IORequestInfo(request,
		++fLastIORequestID);
	if (info == NULL)
		return B_NO_MEMORY;

	// add the info to the maps
	fIORequestInfosByID->Insert(info);
	fIORequestInfosByStruct->Insert(info);

	*requestID = info->id;

	return B_OK;
}


// _UnregisterIORequest
status_t
Volume::_UnregisterIORequest(int32 requestID)
{
	MutexLocker _(fLock);

	if (IORequestInfo* info = fIORequestInfosByID->Lookup(requestID)) {
		fIORequestInfosByID->Remove(info);
		fIORequestInfosByStruct->Remove(info);
		return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


// _FindIORequest
status_t
Volume::_FindIORequest(int32 requestID, io_request** request)
{
	MutexLocker _(fLock);

	if (IORequestInfo* info = fIORequestInfosByID->Lookup(requestID)) {
		*request = info->request;
		return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


// _FindIORequest
status_t
Volume::_FindIORequest(io_request* request, int32* requestID)
{
	MutexLocker _(fLock);

	if (IORequestInfo* info = fIORequestInfosByStruct->Lookup(request)) {
		*requestID = info->id;
		return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


/*static*/ status_t
Volume::_IterativeFDIOGetVecs(void* _cookie, io_request* ioRequest,
	off_t offset, size_t size, struct file_io_vec* vecs, size_t* _count)
{
	IterativeFDIOCookie* cookie = (IterativeFDIOCookie*)_cookie;
	Volume* volume = cookie->volume;

	MutexLocker locker(volume->fLock);

	// If there are vecs cached in the cookie and the offset matches, return
	// those.
	if (cookie->vecs != NULL) {
		size_t vecCount = 0;
		if (offset == cookie->offset) {
			// good, copy the vecs
			while (size > 0 && vecCount < cookie->vecCount
					&& vecCount < *_count) {
				off_t maxSize = std::min((off_t)size,
					cookie->vecs[vecCount].length);
				vecs[vecCount].offset = cookie->vecs[vecCount].offset;
				vecs[vecCount].length = maxSize;

				size -= maxSize;
				vecCount++;
			}
		}

		cookie->vecs = NULL;
		cookie->vecCount = 0;

		// got some vecs? -- then we're done
		if (vecCount > 0) {
			*_count = vecCount;
			return B_OK;
		}
	}

	// we have to ask the client FS
	int32 requestID = cookie->requestID;
	void* clientCookie = cookie->clientCookie;
	locker.Unlock();

	// get a free port
	RequestPort* port = volume->fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(volume->fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	IterativeIOGetVecsRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = volume->fUserlandVolume;
	request->cookie = clientCookie;
	request->offset = offset;
	request->request = requestID;
	request->size = size;
	size_t maxVecs = std::min(*_count,
		(size_t)IterativeIOGetVecsReply::MAX_VECS);
	request->vecCount = maxVecs;

	// send the request
	KernelRequestHandler handler(volume, ITERATIVE_IO_GET_VECS_REPLY);
	IterativeIOGetVecsReply* reply;
	error = volume->_SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	uint32 vecCount = reply->vecCount;
	if (vecCount < 0 || vecCount > maxVecs)
		return B_BAD_DATA;

	memcpy(vecs, reply->vecs, vecCount * sizeof(file_io_vec));
	*_count = vecCount;

	return B_OK;
}


/*static*/ status_t
Volume::_IterativeFDIOFinished(void* _cookie, io_request* ioRequest,
	status_t status, bool partialTransfer, size_t bytesTransferred)
{
	IterativeFDIOCookie* cookie = (IterativeFDIOCookie*)_cookie;
	Volume* volume = cookie->volume;

	// At any rate, we're done with the cookie after this call -- it will not
	// be used anymore.
	BReference<IterativeFDIOCookie> _(cookie, true);

	// We also want to dispose of the request.
	IORequestRemover _2(volume, cookie->requestID);

	// get a free port
	RequestPort* port = volume->fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _3(volume->fFileSystem->GetPortPool(), port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	IterativeIOFinishedRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->volume = volume->fUserlandVolume;
	request->cookie = cookie->clientCookie;
	request->request = cookie->requestID;
	request->status = status;
	request->partialTransfer = partialTransfer;
	request->bytesTransferred = bytesTransferred;

	// send the request
	KernelRequestHandler handler(volume, ITERATIVE_IO_FINISHED_REPLY);
	IterativeIOFinishedReply* reply;
	error = volume->_SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return B_OK;
}
