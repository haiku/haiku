// UserlandRequestHandler.cpp

#include "AutoDeleter.h"
#include "Compatibility.h"
#include "Debug.h"
#include "UserVolume.h"
#include "RequestPort.h"
#include "Requests.h"
#include "RequestThread.h"
#include "SingleReplyRequestHandler.h"
#include "UserFileSystem.h"
#include "UserlandRequestHandler.h"

// constructor
UserlandRequestHandler::UserlandRequestHandler(UserFileSystem* fileSystem)
	: RequestHandler(),
	  fFileSystem(fileSystem),
	  fExpectReply(false),
	  fExpectedReply(0)
{
}

// constructor
UserlandRequestHandler::UserlandRequestHandler(UserFileSystem* fileSystem,
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
		case READ_FS_STAT_REQUEST:
			return _HandleRequest((ReadFSStatRequest*)request);
		case WRITE_FS_STAT_REQUEST:
			return _HandleRequest((WriteFSStatRequest*)request);
		// vnodes
		case READ_VNODE_REQUEST:
			return _HandleRequest((ReadVNodeRequest*)request);
		case WRITE_VNODE_REQUEST:
			return _HandleRequest((WriteVNodeRequest*)request);
		case FS_REMOVE_VNODE_REQUEST:
			return _HandleRequest((FSRemoveVNodeRequest*)request);
		// nodes
		case FSYNC_REQUEST:
			return _HandleRequest((FSyncRequest*)request);
		case READ_STAT_REQUEST:
			return _HandleRequest((ReadStatRequest*)request);
		case WRITE_STAT_REQUEST:
			return _HandleRequest((WriteStatRequest*)request);
		case ACCESS_REQUEST:
			return _HandleRequest((AccessRequest*)request);
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
		case IOCTL_REQUEST:
			return _HandleRequest((IOCtlRequest*)request);
		case SET_FLAGS_REQUEST:
			return _HandleRequest((SetFlagsRequest*)request);
		case SELECT_REQUEST:
			return _HandleRequest((SelectRequest*)request);
		case DESELECT_REQUEST:
			return _HandleRequest((DeselectRequest*)request);
		// hard links / symlinks
		case LINK_REQUEST:
			return _HandleRequest((LinkRequest*)request);
		case UNLINK_REQUEST:
			return _HandleRequest((UnlinkRequest*)request);
		case SYMLINK_REQUEST:
			return _HandleRequest((SymlinkRequest*)request);
		case READ_LINK_REQUEST:
			return _HandleRequest((ReadLinkRequest*)request);
		case RENAME_REQUEST:
			return _HandleRequest((RenameRequest*)request);
		// directories
		case MKDIR_REQUEST:
			return _HandleRequest((MkDirRequest*)request);
		case RMDIR_REQUEST:
			return _HandleRequest((RmDirRequest*)request);
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
		case WALK_REQUEST:
			return _HandleRequest((WalkRequest*)request);
		// attributes
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
		case READ_ATTR_REQUEST:
			return _HandleRequest((ReadAttrRequest*)request);
		case WRITE_ATTR_REQUEST:
			return _HandleRequest((WriteAttrRequest*)request);
		case REMOVE_ATTR_REQUEST:
			return _HandleRequest((RemoveAttrRequest*)request);
		case RENAME_ATTR_REQUEST:
			return _HandleRequest((RenameAttrRequest*)request);
		case STAT_ATTR_REQUEST:
			return _HandleRequest((StatAttrRequest*)request);
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
		case RENAME_INDEX_REQUEST:
			return _HandleRequest((RenameIndexRequest*)request);
		case STAT_INDEX_REQUEST:
			return _HandleRequest((StatIndexRequest*)request);
		// queries
		case OPEN_QUERY_REQUEST:
			return _HandleRequest((OpenQueryRequest*)request);
		case CLOSE_QUERY_REQUEST:
			return _HandleRequest((CloseQueryRequest*)request);
		case FREE_QUERY_COOKIE_REQUEST:
			return _HandleRequest((FreeQueryCookieRequest*)request);
		case READ_QUERY_REQUEST:
			return _HandleRequest((ReadQueryRequest*)request);
	}
