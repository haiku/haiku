// UserlandRequestHandler.h

#ifndef USERLAND_FS_USERLAND_REQUEST_HANDLER_H
#define USERLAND_FS_USERLAND_REQUEST_HANDLER_H

#include "RequestHandler.h"

namespace UserlandFSUtil {

class MountVolumeRequest;
class UnmountVolumeRequest;
class ReadFSStatRequest;
class ReadVNodeRequest;
class WriteVNodeRequest;
class ReadStatRequest;
class AccessRequest;
class OpenRequest;
class CloseRequest;
class FreeCookieRequest;
class ReadRequest;
class WalkRequest;
class OpenDirRequest;
class ReadDirRequest;
class RewindDirRequest;
class CloseDirRequest;
class FreeDirCookieRequest;
class ReadLinkRequest;

}	// namespace UserlandFSUtil

namespace UserlandFS {

class UserFileSystem;

class UserlandRequestHandler : public RequestHandler {
public:
								UserlandRequestHandler(
									UserFileSystem* fileSystem);
								UserlandRequestHandler(
									UserFileSystem* fileSystem,
									uint32 expectedReply);
	virtual						~UserlandRequestHandler();

	virtual	status_t			HandleRequest(Request* request);

private:
			// FS
			status_t			_HandleRequest(MountVolumeRequest* request);
			status_t			_HandleRequest(UnmountVolumeRequest* request);
			status_t			_HandleRequest(SyncVolumeRequest* request);
			status_t			_HandleRequest(ReadFSStatRequest* request);
			status_t			_HandleRequest(WriteFSStatRequest* request);

			// vnodes
			status_t			_HandleRequest(ReadVNodeRequest* request);
			status_t			_HandleRequest(WriteVNodeRequest* request);
			status_t			_HandleRequest(FSRemoveVNodeRequest* request);

			// nodes
			status_t			_HandleRequest(FSyncRequest* request);
			status_t			_HandleRequest(ReadStatRequest* request);
			status_t			_HandleRequest(WriteStatRequest* request);
			status_t			_HandleRequest(AccessRequest* request);

			// files
			status_t			_HandleRequest(CreateRequest* request);
			status_t			_HandleRequest(OpenRequest* request);
			status_t			_HandleRequest(CloseRequest* request);
			status_t			_HandleRequest(FreeCookieRequest* request);
			status_t			_HandleRequest(ReadRequest* request);
			status_t			_HandleRequest(WriteRequest* request);
			status_t			_HandleRequest(IOCtlRequest* request);
			status_t			_HandleRequest(SetFlagsRequest* request);
			status_t			_HandleRequest(SelectRequest* request);
			status_t			_HandleRequest(DeselectRequest* request);

			// hard links / symlinks
			status_t			_HandleRequest(LinkRequest* request);
			status_t			_HandleRequest(UnlinkRequest* request);
			status_t			_HandleRequest(SymlinkRequest* request);
			status_t			_HandleRequest(ReadLinkRequest* request);
			status_t			_HandleRequest(RenameRequest* request);

			// directories
			status_t			_HandleRequest(MkDirRequest* request);
			status_t			_HandleRequest(RmDirRequest* request);
			status_t			_HandleRequest(OpenDirRequest* request);
			status_t			_HandleRequest(CloseDirRequest* request);
			status_t			_HandleRequest(FreeDirCookieRequest* request);
			status_t			_HandleRequest(ReadDirRequest* request);
			status_t			_HandleRequest(RewindDirRequest* request);
			status_t			_HandleRequest(WalkRequest* request);

			// attributes
			status_t			_HandleRequest(OpenAttrDirRequest* request);
			status_t			_HandleRequest(CloseAttrDirRequest* request);
			status_t			_HandleRequest(
									FreeAttrDirCookieRequest* request);
			status_t			_HandleRequest(ReadAttrDirRequest* request);
			status_t			_HandleRequest(RewindAttrDirRequest* request);
			status_t			_HandleRequest(ReadAttrRequest* request);
			status_t			_HandleRequest(WriteAttrRequest* request);
			status_t			_HandleRequest(RemoveAttrRequest* request);
			status_t			_HandleRequest(RenameAttrRequest* request);
			status_t			_HandleRequest(StatAttrRequest* request);

			// indices
			status_t			_HandleRequest(OpenIndexDirRequest* request);
			status_t			_HandleRequest(CloseIndexDirRequest* request);
			status_t			_HandleRequest(
									FreeIndexDirCookieRequest* request);
			status_t			_HandleRequest(ReadIndexDirRequest* request);
			status_t			_HandleRequest(RewindIndexDirRequest* request);
			status_t			_HandleRequest(CreateIndexRequest* request);
			status_t			_HandleRequest(RemoveIndexRequest* request);
			status_t			_HandleRequest(RenameIndexRequest* request);
			status_t			_HandleRequest(StatIndexRequest* request);

			// queries
			status_t			_HandleRequest(OpenQueryRequest* request);
			status_t			_HandleRequest(CloseQueryRequest* request);
			status_t			_HandleRequest(FreeQueryCookieRequest* request);
			status_t			_HandleRequest(ReadQueryRequest* request);

			status_t			_SendReply(RequestAllocator& allocator,
									bool expectsReceipt);

private:
			UserFileSystem*		fFileSystem;
			bool				fExpectReply;
			uint32				fExpectedReply;
};

}	// namespace UserlandFS

using UserlandFS::UserlandRequestHandler;

#endif	// USERLAND_FS_USERLAND_REQUEST_HANDLER_H
