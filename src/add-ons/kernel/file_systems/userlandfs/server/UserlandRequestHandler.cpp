/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "UserlandRequestHandler.h"

#include <algorithm>

#include <util/KMessage.h>

#include <Notifications.h>

#include "AutoDeleter.h"
#include "Compatibility.h"
#include "Debug.h"
#include "FileSystem.h"
#include "IORequestInfo.h"
#include "RequestPort.h"
#include "Requests.h"
#include "RequestThread.h"
#include "SingleReplyRequestHandler.h"
#include "Volume.h"


// constructor
UserlandRequestHandler::UserlandRequestHandler(FileSystem* fileSystem)
	: RequestHandler(),
	  fFileSystem(fileSystem),
	  fExpectReply(false),
	  fExpectedReply(0)
{
}

// constructor
UserlandRequestHandler::UserlandRequestHandler(FileSystem* fileSystem,
	uint32 expectedReply)
	: RequestHandler(),
	  fFileSystem(fileSystem),
	  fExpectReply(true),
	  fExpectedReply(expectedReply)
{
}

// destructor
UserlandRequestHandler::~UserlandRequestHandler()
{
}

// HandleRequest
status_t
UserlandRequestHandler::HandleRequest(Request* request)
{
	if (fExpectReply && request->GetType() == fExpectedReply) {
		fDone = true;
		return B_OK;
	}

	switch (request->GetType()) {
		// FS
		case MOUNT_VOLUME_REQUEST:
			return _HandleRequest((MountVolumeRequest*)request);
		case UNMOUNT_VOLUME_REQUEST:
			return _HandleRequest((UnmountVolumeRequest*)request);
		case SYNC_VOLUME_REQUEST:
			return _HandleRequest((SyncVolumeRequest*)request);
		case READ_FS_INFO_REQUEST:
			return _HandleRequest((ReadFSInfoRequest*)request);
		case WRITE_FS_INFO_REQUEST:
			return _HandleRequest((WriteFSInfoRequest*)request);

		// vnodes
		case LOOKUP_REQUEST:
			return _HandleRequest((LookupRequest*)request);
		case GET_VNODE_NAME_REQUEST:
			return _HandleRequest((GetVNodeNameRequest*)request);
		case READ_VNODE_REQUEST:
			return _HandleRequest((ReadVNodeRequest*)request);
		case WRITE_VNODE_REQUEST:
			return _HandleRequest((WriteVNodeRequest*)request);
		case FS_REMOVE_VNODE_REQUEST:
			return _HandleRequest((FSRemoveVNodeRequest*)request);

		// asynchronous I/O
		case DO_IO_REQUEST:
			return _HandleRequest((DoIORequest*)request);
		case CANCEL_IO_REQUEST:
			return _HandleRequest((CancelIORequest*)request);
		case ITERATIVE_IO_GET_VECS_REQUEST:
			return _HandleRequest((IterativeIOGetVecsRequest*)request);
		case ITERATIVE_IO_FINISHED_REQUEST:
			return _HandleRequest((IterativeIOFinishedRequest*)request);

		// nodes
		case IOCTL_REQUEST:
			return _HandleRequest((IOCtlRequest*)request);
		case SET_FLAGS_REQUEST:
			return _HandleRequest((SetFlagsRequest*)request);
		case SELECT_REQUEST:
			return _HandleRequest((SelectRequest*)request);
		case DESELECT_REQUEST:
			return _HandleRequest((DeselectRequest*)request);
		case FSYNC_REQUEST:
			return _HandleRequest((FSyncRequest*)request);
		case READ_SYMLINK_REQUEST:
			return _HandleRequest((ReadSymlinkRequest*)request);
		case CREATE_SYMLINK_REQUEST:
			return _HandleRequest((CreateSymlinkRequest*)request);
		case LINK_REQUEST:
			return _HandleRequest((LinkRequest*)request);
		case UNLINK_REQUEST:
			return _HandleRequest((UnlinkRequest*)request);
		case RENAME_REQUEST:
			return _HandleRequest((RenameRequest*)request);
		case ACCESS_REQUEST:
			return _HandleRequest((AccessRequest*)request);
		case READ_STAT_REQUEST:
			return _HandleRequest((ReadStatRequest*)request);
		case WRITE_STAT_REQUEST:
			return _HandleRequest((WriteStatRequest*)request);

		// files
		case CREATE_REQUEST:
			return _HandleRequest((CreateRequest*)request);
		case OPEN_REQUEST:
			return _HandleRequest((OpenRequest*)request);
		case CLOSE_REQUEST:
			return _HandleRequest((CloseRequest*)request);
		case FREE_COOKIE_REQUEST:
			return _HandleRequest((FreeCookieRequest*)request);
		case READ_REQUEST:
			return _HandleRequest((ReadRequest*)request);
		case WRITE_REQUEST:
			return _HandleRequest((WriteRequest*)request);

		// directories
		case CREATE_DIR_REQUEST:
			return _HandleRequest((CreateDirRequest*)request);
		case REMOVE_DIR_REQUEST:
			return _HandleRequest((RemoveDirRequest*)request);
		case OPEN_DIR_REQUEST:
			return _HandleRequest((OpenDirRequest*)request);
		case CLOSE_DIR_REQUEST:
			return _HandleRequest((CloseDirRequest*)request);
		case FREE_DIR_COOKIE_REQUEST:
			return _HandleRequest((FreeDirCookieRequest*)request);
		case READ_DIR_REQUEST:
			return _HandleRequest((ReadDirRequest*)request);
		case REWIND_DIR_REQUEST:
			return _HandleRequest((RewindDirRequest*)request);

		// attribute directories
		case OPEN_ATTR_DIR_REQUEST:
			return _HandleRequest((OpenAttrDirRequest*)request);
		case CLOSE_ATTR_DIR_REQUEST:
			return _HandleRequest((CloseAttrDirRequest*)request);
		case FREE_ATTR_DIR_COOKIE_REQUEST:
			return _HandleRequest((FreeAttrDirCookieRequest*)request);
		case READ_ATTR_DIR_REQUEST:
			return _HandleRequest((ReadAttrDirRequest*)request);
		case REWIND_ATTR_DIR_REQUEST:
			return _HandleRequest((RewindAttrDirRequest*)request);

		// attributes
		case CREATE_ATTR_REQUEST:
			return _HandleRequest((CreateAttrRequest*)request);
		case OPEN_ATTR_REQUEST:
			return _HandleRequest((OpenAttrRequest*)request);
		case CLOSE_ATTR_REQUEST:
			return _HandleRequest((CloseAttrRequest*)request);
		case FREE_ATTR_COOKIE_REQUEST:
			return _HandleRequest((FreeAttrCookieRequest*)request);
		case READ_ATTR_REQUEST:
			return _HandleRequest((ReadAttrRequest*)request);
		case WRITE_ATTR_REQUEST:
			return _HandleRequest((WriteAttrRequest*)request);
		case READ_ATTR_STAT_REQUEST:
			return _HandleRequest((ReadAttrStatRequest*)request);
		case WRITE_ATTR_STAT_REQUEST:
			return _HandleRequest((WriteAttrStatRequest*)request);
		case RENAME_ATTR_REQUEST:
			return _HandleRequest((RenameAttrRequest*)request);
		case REMOVE_ATTR_REQUEST:
			return _HandleRequest((RemoveAttrRequest*)request);

		// indices
		case OPEN_INDEX_DIR_REQUEST:
			return _HandleRequest((OpenIndexDirRequest*)request);
		case CLOSE_INDEX_DIR_REQUEST:
			return _HandleRequest((CloseIndexDirRequest*)request);
		case FREE_INDEX_DIR_COOKIE_REQUEST:
			return _HandleRequest((FreeIndexDirCookieRequest*)request);
		case READ_INDEX_DIR_REQUEST:
			return _HandleRequest((ReadIndexDirRequest*)request);
		case REWIND_INDEX_DIR_REQUEST:
			return _HandleRequest((RewindIndexDirRequest*)request);
		case CREATE_INDEX_REQUEST:
			return _HandleRequest((CreateIndexRequest*)request);
		case REMOVE_INDEX_REQUEST:
			return _HandleRequest((RemoveIndexRequest*)request);
		case READ_INDEX_STAT_REQUEST:
			return _HandleRequest((ReadIndexStatRequest*)request);

		// queries
		case OPEN_QUERY_REQUEST:
			return _HandleRequest((OpenQueryRequest*)request);
		case CLOSE_QUERY_REQUEST:
			return _HandleRequest((CloseQueryRequest*)request);
		case FREE_QUERY_COOKIE_REQUEST:
			return _HandleRequest((FreeQueryCookieRequest*)request);
		case READ_QUERY_REQUEST:
			return _HandleRequest((ReadQueryRequest*)request);
		case REWIND_QUERY_REQUEST:
			return _HandleRequest((RewindQueryRequest*)request);

		// node monitoring
		case NODE_MONITORING_EVENT_REQUEST:
			return _HandleRequest((NodeMonitoringEventRequest*)request);
	}
	PRINT(("UserlandRequestHandler::HandleRequest(): unexpected request: %"
		B_PRIu32 "\n", request->GetType()));
	return B_BAD_DATA;
}