PRINT(("UserlandRequestHandler::HandleRequest(): unexpected request: %lu\n",
request->GetType()));
	return B_BAD_DATA;
}


// #pragma mark -
// #pragma mark ----- FS -----

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
	UserVolume* volume = NULL;
	if (result == B_OK)
		result = fFileSystem->CreateVolume(&volume, request->nsid);

	// mount it
	vnode_id rootID;
	if (result == B_OK) {
		RequestThreadContext context(volume);
		result = volume->Mount(device, request->flags,
			(const char*)request->parameters.GetData(),
			request->parameters.GetSize(), &rootID);
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

	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(UnmountVolumeRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
		result = volume->Unmount();
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
UserlandRequestHandler::_HandleRequest(ReadFSStatRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	fs_info info;
	if (result == B_OK) {
		RequestThreadContext context(volume);
		result = volume->ReadFSStat(&info);
	}
	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	ReadFSStatReply* reply;
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
UserlandRequestHandler::_HandleRequest(WriteFSStatRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
		result = volume->WriteFSStat(&request->info, request->mask);
	}
	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	WriteFSStatReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	reply->error = result;
	// send the reply
	return _SendReply(allocator, false);
}


// #pragma mark -
// #pragma mark ----- vnodes -----

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(ReadVNodeRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	void* node;
	if (result == B_OK) {
		RequestThreadContext context(volume);
		result = volume->ReadVNode(request->vnid, request->reenter, &node);
	}
	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	ReadVNodeReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	reply->error = result;
	reply->node = node;
	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(WriteVNodeRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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


// #pragma mark -
// #pragma mark ----- nodes -----

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(FSyncRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
UserlandRequestHandler::_HandleRequest(ReadStatRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	struct stat st;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(AccessRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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


// #pragma mark -
// #pragma mark ----- files -----

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(CreateRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	vnode_id vnid;
	void* fileCookie;
	if (result == B_OK) {
		RequestThreadContext context(volume);
		result = volume->Create(request->node,
			(const char*)request->name.GetData(), request->openMode,
			request->mode, &vnid, &fileCookie);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	void* fileCookie;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
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
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	size_t bytesWritten;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(IOCtlRequest* request)
{
	// get the request parameters
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
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
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
		result = volume->Select(request->node, request->fileCookie,
			request->event, request->ref, request->sync);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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


// #pragma mark -
// #pragma mark ----- hard links / symlinks -----

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(LinkRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
UserlandRequestHandler::_HandleRequest(SymlinkRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
		result = volume->Symlink(request->node,
			(const char*)request->name.GetData(),
			(const char*)request->target.GetData());
	}
	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	SymlinkReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	reply->error = result;
	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(ReadLinkRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	void* node = request->node;
	size_t bufferSize = request->size;
	// allocate the reply
	RequestAllocator allocator(fPort->GetPort());
	ReadLinkReply* reply;
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
		RequestThreadContext context(volume);
		result = volume->ReadLink(node, buffer, bufferSize, &bytesRead);
	}
	// reconstruct the reply, in case it has been overwritten
	reply = new(reply) ReadLinkReply;
	// send the reply
	reply->error = result;
	reply->bytesRead = bytesRead;
	return _SendReply(allocator, (result == B_OK));
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(RenameRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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


// #pragma mark -
// #pragma mark ----- directories -----

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(MkDirRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
		result = volume->MkDir(request->node,
			(const char*)request->name.GetData(), request->mode);
	}
	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	MkDirReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	reply->error = result;
	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(RmDirRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
		result = volume->RmDir(request->node,
			(const char*)request->name.GetData());
	}
	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	RmDirReply* reply;
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	void* dirCookie;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	void* node = request->node;
	void* dirCookie = request->dirCookie;
	size_t bufferSize = request->bufferSize;
	int32 count = request->count;
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
	int32 countRead;
	if (result == B_OK) {
		RequestThreadContext context(volume);
		result = volume->ReadDir(node, dirCookie, buffer, bufferSize, count,
			&countRead);
	}
D(
if (result == B_OK && countRead > 0) {
	dirent* entry = (dirent*)buffer;
	PRINT(("  entry: d_dev: %ld, d_pdev: %ld, d_ino: %Ld, d_pino: %Ld, "
		"d_reclen: %hu, d_name: %.32s\n",
		entry->d_dev, entry->d_pdev, entry->d_ino, entry->d_pino,
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(WalkRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	vnode_id vnid;
	char* resolvedPath = NULL;
	if (result == B_OK) {
		RequestThreadContext context(volume);
		result = volume->Walk(request->node,
			(const char*)request->entryName.GetData(),
			(request->traverseLink ? &resolvedPath : NULL), &vnid);
	}
	MemoryDeleter _(resolvedPath);
	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	WalkReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	if (result == B_OK && resolvedPath)
		result = allocator.AllocateString(reply->resolvedPath, resolvedPath);
	reply->vnid = vnid;
	reply->error = result;
	// send the reply
	return _SendReply(allocator, (result == B_OK && resolvedPath));
}


// #pragma mark -
// #pragma mark ----- attributes -----

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(OpenAttrDirRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	void* attrDirCookie;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	void* node = request->node;
	void* attrDirCookie = request->attrDirCookie;
	size_t bufferSize = request->bufferSize;
	int32 count = request->count;
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
	int32 countRead;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(ReadAttrRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	void* node = request->node;
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
		RequestThreadContext context(volume);
		result = volume->ReadAttr(node, (const char*)request->name.GetData(),
			request->type, pos, buffer, size, &bytesRead);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	size_t bytesWritten;
	if (result == B_OK) {
		RequestThreadContext context(volume);
		result = volume->WriteAttr(request->node,
			(const char*)request->name.GetData(), request->type, request->pos,
			request->buffer.GetData(), request->buffer.GetSize(),
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
UserlandRequestHandler::_HandleRequest(RemoveAttrRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(RenameAttrRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
		result = volume->RenameAttr(request->node,
			(const char*)request->oldName.GetData(),
			(const char*)request->newName.GetData());
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
UserlandRequestHandler::_HandleRequest(StatAttrRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	attr_info info;
	if (result == B_OK) {
		RequestThreadContext context(volume);
		result = volume->StatAttr(request->node,
			(const char*)request->name.GetData(), &info);
	}
	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	StatAttrReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	reply->error = result;
	reply->info = info;
	// send the reply
	return _SendReply(allocator, false);
}


// #pragma mark -
// #pragma mark ----- indices -----

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(OpenIndexDirRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	void* indexDirCookie;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	void* indexDirCookie = request->indexDirCookie;
	size_t bufferSize = request->bufferSize;
	int32 count = request->count;
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
	int32 countRead;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
UserlandRequestHandler::_HandleRequest(RenameIndexRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
		result = volume->RenameIndex((const char*)request->oldName.GetData(),
			(const char*)request->newName.GetData());
	}
	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	RenameIndexReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	reply->error = result;
	// send the reply
	return _SendReply(allocator, false);
}

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(StatIndexRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	index_info info;
	if (result == B_OK) {
		RequestThreadContext context(volume);
		result = volume->StatIndex((const char*)request->name.GetData(), &info);
	}
	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	StatIndexReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	reply->error = result;
	reply->info = info;
	// send the reply
	return _SendReply(allocator, false);
}


// #pragma mark -
// #pragma mark ----- queries -----

// _HandleRequest
status_t
UserlandRequestHandler::_HandleRequest(OpenQueryRequest* request)
{
	// check and execute the request
	status_t result = B_OK;
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	void* queryCookie;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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
	UserVolume* volume = (UserVolume*)request->volume;
	if (!volume)
		result = B_BAD_VALUE;
	void* queryCookie = request->queryCookie;
	size_t bufferSize = request->bufferSize;
	int32 count = request->count;
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
	int32 countRead;
	if (result == B_OK) {
		RequestThreadContext context(volume);
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


// #pragma mark -
// #pragma mark ----- other -----

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

