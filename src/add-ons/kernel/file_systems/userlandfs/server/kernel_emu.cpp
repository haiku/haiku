// kernel_emu.cpp

#include "kernel_emu.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>

#include "FileSystem.h"
#include "RequestPort.h"
#include "Requests.h"
#include "RequestThread.h"
#include "UserlandFSServer.h"
#include "UserlandRequestHandler.h"
#include "Volume.h"


// Taken from the Haiku Storage Kit (storage_support.cpp)
/*! The length of the first component is returned as well as the index at
	which the next one starts. These values are only valid, if the function
	returns \c B_OK.
	\param path the path to be parsed
	\param length the variable the length of the first component is written
		   into
	\param nextComponent the variable the index of the next component is
		   written into. \c 0 is returned, if there is no next component.
	\return \c B_OK, if \a path is not \c NULL, \c B_BAD_VALUE otherwise
*/
static status_t
parse_first_path_component(const char *path, int32& length,
						   int32& nextComponent)
{
	status_t error = (path ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		int32 i = 0;
		// find first '/' or end of name
		for (; path[i] != '/' && path[i] != '\0'; i++);
		// handle special case "/..." (absolute path)
		if (i == 0 && path[i] != '\0')
			i = 1;
		length = i;
		// find last '/' or end of name
		for (; path[i] == '/' && path[i] != '\0'; i++);
		if (path[i] == '\0')	// this covers "" as well
			nextComponent = 0;
		else
			nextComponent = i;
	}
	return error;
}

// new_path
int
UserlandFS::KernelEmu::new_path(const char *path, char **copy)
{
	// check errors and special cases
	if (!copy)
		return B_BAD_VALUE;
	if (!path) {
		*copy = NULL;
		return B_OK;
	}
	int32 len = strlen(path);
	if (len < 1)
		return B_ENTRY_NOT_FOUND;
	bool appendDot = (path[len - 1] == '/');
	if (appendDot)
		len++;
	if (len >= B_PATH_NAME_LENGTH)
		return B_NAME_TOO_LONG;
	// check the path components
	const char *remainder = path;
	int32 length, nextComponent;
	do {
		status_t error
			= parse_first_path_component(remainder, length, nextComponent);
		if (error != B_OK)
			return error;
		if (length >= B_FILE_NAME_LENGTH)
			error = B_NAME_TOO_LONG;
		remainder += nextComponent;
	} while (nextComponent != 0);
	// clone the path
	char *copiedPath = (char*)malloc(len + 1);
	if (!copiedPath)
		return B_NO_MEMORY;
	strcpy(copiedPath, path);
	// append a dot, if desired
	if (appendDot) {
		copiedPath[len - 1] = '.';
		copiedPath[len] = '\0';
	}
	*copy = copiedPath;
	return B_OK;
}

// free_path
void
UserlandFS::KernelEmu::free_path(char *p)
{
	free(p);
}


// #pragma mark -


// get_port_and_fs
static status_t
get_port_and_fs(RequestPort** port, FileSystem** fileSystem)
{
	// get the request thread
	RequestThread* thread = RequestThread::GetCurrentThread();
	if (thread) {
		*port = thread->GetPort();
		*fileSystem = thread->GetFileSystem();
	} else {
		*port = UserlandFSServer::GetNotificationRequestPort();
		*fileSystem = UserlandFSServer::GetFileSystem();
		if (!*port || !*fileSystem)
			return B_BAD_VALUE;
	}
	return B_OK;
}

// notify_listener
status_t
UserlandFS::KernelEmu::notify_listener(int32 operation, uint32 details,
	dev_t device, ino_t oldDirectory, ino_t directory,
	ino_t node, const char* oldName, const char* name)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	NotifyListenerRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->operation = operation;
	request->details = details;
	request->device = device;
	request->oldDirectory = oldDirectory;
	request->directory = directory;
	request->node = node;
	error = allocator.AllocateString(request->oldName, oldName);
	if (error != B_OK)
		return error;
	error = allocator.AllocateString(request->name, name);
	if (error != B_OK)
		return error;

	// send the request
	UserlandRequestHandler handler(fileSystem, NOTIFY_LISTENER_REPLY);
	NotifyListenerReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// notify_select_event
status_t
UserlandFS::KernelEmu::notify_select_event(selectsync *sync, uint8 event,
	bool unspecifiedEvent)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	NotifySelectEventRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->sync = sync;
	request->event = event;
	request->unspecifiedEvent = unspecifiedEvent;

	// send the request
	UserlandRequestHandler handler(fileSystem, NOTIFY_SELECT_EVENT_REPLY);
	NotifySelectEventReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// send_notification