// #pragma mark - FS


// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(MountVolumeRequest* request)
{
	// check and execute the request
	status_t result = B_OK;

	// if the device path is relative, make it absolute by appending the
	// provided CWD
	const char* device = (const char*)request->device.GetData();
	char stackDevice[B_PATH_NAME_LENGTH];
	if (result == B_OK) {
		if (device && device[0] != '/') {
			if (const char* cwd = (const char*)request->cwd.GetData()) {
				int32 deviceLen = strlen(device);
				int32 cwdLen = strlen(cwd);
				if (cwdLen + 1 + deviceLen < (int32)sizeof(stackDevice)) {
					strcpy(stackDevice, cwd);
					strcat(stackDevice, "/");
					strcat(stackDevice, device);
					device = stackDevice;
				} else
					result = B_NAME_TOO_LONG;
			} else
				result = B_BAD_VALUE;
		}
	}

	// create the volume
	Volume* volume = NULL;
	if (result == B_OK)
		result = fFileSystem->CreateVolume(&volume, request->nsid);

	// mount it
	ino_t rootID;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->Mount(device, request->flags,
			(const char*)request->parameters.GetData(), &rootID);
		if (result != B_OK)
			fFileSystem->DeleteVolume(volume);
	}
	if (result != B_OK)
		volume = NULL;

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	MountVolumeReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	reply->error = result;
	reply->volume = volume;
	reply->rootID = rootID;
	if (volume != NULL)
		volume->GetCapabilities(reply->capabilities);

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(UnmountVolumeRequest* request)
{
	// check and execute the request
	status_t result = B_BAD_VALUE;
	if (Volume* volume = (Volume*)request->volume) {
		RequestThreadContext context(volume, request);
		result = volume->Unmount();
		fFileSystem->DeleteVolume(volume);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	UnmountVolumeReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(SyncVolumeRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->Sync();
	}
	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	SyncVolumeReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(ReadFSInfoRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	fs_info info;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->ReadFSInfo(&info);
	}
	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	ReadFSInfoReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	reply->error = result;
	reply->info = info;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(WriteFSInfoRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->WriteFSInfo(&request->info, request->mask);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	WriteFSInfoReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}


// #pragma mark - vnodes


// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(LookupRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	ino_t vnid = 0;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->Lookup(request->node,
			(const char*)request->entryName.GetData(), &vnid);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	LookupReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->vnid = vnid;
	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(GetVNodeNameRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	void* node = request->node;
	size_t bufferSize = request->size;

	// allocate the reply
	RequestAllocator allocator(fPort->GetPort());
	GetVNodeNameReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	char* buffer;
	if (result == B_OK) {
		result = allocator.AllocateAddress(reply->buffer, bufferSize, 1,
			(void**)&buffer, true);
	}

	// execute the request
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->GetVNodeName(node, buffer, bufferSize);
	}

	// reconstruct the reply, in case it has been overwritten
	reply = new(reply) GetVNodeNameReply;

	// send the reply
	reply->error = result;
	return _SendReply(allocator, (result == B_OK));
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(ReadVNodeRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	void* node;
	int type;
	uint32 flags;
	FSVNodeCapabilities capabilities;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->ReadVNode(request->vnid, request->reenter, &node,
			&type, &flags, &capabilities);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	ReadVNodeReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;
	reply->node = node;
	reply->type = type;
	reply->flags = flags;
	reply->capabilities = capabilities;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(WriteVNodeRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->WriteVNode(request->node, request->reenter);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	WriteVNodeReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(FSRemoveVNodeRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->RemoveVNode(request->node, request->reenter);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	FSRemoveVNodeReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}


// #pragma mark - asynchronous I/O


status_t
UserlandRequestHandler::_HandleRequest(DoIORequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		IORequestInfo requestInfo(request->request, request->isWrite,
			request->offset, request->length, request->isVIP);
		result = volume->DoIO(request->node, request->fileCookie, requestInfo);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	DoIOReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}


status_t
UserlandRequestHandler::_HandleRequest(CancelIORequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->CancelIO(request->node, request->fileCookie,
			request->request);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	CancelIOReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}


// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(IterativeIOGetVecsRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	file_io_vec vecs[IterativeIOGetVecsReply::MAX_VECS];
	size_t vecCount = IterativeIOGetVecsReply::MAX_VECS;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->IterativeIOGetVecs(request->cookie, request->request,
			request->offset, request->size, vecs, &vecCount);
		if (result == B_OK) {
			vecCount = std::min(vecCount,
				(size_t)IterativeIOGetVecsReply::MAX_VECS);
		}
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	IterativeIOGetVecsReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;
	if (result == B_OK) {
		memcpy(reply->vecs, vecs, vecCount * sizeof(file_io_vec));
		reply->vecCount = vecCount;
	} else
		reply->vecCount = 0;

	// send the reply
	return _SendReply(allocator, false);
}


// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(IterativeIOFinishedRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->IterativeIOFinished(request->cookie, request->request,
			request->status, request->partialTransfer,
			request->bytesTransferred);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	IterativeIOFinishedReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}


