// RequestFactory.cpp

#include <new>

#include "RequestFactory.h"
#include "Requests.h"

// constructor
RequestFactory::RequestFactory()
{
}

// destructor
RequestFactory::~RequestFactory()
{
}

// CreateRequest
status_t
RequestFactory::CreateRequest(uint32 type, Request** request)
{
	if (!request)
		return B_BAD_VALUE;
	switch (type) {
		case INIT_CONNECTION_REQUEST:
			*request = new(std::nothrow) InitConnectionRequest;
			break;
		case INIT_CONNECTION_REPLY:
			*request = new(std::nothrow) InitConnectionReply;
			break;
		case MOUNT_REQUEST:
			*request = new(std::nothrow) MountRequest;
			break;
		case MOUNT_REPLY:
			*request = new(std::nothrow) MountReply;
			break;
		case UNMOUNT_REQUEST:
			*request = new(std::nothrow) UnmountRequest;
			break;
		case READ_VNODE_REQUEST:
			*request = new(std::nothrow) ReadVNodeRequest;
			break;
		case READ_VNODE_REPLY:
			*request = new(std::nothrow) ReadVNodeReply;
			break;
		case WRITE_STAT_REQUEST:
			*request = new(std::nothrow) WriteStatRequest;
			break;
		case WRITE_STAT_REPLY:
			*request = new(std::nothrow) WriteStatReply;
			break;
		case CREATE_FILE_REQUEST:
			*request = new(std::nothrow) CreateFileRequest;
			break;
		case CREATE_FILE_REPLY:
			*request = new(std::nothrow) CreateFileReply;
			break;
		case OPEN_REQUEST:
			*request = new(std::nothrow) OpenRequest;
			break;
		case OPEN_REPLY:
			*request = new(std::nothrow) OpenReply;
			break;
		case CLOSE_REQUEST:
			*request = new(std::nothrow) CloseRequest;
			break;
		case CLOSE_REPLY:
			*request = new(std::nothrow) CloseReply;
			break;
		case READ_REQUEST:
			*request = new(std::nothrow) ReadRequest;
			break;
		case READ_REPLY:
			*request = new(std::nothrow) ReadReply;
			break;
		case WRITE_REQUEST:
			*request = new(std::nothrow) WriteRequest;
			break;
		case WRITE_REPLY:
			*request = new(std::nothrow) WriteReply;
			break;
		case CREATE_LINK_REQUEST:
			*request = new(std::nothrow) CreateLinkRequest;
			break;
		case CREATE_LINK_REPLY:
			*request = new(std::nothrow) CreateLinkReply;
			break;
		case UNLINK_REQUEST:
			*request = new(std::nothrow) UnlinkRequest;
			break;
		case UNLINK_REPLY:
			*request = new(std::nothrow) UnlinkReply;
			break;
		case CREATE_SYMLINK_REQUEST:
			*request = new(std::nothrow) CreateSymlinkRequest;
			break;
		case CREATE_SYMLINK_REPLY:
			*request = new(std::nothrow) CreateSymlinkReply;
			break;
		case READ_LINK_REQUEST:
			*request = new(std::nothrow) ReadLinkRequest;
			break;
		case READ_LINK_REPLY:
			*request = new(std::nothrow) ReadLinkReply;
			break;
		case RENAME_REQUEST:
			*request = new(std::nothrow) RenameRequest;
			break;
		case RENAME_REPLY:
			*request = new(std::nothrow) RenameReply;
			break;
		case MAKE_DIR_REQUEST:
			*request = new(std::nothrow) MakeDirRequest;
			break;
		case MAKE_DIR_REPLY:
			*request = new(std::nothrow) MakeDirReply;
			break;
		case REMOVE_DIR_REQUEST:
			*request = new(std::nothrow) RemoveDirRequest;
			break;
		case REMOVE_DIR_REPLY:
			*request = new(std::nothrow) RemoveDirReply;
			break;
		case OPEN_DIR_REQUEST:
			*request = new(std::nothrow) OpenDirRequest;
			break;
		case OPEN_DIR_REPLY:
			*request = new(std::nothrow) OpenDirReply;
			break;
		case READ_DIR_REQUEST:
			*request = new(std::nothrow) ReadDirRequest;
			break;
		case READ_DIR_REPLY:
			*request = new(std::nothrow) ReadDirReply;
			break;
		case WALK_REQUEST:
			*request = new(std::nothrow) WalkRequest;
			break;
		case WALK_REPLY:
			*request = new(std::nothrow) WalkReply;
			break;
		case MULTI_WALK_REQUEST:
			*request = new(std::nothrow) MultiWalkRequest;
			break;
		case MULTI_WALK_REPLY:
			*request = new(std::nothrow) MultiWalkReply;
			break;
		case OPEN_ATTR_DIR_REQUEST:
			*request = new(std::nothrow) OpenAttrDirRequest;
			break;
		case OPEN_ATTR_DIR_REPLY:
			*request = new(std::nothrow) OpenAttrDirReply;
			break;
		case READ_ATTR_DIR_REQUEST:
			*request = new(std::nothrow) ReadAttrDirRequest;
			break;
		case READ_ATTR_DIR_REPLY:
			*request = new(std::nothrow) ReadAttrDirReply;
			break;
		case READ_ATTR_REQUEST:
			*request = new(std::nothrow) ReadAttrRequest;
			break;
		case READ_ATTR_REPLY:
			*request = new(std::nothrow) ReadAttrReply;
			break;
		case WRITE_ATTR_REQUEST:
			*request = new(std::nothrow) WriteAttrRequest;
			break;
		case WRITE_ATTR_REPLY:
			*request = new(std::nothrow) WriteAttrReply;
			break;
		case REMOVE_ATTR_REQUEST:
			*request = new(std::nothrow) RemoveAttrRequest;
			break;
		case REMOVE_ATTR_REPLY:
			*request = new(std::nothrow) RemoveAttrReply;
			break;
		case RENAME_ATTR_REQUEST:
			*request = new(std::nothrow) RenameAttrRequest;
			break;
		case RENAME_ATTR_REPLY:
			*request = new(std::nothrow) RenameAttrReply;
			break;
		case STAT_ATTR_REQUEST:
			*request = new(std::nothrow) StatAttrRequest;
			break;
		case STAT_ATTR_REPLY:
			*request = new(std::nothrow) StatAttrReply;
			break;
		case OPEN_QUERY_REQUEST:
			*request = new(std::nothrow) OpenQueryRequest;
			break;
		case OPEN_QUERY_REPLY:
			*request = new(std::nothrow) OpenQueryReply;
			break;
		case READ_QUERY_REQUEST:
			*request = new(std::nothrow) ReadQueryRequest;
			break;
		case READ_QUERY_REPLY:
			*request = new(std::nothrow) ReadQueryReply;
			break;
		case ENTRY_CREATED_REQUEST:
			*request = new(std::nothrow) EntryCreatedRequest;
			break;
		case ENTRY_REMOVED_REQUEST:
			*request = new(std::nothrow) EntryRemovedRequest;
			break;
		case ENTRY_MOVED_REQUEST:
			*request = new(std::nothrow) EntryMovedRequest;
			break;
		case ENTRY_STAT_CHANGED_REQUEST:
			*request = new(std::nothrow) StatChangedRequest;
			break;
		case ENTRY_ATTRIBUTE_CHANGED_REQUEST:
			*request = new(std::nothrow) AttributeChangedRequest;
			break;
		case SERVER_INFO_REQUEST:
			*request = new(std::nothrow) ServerInfoRequest;
			break;
		default:
			return B_BAD_VALUE;
	}
	return (*request ? B_OK : B_NO_MEMORY);
}

