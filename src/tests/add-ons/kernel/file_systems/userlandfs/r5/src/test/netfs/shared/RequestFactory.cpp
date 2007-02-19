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
			*request = new(nothrow) InitConnectionRequest;
			break;
		case INIT_CONNECTION_REPLY:
			*request = new(nothrow) InitConnectionReply;
			break;
		case MOUNT_REQUEST:
			*request = new(nothrow) MountRequest;
			break;
		case MOUNT_REPLY:
			*request = new(nothrow) MountReply;
			break;
		case UNMOUNT_REQUEST:
			*request = new(nothrow) UnmountRequest;
			break;
		case READ_VNODE_REQUEST:
			*request = new(nothrow) ReadVNodeRequest;
			break;
		case READ_VNODE_REPLY:
			*request = new(nothrow) ReadVNodeReply;
			break;
		case WRITE_STAT_REQUEST:
			*request = new(nothrow) WriteStatRequest;
			break;
		case WRITE_STAT_REPLY:
			*request = new(nothrow) WriteStatReply;
			break;
		case CREATE_FILE_REQUEST:
			*request = new(nothrow) CreateFileRequest;
			break;
		case CREATE_FILE_REPLY:
			*request = new(nothrow) CreateFileReply;
			break;
		case OPEN_REQUEST:
			*request = new(nothrow) OpenRequest;
			break;
		case OPEN_REPLY:
			*request = new(nothrow) OpenReply;
			break;
		case CLOSE_REQUEST:
			*request = new(nothrow) CloseRequest;
			break;
		case CLOSE_REPLY:
			*request = new(nothrow) CloseReply;
			break;
		case READ_REQUEST:
			*request = new(nothrow) ReadRequest;
			break;
		case READ_REPLY:
			*request = new(nothrow) ReadReply;
			break;
		case WRITE_REQUEST:
			*request = new(nothrow) WriteRequest;
			break;
		case WRITE_REPLY:
			*request = new(nothrow) WriteReply;
			break;
		case CREATE_LINK_REQUEST:
			*request = new(nothrow) CreateLinkRequest;
			break;
		case CREATE_LINK_REPLY:
			*request = new(nothrow) CreateLinkReply;
			break;
		case UNLINK_REQUEST:
			*request = new(nothrow) UnlinkRequest;
			break;
		case UNLINK_REPLY:
			*request = new(nothrow) UnlinkReply;
			break;
		case CREATE_SYMLINK_REQUEST:
			*request = new(nothrow) CreateSymlinkRequest;
			break;
		case CREATE_SYMLINK_REPLY:
			*request = new(nothrow) CreateSymlinkReply;
			break;
		case READ_LINK_REQUEST:
			*request = new(nothrow) ReadLinkRequest;
			break;
		case READ_LINK_REPLY:
			*request = new(nothrow) ReadLinkReply;
			break;
		case RENAME_REQUEST:
			*request = new(nothrow) RenameRequest;
			break;
		case RENAME_REPLY:
			*request = new(nothrow) RenameReply;
			break;
		case MAKE_DIR_REQUEST:
			*request = new(nothrow) MakeDirRequest;
			break;
		case MAKE_DIR_REPLY:
			*request = new(nothrow) MakeDirReply;
			break;
		case REMOVE_DIR_REQUEST:
			*request = new(nothrow) RemoveDirRequest;
			break;
		case REMOVE_DIR_REPLY:
			*request = new(nothrow) RemoveDirReply;
			break;
		case OPEN_DIR_REQUEST:
			*request = new(nothrow) OpenDirRequest;
			break;
		case OPEN_DIR_REPLY:
			*request = new(nothrow) OpenDirReply;
			break;
		case READ_DIR_REQUEST:
			*request = new(nothrow) ReadDirRequest;
			break;
		case READ_DIR_REPLY:
			*request = new(nothrow) ReadDirReply;
			break;
		case WALK_REQUEST:
			*request = new(nothrow) WalkRequest;
			break;
		case WALK_REPLY:
			*request = new(nothrow) WalkReply;
			break;
		case MULTI_WALK_REQUEST:
			*request = new(nothrow) MultiWalkRequest;
			break;
		case MULTI_WALK_REPLY:
			*request = new(nothrow) MultiWalkReply;
			break;
		case OPEN_ATTR_DIR_REQUEST:
			*request = new(nothrow) OpenAttrDirRequest;
			break;
		case OPEN_ATTR_DIR_REPLY:
			*request = new(nothrow) OpenAttrDirReply;
			break;
		case READ_ATTR_DIR_REQUEST:
			*request = new(nothrow) ReadAttrDirRequest;
			break;
		case READ_ATTR_DIR_REPLY:
			*request = new(nothrow) ReadAttrDirReply;
			break;
		case READ_ATTR_REQUEST:
			*request = new(nothrow) ReadAttrRequest;
			break;
		case READ_ATTR_REPLY:
			*request = new(nothrow) ReadAttrReply;
			break;
		case WRITE_ATTR_REQUEST:
			*request = new(nothrow) WriteAttrRequest;
			break;
		case WRITE_ATTR_REPLY:
			*request = new(nothrow) WriteAttrReply;
			break;
		case REMOVE_ATTR_REQUEST:
			*request = new(nothrow) RemoveAttrRequest;
			break;
		case REMOVE_ATTR_REPLY:
			*request = new(nothrow) RemoveAttrReply;
			break;
		case RENAME_ATTR_REQUEST:
			*request = new(nothrow) RenameAttrRequest;
			break;
		case RENAME_ATTR_REPLY:
			*request = new(nothrow) RenameAttrReply;
			break;
		case STAT_ATTR_REQUEST:
			*request = new(nothrow) StatAttrRequest;
			break;
		case STAT_ATTR_REPLY:
			*request = new(nothrow) StatAttrReply;
			break;
		case OPEN_QUERY_REQUEST:
			*request = new(nothrow) OpenQueryRequest;
			break;
		case OPEN_QUERY_REPLY:
			*request = new(nothrow) OpenQueryReply;
			break;
		case READ_QUERY_REQUEST:
			*request = new(nothrow) ReadQueryRequest;
			break;
		case READ_QUERY_REPLY:
			*request = new(nothrow) ReadQueryReply;
			break;
		case ENTRY_CREATED_REQUEST:
			*request = new(nothrow) EntryCreatedRequest;
			break;
		case ENTRY_REMOVED_REQUEST:
			*request = new(nothrow) EntryRemovedRequest;
			break;
		case ENTRY_MOVED_REQUEST:
			*request = new(nothrow) EntryMovedRequest;
			break;
		case ENTRY_STAT_CHANGED_REQUEST:
			*request = new(nothrow) StatChangedRequest;
			break;
		case ENTRY_ATTRIBUTE_CHANGED_REQUEST:
			*request = new(nothrow) AttributeChangedRequest;
			break;
		case SERVER_INFO_REQUEST:
			*request = new(nothrow) ServerInfoRequest;
			break;
		default:
			return B_BAD_VALUE;
	}
	return (*request ? B_OK : B_NO_MEMORY);
}