// #pragma mark - nodes


// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(IOCtlRequest* request)
{
	// get the request parameters
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	void* buffer = request->bufferParameter;
	size_t len = request->lenParameter;
	bool isBuffer = request->isBuffer;
	int32 bufferSize = 0;
	int32 writeSize = request->writeSize;
	if (isBuffer) {
		buffer = request->buffer.GetData();
		bufferSize = request->buffer.GetSize();
	}

	// allocate the reply
	RequestAllocator allocator(fPort->GetPort());
	IOCtlReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	// allocate the writable buffer for the call
	void* writeBuffer = NULL;
	if (result == B_OK && isBuffer && writeSize > 0) {
		if (writeSize < bufferSize)
			writeSize = bufferSize;
		result = allocator.AllocateAddress(reply->buffer, writeSize, 8,
			(void**)&writeBuffer, true);
		if (result == B_OK && bufferSize > 0)
			memcpy(writeBuffer, buffer, bufferSize);
		buffer = writeBuffer;
	}

	// execute the request
	status_t ioctlError = B_OK;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		ioctlError = volume->IOCtl(request->node, request->fileCookie,
			request->command, buffer, len);
	}

	// reconstruct the reply, in case it has been overwritten
	reply = new(reply) IOCtlReply;

	// send the reply
	reply->error = result;
	reply->ioctlError = ioctlError;
	return _SendReply(allocator, (result == B_OK && writeBuffer));
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(SetFlagsRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->SetFlags(request->node, request->fileCookie,
			request->flags);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	SetFlagsReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(SelectRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->Select(request->node, request->fileCookie,
			request->event, request->sync);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	SelectReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(DeselectRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->Deselect(request->node, request->fileCookie,
			request->event, request->sync);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	DeselectReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(FSyncRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->FSync(request->node);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	FSyncReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(ReadSymlinkRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	void* node = request->node;
	size_t bufferSize = request->size;

	// allocate the reply
	RequestAllocator allocator(fPort->GetPort());
	ReadSymlinkReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	char* buffer;
	if (result == B_OK) {
		result = allocator.AllocateAddress(reply->buffer, bufferSize, 1,
			(void**)&buffer, true);
	}

	// execute the request
	size_t bytesRead;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->ReadSymlink(node, buffer, bufferSize, &bytesRead);
	}

	// reconstruct the reply, in case it has been overwritten
	reply = new(reply) ReadSymlinkReply;

	// send the reply
	reply->error = result;
	reply->bytesRead = bytesRead;
	return _SendReply(allocator, (result == B_OK));
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(CreateSymlinkRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->CreateSymlink(request->node,
			(const char*)request->name.GetData(),
			(const char*)request->target.GetData(), request->mode);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	CreateSymlinkReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(LinkRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->Link(request->node,
			(const char*)request->name.GetData(), request->target);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	LinkReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(UnlinkRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->Unlink(request->node,
			(const char*)request->name.GetData());
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	UnlinkReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(RenameRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->Rename(request->oldDir,
			(const char*)request->oldName.GetData(), request->newDir,
			(const char*)request->newName.GetData());
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	RenameReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(AccessRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->Access(request->node, request->mode);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	AccessReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(ReadStatRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	struct stat st;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->ReadStat(request->node, &st);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	ReadStatReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;
	reply->st = st;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(WriteStatRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->WriteStat(request->node, &request->st, request->mask);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	WriteStatReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}


// #pragma mark - files


// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(CreateRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	ino_t vnid;
	void* fileCookie;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->Create(request->node,
			(const char*)request->name.GetData(), request->openMode,
			request->mode, &fileCookie, &vnid);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	CreateReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;
	reply->vnid = vnid;
	reply->fileCookie = fileCookie;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(OpenRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	void* fileCookie;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->Open(request->node, request->openMode, &fileCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	OpenReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;
	reply->fileCookie = fileCookie;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(CloseRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->Close(request->node, request->fileCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	CloseReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(FreeCookieRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->FreeCookie(request->node, request->fileCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	FreeCookieReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(ReadRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	void* node = request->node;
	void* fileCookie = request->fileCookie;
	off_t pos = request->pos;
	size_t size = request->size;

	// allocate the reply
	RequestAllocator allocator(fPort->GetPort());
	ReadReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	void* buffer;
	if (result == B_OK) {
		result = allocator.AllocateAddress(reply->buffer, size, 1, &buffer,
			true);
	}

	// execute the request
	size_t bytesRead;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->Read(node, fileCookie, pos, buffer, size, &bytesRead);
	}

	// reconstruct the reply, in case it has been overwritten
	reply = new(reply) ReadReply;

	// send the reply
	reply->error = result;
	reply->bytesRead = bytesRead;
	return _SendReply(allocator, (result == B_OK));
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(WriteRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	size_t bytesWritten;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->Write(request->node, request->fileCookie,
			request->pos, request->buffer.GetData(), request->buffer.GetSize(),
			&bytesWritten);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	WriteReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;
	reply->bytesWritten = bytesWritten;

	// send the reply
	return _SendReply(allocator, false);
}


// #pragma mark - directories


// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(CreateDirRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->CreateDir(request->node,
			(const char*)request->name.GetData(), request->mode);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	CreateDirReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(RemoveDirRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->RemoveDir(request->node,
			(const char*)request->name.GetData());
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	RemoveDirReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(OpenDirRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	void* dirCookie = NULL;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->OpenDir(request->node, &dirCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	OpenDirReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;
	reply->dirCookie = dirCookie;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(CloseDirRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->CloseDir(request->node, request->dirCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	CloseDirReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(FreeDirCookieRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->FreeDirCookie(request->node, request->dirCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	FreeDirCookieReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(ReadDirRequest* request)
{
	// check the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	void* node = request->node;
	void* dirCookie = request->dirCookie;
	size_t bufferSize = request->bufferSize;
	uint32 count = request->count;

	// allocate the reply
	RequestAllocator allocator(fPort->GetPort());
	ReadDirReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	void* buffer;
	if (result == B_OK) {
		result = allocator.AllocateAddress(reply->buffer, bufferSize, 1,
			&buffer, true);
	}

	// execute the request
	uint32 countRead;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->ReadDir(node, dirCookie, buffer, bufferSize, count,
			&countRead);
	}

	D(
		if (result == B_OK && countRead > 0) {
			dirent* entry = (dirent*)buffer;
			PRINT(("  entry: d_dev: %" B_PRIdDEV ", d_pdev: %" B_PRIdDEV ", "
				"d_ino: %" B_PRIdINO ", d_pino: %" B_PRIdINO ", "
				"d_reclen: %hu, d_name: %.32s\n",
			entry->d_dev, entry->d_pdev,
			entry->d_ino, entry->d_pino,
			entry->d_reclen, entry->d_name));
		}
	)

	// reconstruct the reply, in case it has been overwritten
	reply = new(reply) ReadDirReply;

	// send the reply
	reply->error = result;
	reply->count = countRead;
	return _SendReply(allocator, (result == B_OK));
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(RewindDirRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->RewindDir(request->node, request->dirCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	RewindDirReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}


// #pragma mark - attribute directories


// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(OpenAttrDirRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	void* attrDirCookie;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->OpenAttrDir(request->node, &attrDirCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	OpenAttrDirReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;
	reply->attrDirCookie = attrDirCookie;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(CloseAttrDirRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->CloseAttrDir(request->node, request->attrDirCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	CloseAttrDirReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(FreeAttrDirCookieRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->FreeAttrDirCookie(request->node,
			request->attrDirCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	FreeAttrDirCookieReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(ReadAttrDirRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	void* node = request->node;
	void* attrDirCookie = request->attrDirCookie;
	size_t bufferSize = request->bufferSize;
	uint32 count = request->count;

	// allocate the reply
	RequestAllocator allocator(fPort->GetPort());
	ReadAttrDirReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	void* buffer;
	if (result == B_OK) {
		result = allocator.AllocateAddress(reply->buffer, bufferSize, 1,
			&buffer, true);
	}

	// execute the request
	uint32 countRead;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->ReadAttrDir(node, attrDirCookie, buffer, bufferSize,
			count, &countRead);
	}

	// reconstruct the reply, in case it has been overwritten
	reply = new(reply) ReadAttrDirReply;

	// send the reply
	reply->error = result;
	reply->count = countRead;
	return _SendReply(allocator, (result == B_OK));
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(RewindAttrDirRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->RewindAttrDir(request->node, request->attrDirCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	RewindAttrDirReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}


// #pragma mark - attributes


// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(CreateAttrRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	void* attrCookie;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->CreateAttr(request->node,
			(const char*)request->name.GetData(), request->type,
			request->openMode, &attrCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	CreateAttrReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;
	reply->attrCookie = attrCookie;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(OpenAttrRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	void* attrCookie;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->OpenAttr(request->node,
			(const char*)request->name.GetData(), request->openMode,
			&attrCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	OpenAttrReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;
	reply->attrCookie = attrCookie;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(CloseAttrRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->CloseAttr(request->node, request->attrCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	CloseAttrReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(FreeAttrCookieRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->FreeAttrCookie(request->node, request->attrCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	FreeAttrCookieReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(ReadAttrRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	void* node = request->node;
	void* attrCookie = request->attrCookie;
	off_t pos = request->pos;
	size_t size = request->size;

	// allocate the reply
	RequestAllocator allocator(fPort->GetPort());
	ReadAttrReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	void* buffer;
	if (result == B_OK) {
		result = allocator.AllocateAddress(reply->buffer, size, 1, &buffer,
			true);
	}

	// execute the request
	size_t bytesRead;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->ReadAttr(node, attrCookie, pos, buffer, size,
			&bytesRead);
	}

	// reconstruct the reply, in case it has been overwritten
	reply = new(reply) ReadAttrReply;

	// send the reply
	reply->error = result;
	reply->bytesRead = bytesRead;
	return _SendReply(allocator, (result == B_OK));
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(WriteAttrRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	size_t bytesWritten;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->WriteAttr(request->node, request->attrCookie,
			request->pos, request->buffer.GetData(), request->buffer.GetSize(),
			&bytesWritten);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	WriteAttrReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;
	reply->bytesWritten = bytesWritten;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(ReadAttrStatRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	struct stat st;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->ReadAttrStat(request->node, request->attrCookie,
			&st);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	ReadAttrStatReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;
	reply->st = st;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(WriteAttrStatRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->WriteAttrStat(request->node, request->attrCookie,
			&request->st, request->mask);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	WriteAttrStatReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(RenameAttrRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->RenameAttr(
			request->oldNode, (const char*)request->oldName.GetData(),
			request->newNode, (const char*)request->newName.GetData());
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	RenameAttrReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(RemoveAttrRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->RemoveAttr(request->node,
			(const char*)request->name.GetData());
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	RemoveAttrReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}


// #pragma mark - indices


// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(OpenIndexDirRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	void* indexDirCookie;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->OpenIndexDir(&indexDirCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	OpenIndexDirReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;
	reply->indexDirCookie = indexDirCookie;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(CloseIndexDirRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->CloseIndexDir(request->indexDirCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	CloseIndexDirReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(FreeIndexDirCookieRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->FreeIndexDirCookie(request->indexDirCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	FreeIndexDirCookieReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(ReadIndexDirRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	void* indexDirCookie = request->indexDirCookie;
	size_t bufferSize = request->bufferSize;
	uint32 count = request->count;

	// allocate the reply
	RequestAllocator allocator(fPort->GetPort());
	ReadIndexDirReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	void* buffer;
	if (result == B_OK) {
		result = allocator.AllocateAddress(reply->buffer, bufferSize, 1,
			&buffer, true);
	}

	// execute the request
	uint32 countRead;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->ReadIndexDir(indexDirCookie, buffer, bufferSize,
			count, &countRead);
	}

	// reconstruct the reply, in case it has been overwritten
	reply = new(reply) ReadIndexDirReply;

	// send the reply
	reply->error = result;
	reply->count = countRead;
	return _SendReply(allocator, (result == B_OK));
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(RewindIndexDirRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->RewindIndexDir(request->indexDirCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	RewindIndexDirReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(CreateIndexRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->CreateIndex((const char*)request->name.GetData(),
			request->type, request->flags);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	CreateIndexReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(RemoveIndexRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->RemoveIndex((const char*)request->name.GetData());
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	RemoveIndexReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(ReadIndexStatRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	struct stat st;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->ReadIndexStat((const char*)request->name.GetData(),
			&st);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	ReadIndexStatReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;
	reply->st = st;

	// send the reply
	return _SendReply(allocator, false);
}


// #pragma mark - queries


// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(OpenQueryRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	void* queryCookie;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->OpenQuery((const char*)request->queryString.GetData(),
			request->flags, request->port, request->token, &queryCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	OpenQueryReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;
	reply->queryCookie = queryCookie;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(CloseQueryRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->CloseQuery(request->queryCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	CloseQueryReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(FreeQueryCookieRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->FreeQueryCookie(request->queryCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	FreeQueryCookieReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(ReadQueryRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	void* queryCookie = request->queryCookie;
	size_t bufferSize = request->bufferSize;
	uint32 count = request->count;

	// allocate the reply
	RequestAllocator allocator(fPort->GetPort());
	ReadQueryReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	void* buffer;
	if (result == B_OK) {
		result = allocator.AllocateAddress(reply->buffer, bufferSize, 1,
			&buffer, true);
	}

	// execute the request
	uint32 countRead;
	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->ReadQuery(queryCookie, buffer, bufferSize,
			count, &countRead);
	}

	// reconstruct the reply, in case it has been overwritten
	reply = new(reply) ReadQueryReply;

	// send the reply
	reply->error = result;
	reply->count = countRead;
	return _SendReply(allocator, (result == B_OK));
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(RewindQueryRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	Volume* volume = (Volume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;

	if (result == B_OK) {
		RequestThreadContext context(volume, request);
		result = volume->RewindQuery(request->queryCookie);
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	RewindQueryReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = result;

	// send the reply
	return _SendReply(allocator, false);
}


// #pragma mark - node monitoring


// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(NodeMonitoringEventRequest* request)
{
	// check and execute the request
	KMessage event;
	event.SetTo(request->event.GetData(), request->event.GetSize());
	((NotificationListener*)request->listener)->EventOccurred(
		*(NotificationService*)NULL, &event);

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	NodeMonitoringEventReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);

	reply->error = B_OK;

	// send the reply
	return _SendReply(allocator, false);
}


// #pragma mark - other


// _SendReply
status_t
UserlandRequestHandler::_SendReply(RequestAllocator& allocator,
	bool expectsReceipt)
{
	if (expectsReceipt) {
		SingleReplyRequestHandler handler(RECEIPT_ACK_REPLY);
		return fPort->SendRequest(&allocator, &handler);
	} else
		return fPort->SendRequest(&allocator);
}