status_t
UserlandFS::KernelEmu::notify_query(port_id targetPort, int32 token,
	int32 operation, dev_t device, ino_t directory, const char* name,
	ino_t node)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	NotifyQueryRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->port = targetPort;
	request->token = token;
	request->operation = operation;
	request->device = device;
	request->directory = directory;
	request->node = node;
	error = allocator.AllocateString(request->name, name);
	if (error != B_OK)
		return error;

	// send the request
	UserlandRequestHandler handler(fileSystem, NOTIFY_QUERY_REPLY);
	NotifyQueryReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}


// #pragma mark -


// get_vnode
status_t
UserlandFS::KernelEmu::get_vnode(dev_t nsid, ino_t vnid, void** node)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	GetVNodeRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->nsid = nsid;
	request->vnid = vnid;

	// send the request
	UserlandRequestHandler handler(fileSystem, GET_VNODE_REPLY);
	GetVNodeReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	*node = reply->node;
	return error;
}

// put_vnode
status_t
UserlandFS::KernelEmu::put_vnode(dev_t nsid, ino_t vnid)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	PutVNodeRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->nsid = nsid;
	request->vnid = vnid;

	// send the request
	UserlandRequestHandler handler(fileSystem, PUT_VNODE_REPLY);
	PutVNodeReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// acquire_vnode
status_t
UserlandFS::KernelEmu::acquire_vnode(dev_t nsid, ino_t vnid)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	AcquireVNodeRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->nsid = nsid;
	request->vnid = vnid;

	// send the request
	UserlandRequestHandler handler(fileSystem, ACQUIRE_VNODE_REPLY);
	AcquireVNodeReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// new_vnode
status_t
UserlandFS::KernelEmu::new_vnode(dev_t nsid, ino_t vnid, void* data,
	const FSVNodeCapabilities& capabilities)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	NewVNodeRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->nsid = nsid;
	request->vnid = vnid;
	request->node = data;
	request->capabilities = capabilities;

	// send the request
	UserlandRequestHandler handler(fileSystem, NEW_VNODE_REPLY);
	NewVNodeReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// publish_vnode
status_t
UserlandFS::KernelEmu::publish_vnode(dev_t nsid, ino_t vnid, void* data,
	int type, uint32 flags, const FSVNodeCapabilities& capabilities)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	PublishVNodeRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->nsid = nsid;
	request->vnid = vnid;
	request->node = data;
	request->type = type;
	request->flags = flags;
	request->capabilities = capabilities;

	// send the request
	UserlandRequestHandler handler(fileSystem, PUBLISH_VNODE_REPLY);
	PublishVNodeReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}


// publish_vnode
status_t
UserlandFS::KernelEmu::publish_vnode(dev_t nsid, ino_t vnid, void* data,
	const FSVNodeCapabilities& capabilities)
{
	// get the volume
	Volume* volume = FileSystem::GetInstance()->VolumeWithID(nsid);
	if (volume == NULL)
		return B_BAD_VALUE;

	// stat() the node to get its type
	int type;
	status_t error = volume->GetVNodeType(data, &type);
	if (error != B_OK)
		return error;

	// publish the node
	return UserlandFS::KernelEmu::publish_vnode(nsid, vnid, data, type, 0,
		capabilities);
}


// remove_vnode
status_t
UserlandFS::KernelEmu::remove_vnode(dev_t nsid, ino_t vnid)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	RemoveVNodeRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->nsid = nsid;
	request->vnid = vnid;

	// send the request
	UserlandRequestHandler handler(fileSystem, REMOVE_VNODE_REPLY);
	RemoveVNodeReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// unremove_vnode
status_t
UserlandFS::KernelEmu::unremove_vnode(dev_t nsid, ino_t vnid)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	UnremoveVNodeRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->nsid = nsid;
	request->vnid = vnid;

	// send the request
	UserlandRequestHandler handler(fileSystem, UNREMOVE_VNODE_REPLY);
	UnremoveVNodeReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// get_vnode_removed
status_t
UserlandFS::KernelEmu::get_vnode_removed(dev_t nsid, ino_t vnid,
	bool* removed)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	GetVNodeRemovedRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->nsid = nsid;
	request->vnid = vnid;

	// send the request
	UserlandRequestHandler handler(fileSystem, GET_VNODE_REMOVED_REPLY);
	GetVNodeRemovedReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	*removed = reply->removed;
	return reply->error;
}


