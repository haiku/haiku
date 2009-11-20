/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_USERLAND_REQUEST_HANDLER_H
#define USERLAND_FS_USERLAND_REQUEST_HANDLER_H

#include "RequestHandler.h"

namespace UserlandFSUtil {

// FS
class MountVolumeRequest;
class UnmountVolumeRequest;
class SyncVolumeRequest;
class ReadFSInfoRequest;
class WriteFSInfoRequest;
// vnodes
class LookupRequest;
class GetVNodeNameRequest;
class ReadVNodeRequest;
class WriteVNodeRequest;
class FSRemoveVNodeRequest;
// asynchronous I/O
class DoIORequest;
class CancelIORequest;
class IterativeIOGetVecsRequest;
class IterativeIOFinishedRequest;
// nodes
class IOCtlRequest;
class SetFlagsRequest;
class SelectRequest;
class DeselectRequest;
class FSyncRequest;
class ReadSymlinkRequest;
class CreateSymlinkRequest;
class LinkRequest;
class UnlinkRequest;
class RenameRequest;
class AccessRequest;
class ReadStatRequest;
class WriteStatRequest;
// files
class CreateRequest;
class OpenRequest;
class CloseRequest;
class FreeCookieRequest;
class ReadRequest;
class WriteRequest;
// directories
class CreateDirRequest;
class RemoveDirRequest;
class OpenDirRequest;
class CloseDirRequest;
class FreeDirCookieRequest;
class ReadDirRequest;
class RewindDirRequest;
// attribute directories
class OpenAttrDirRequest;
class CloseAttrDirRequest;
class FreeAttrDirCookieRequest;
class ReadAttrDirRequest;
class RewindAttrDirRequest;
// attributes
class CreateAttrRequest;
class OpenAttrRequest;
class CloseAttrRequest;
class FreeAttrCookieRequest;
class ReadAttrRequest;
class WriteAttrRequest;
class ReadAttrStatRequest;
class WriteAttrStatRequest;
class RenameAttrRequest;
class RemoveAttrRequest;
// indices
class OpenIndexDirRequest;
class CloseIndexDirRequest;
class FreeIndexDirCookieRequest;
class ReadIndexDirRequest;
class RewindIndexDirRequest;
class CreateIndexRequest;
class RemoveIndexRequest;
class ReadIndexStatRequest;
// queries
class OpenQueryRequest;
class CloseQueryRequest;
class FreeQueryCookieRequest;
class ReadQueryRequest;
class RewindQueryRequest;
// node monitoring
class NodeMonitoringEventRequest;

class RequestAllocator;

}	// namespace UserlandFSUtil

using namespace UserlandFSUtil;

namespace UserlandFS {

class FileSystem;

class UserlandRequestHandler : public RequestHandler {
public:
								UserlandRequestHandler(FileSystem* fileSystem);
								UserlandRequestHandler(FileSystem* fileSystem,
									uint32 expectedReply);
	virtual						~UserlandRequestHandler();

	virtual	status_t			HandleRequest(Request* request);

private:
			// FS
			status_t			_HandleRequest(MountVolumeRequest* request);
			status_t			_HandleRequest(UnmountVolumeRequest* request);
			status_t			_HandleRequest(SyncVolumeRequest* request);
			status_t			_HandleRequest(ReadFSInfoRequest* request);
			status_t			_HandleRequest(WriteFSInfoRequest* request);

			// vnodes
			status_t			_HandleRequest(LookupRequest* request);
			status_t			_HandleRequest(GetVNodeNameRequest* request);
			status_t			_HandleRequest(ReadVNodeRequest* request);
			status_t			_HandleRequest(WriteVNodeRequest* request);
			status_t			_HandleRequest(FSRemoveVNodeRequest* request);

			// asynchronous I/O
			status_t			_HandleRequest(DoIORequest* request);
			status_t			_HandleRequest(CancelIORequest* request);
			status_t			_HandleRequest(
									IterativeIOGetVecsRequest* request);
			status_t			_HandleRequest(
									IterativeIOFinishedRequest* request);