// #pragma mark - file cache


// file_cache_create
status_t
UserlandFS::KernelEmu::file_cache_create(dev_t mountID, ino_t vnodeID,
	off_t size)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		RETURN_ERROR(error);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FileCacheCreateRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		RETURN_ERROR(error);

	request->nsid = mountID;
	request->vnid = vnodeID;
	request->size = size;

	// send the request
	UserlandRequestHandler handler(fileSystem, FILE_CACHE_CREATE_REPLY);
	FileCacheCreateReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	RETURN_ERROR(reply->error);
}


// file_cache_delete
status_t
UserlandFS::KernelEmu::file_cache_delete(dev_t mountID, ino_t vnodeID)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FileCacheDeleteRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->nsid = mountID;
	request->vnid = vnodeID;

	// send the request
	UserlandRequestHandler handler(fileSystem, FILE_CACHE_DELETE_REPLY);
	FileCacheDeleteReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	return reply->error;
}


// file_cache_set_enable
status_t
UserlandFS::KernelEmu::file_cache_set_enabled(dev_t mountID, ino_t vnodeID,
	bool enabled)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FileCacheSetEnabledRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->nsid = mountID;
	request->vnid = vnodeID;
	request->enabled = enabled;

	// send the request
	UserlandRequestHandler handler(fileSystem, FILE_CACHE_SET_ENABLED_REPLY);
	FileCacheSetEnabledReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	return reply->error;
}


// file_cache_set_size
status_t
UserlandFS::KernelEmu::file_cache_set_size(dev_t mountID, ino_t vnodeID,
	off_t size)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FileCacheSetSizeRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->nsid = mountID;
	request->vnid = vnodeID;
	request->size = size;

	// send the request
	UserlandRequestHandler handler(fileSystem, FILE_CACHE_SET_SIZE_REPLY);
	FileCacheSetSizeReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	return reply->error;
}


// file_cache_sync
status_t
UserlandFS::KernelEmu::file_cache_sync(dev_t mountID, ino_t vnodeID)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FileCacheSyncRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->nsid = mountID;
	request->vnid = vnodeID;

	// send the request
	UserlandRequestHandler handler(fileSystem, FILE_CACHE_SYNC_REPLY);
	FileCacheSyncReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	return reply->error;
}


// file_cache_read
status_t
UserlandFS::KernelEmu::file_cache_read(dev_t mountID, ino_t vnodeID,
	void *cookie, off_t offset, void *bufferBase, size_t *_size)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FileCacheReadRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->nsid = mountID;
	request->vnid = vnodeID;
	request->cookie = cookie;
	request->pos = offset;
	request->size = *_size;

	// send the request
	UserlandRequestHandler handler(fileSystem, FILE_CACHE_READ_REPLY);
	FileCacheReadReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;

	if (reply->bytesRead > 0) {
		memcpy(bufferBase, reply->buffer.GetData(), reply->buffer.GetSize());

		// send receipt-ack
		RequestAllocator receiptAckAllocator(port->GetPort());
		ReceiptAckReply* receiptAck;
		if (AllocateRequest(receiptAckAllocator, &receiptAck) == B_OK)
			port->SendRequest(&receiptAckAllocator);
	}

	*_size = reply->bytesRead;

	return B_OK;
}


// file_cache_write
status_t
UserlandFS::KernelEmu::file_cache_write(dev_t mountID, ino_t vnodeID,
	void *cookie, off_t offset, const void *buffer, size_t *_size)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FileCacheWriteRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->nsid = mountID;
	request->vnid = vnodeID;
	request->cookie = cookie;
	request->size = *_size;
	request->pos = offset;

	if (buffer != NULL) {
		error = allocator.AllocateData(request->buffer, buffer, *_size, 1,
			false);
		if (error != B_OK)
			return error;
	}

	// send the request
	UserlandRequestHandler handler(fileSystem, FILE_CACHE_WRITE_REPLY);
	FileCacheWriteReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	*_size = reply->bytesWritten;
	return reply->error;
}


// #pragma mark - I/O