			// nodes
			status_t			_HandleRequest(IOCtlRequest* request);
			status_t			_HandleRequest(SetFlagsRequest* request);
			status_t			_HandleRequest(SelectRequest* request);
			status_t			_HandleRequest(DeselectRequest* request);
			status_t			_HandleRequest(FSyncRequest* request);
			status_t			_HandleRequest(ReadSymlinkRequest* request);
			status_t			_HandleRequest(CreateSymlinkRequest* request);
			status_t			_HandleRequest(LinkRequest* request);
			status_t			_HandleRequest(UnlinkRequest* request);
			status_t			_HandleRequest(RenameRequest* request);
			status_t			_HandleRequest(AccessRequest* request);
			status_t			_HandleRequest(ReadStatRequest* request);
			status_t			_HandleRequest(WriteStatRequest* request);

			// files
			status_t			_HandleRequest(CreateRequest* request);
			status_t			_HandleRequest(OpenRequest* request);
			status_t			_HandleRequest(CloseRequest* request);
			status_t			_HandleRequest(FreeCookieRequest* request);
			status_t			_HandleRequest(ReadRequest* request);
			status_t			_HandleRequest(WriteRequest* request);

			// directories
			status_t			_HandleRequest(CreateDirRequest* request);
			status_t			_HandleRequest(RemoveDirRequest* request);
			status_t			_HandleRequest(OpenDirRequest* request);
			status_t			_HandleRequest(CloseDirRequest* request);
			status_t			_HandleRequest(FreeDirCookieRequest* request);
			status_t			_HandleRequest(ReadDirRequest* request);
			status_t			_HandleRequest(RewindDirRequest* request);

			// attribute directories
			status_t			_HandleRequest(OpenAttrDirRequest* request);
			status_t			_HandleRequest(CloseAttrDirRequest* request);
			status_t			_HandleRequest(
									FreeAttrDirCookieRequest* request);
			status_t			_HandleRequest(ReadAttrDirRequest* request);
			status_t			_HandleRequest(RewindAttrDirRequest* request);

			// attributes
			status_t			_HandleRequest(CreateAttrRequest* request);
			status_t			_HandleRequest(OpenAttrRequest* request);
			status_t			_HandleRequest(CloseAttrRequest* request);
			status_t			_HandleRequest(FreeAttrCookieRequest* request);
			status_t			_HandleRequest(ReadAttrRequest* request);
			status_t			_HandleRequest(WriteAttrRequest* request);
			status_t			_HandleRequest(ReadAttrStatRequest* request);
			status_t			_HandleRequest(WriteAttrStatRequest* request);
			status_t			_HandleRequest(RenameAttrRequest* request);
			status_t			_HandleRequest(RemoveAttrRequest* request);

			// indices
			status_t			_HandleRequest(OpenIndexDirRequest* request);
			status_t			_HandleRequest(CloseIndexDirRequest* request);
			status_t			_HandleRequest(
									FreeIndexDirCookieRequest* request);
			status_t			_HandleRequest(ReadIndexDirRequest* request);
			status_t			_HandleRequest(RewindIndexDirRequest* request);
			status_t			_HandleRequest(CreateIndexRequest* request);
			status_t			_HandleRequest(RemoveIndexRequest* request);
			status_t			_HandleRequest(ReadIndexStatRequest* request);

			// queries
			status_t			_HandleRequest(OpenQueryRequest* request);
			status_t			_HandleRequest(CloseQueryRequest* request);
			status_t			_HandleRequest(FreeQueryCookieRequest* request);
			status_t			_HandleRequest(ReadQueryRequest* request);
			status_t			_HandleRequest(RewindQueryRequest* request);

			// node monitoring
			status_t			_HandleRequest(
									NodeMonitoringEventRequest* request);

			status_t			_SendReply(RequestAllocator& allocator,
									bool expectsReceipt);

private:
			FileSystem*			fFileSystem;
			bool				fExpectReply;
			uint32				fExpectedReply;
};

}	// namespace UserlandFS

using UserlandFS::UserlandRequestHandler;

#endif	// USERLAND_FS_USERLAND_REQUEST_HANDLER_H