status_t
UserlandFS::KernelEmu::do_iterative_fd_io(dev_t volumeID, int fd,
	int32 requestID, void* cookie, const file_io_vec* vecs, uint32 vecCount)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	DoIterativeFDIORequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->nsid = volumeID;
	request->fd = fd;
	request->request = requestID;
	request->cookie = cookie;

	if (vecCount > 0) {
		vecCount = std::min(vecCount, (uint32)DoIterativeFDIORequest::MAX_VECS);
		memcpy(request->vecs, vecs, sizeof(file_io_vec) * vecCount);
	}
	request->vecCount = vecCount;

	// send the request
	UserlandRequestHandler handler(fileSystem, DO_ITERATIVE_FD_IO_REPLY);
	DoIterativeFDIOReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
// TODO: Up to this point we should call the finished hook on error!
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	return reply->error;
}


status_t
UserlandFS::KernelEmu::read_from_io_request(dev_t volumeID, int32 requestID,
	void* buffer, size_t size)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadFromIORequestRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->nsid = volumeID;
	request->request = requestID;
	request->size = size;

	// send the request
	UserlandRequestHandler handler(fileSystem, READ_FROM_IO_REQUEST_REPLY);
	ReadFromIORequestReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;

	memcpy(buffer, reply->buffer.GetData(), reply->buffer.GetSize());

	// send receipt-ack
	RequestAllocator receiptAckAllocator(port->GetPort());
	ReceiptAckReply* receiptAck;
	if (AllocateRequest(receiptAckAllocator, &receiptAck) == B_OK)
		port->SendRequest(&receiptAckAllocator);

	return B_OK;
}


status_t
UserlandFS::KernelEmu::write_to_io_request(dev_t volumeID, int32 requestID,
	const void* buffer, size_t size)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	WriteToIORequestRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->nsid = volumeID;
	request->request = requestID;

	error = allocator.AllocateData(request->buffer, buffer, size, 1, false);
	if (error != B_OK)
		return error;

	// send the request
	UserlandRequestHandler handler(fileSystem, WRITE_TO_IO_REQUEST_REPLY);
	FileCacheWriteReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	return reply->error;
}


status_t
UserlandFS::KernelEmu::notify_io_request(dev_t volumeID, int32 requestID,
	status_t status)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	NotifyIORequestRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->nsid = volumeID;
	request->request = requestID;
	request->status = status;

	// send the request
	UserlandRequestHandler handler(fileSystem, NOTIFY_IO_REQUEST_REPLY);
	NotifyIORequestReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	return reply->error;
}


// #pragma mark - node monitoring


status_t
UserlandFS::KernelEmu::add_node_listener(dev_t device, ino_t node, uint32 flags,
	void* listener)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	AddNodeListenerRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->device = device;
	request->node = node;
	request->flags = flags;
	request->listener = listener;

	// send the request
	UserlandRequestHandler handler(fileSystem, ADD_NODE_LISTENER_REPLY);
	AddNodeListenerReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	return reply->error;
}


status_t
UserlandFS::KernelEmu::remove_node_listener(dev_t device, ino_t node,
	void* listener)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	RemoveNodeListenerRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->device = device;
	request->node = node;
	request->listener = listener;

	// send the request
	UserlandRequestHandler handler(fileSystem, REMOVE_NODE_LISTENER_REPLY);
	RemoveNodeListenerReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	return reply->error;
}


// #pragma mark -


// kernel_debugger
void
UserlandFS::KernelEmu::kernel_debugger(const char *message)
{
	debugger(message);
}

// vdprintf
void
UserlandFS::KernelEmu::vdprintf(const char *format, va_list args)
{
	vprintf(format, args);
}

// dprintf
void
UserlandFS::KernelEmu::dprintf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vdprintf(format, args);
	va_end(args);
}

void
UserlandFS::KernelEmu::dump_block(const char *buffer, int size,
	const char *prefix)
{
	// TODO: Implement!
}

// parse_expression
//ulong
//parse_expression(char *str)
//{
//	return 0;
//}

// add_debugger_command
int
UserlandFS::KernelEmu::add_debugger_command(char *name,
	int (*func)(int argc, char **argv), char *help)
{
	return B_OK;
}

// remove_debugger_command
int
UserlandFS::KernelEmu::remove_debugger_command(char *name,
	int (*func)(int argc, char **argv))
{
	return B_OK;
}

// parse_expression
uint32
UserlandFS::KernelEmu::parse_expression(const char *string)
{
	return 0;
}


// kprintf
//void
//kprintf(const char *format, ...)
//{
//}

// spawn_kernel_thread
thread_id
UserlandFS::KernelEmu::spawn_kernel_thread(thread_entry function,
	const char *threadName, long priority, void *arg)
{
	return spawn_thread(function, threadName, priority, arg);
}
