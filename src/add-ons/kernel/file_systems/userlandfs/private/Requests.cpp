// Requests.cpp

#include <limits.h>

#include "Debug.h"
#include "Requests.h"

#define _ADD_ADDRESS(_address, _flags)				\
	if (*count >= MAX_REQUEST_ADDRESS_COUNT)		\
		return B_BAD_VALUE;							\
	infos[*count].address = &_address;				\
	infos[*count].flags = _flags;					\
	infos[(*count)++].max_size = LONG_MAX;	// TODO:...

#define ADD_ADDRESS(address)	_ADD_ADDRESS(address, 0)
#define ADD_STRING(address)		_ADD_ADDRESS(address, ADDRESS_IS_STRING)
#define ADD_NON_NULL_ADDRESS(address)	_ADD_ADDRESS(address, ADDRESS_NOT_NULL)
#define ADD_NON_NULL_STRING(address)	\
	_ADD_ADDRESS(address, (ADDRESS_IS_STRING | ADDRESS_NOT_NULL))

// FSConnectRequest
status_t
FSConnectRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(fsName);
	return B_OK;
}

// FSConnectReply
status_t
FSConnectReply::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_ADDRESS(portInfos);
	return B_OK;
}

// MountVolumeRequest
status_t
MountVolumeRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(cwd);
	ADD_STRING(device);
	ADD_STRING(parameters);
	return B_OK;
}

// InitializeVolumeRequest
status_t
InitializeVolumeRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_STRING(device);
	ADD_STRING(parameters);
	return B_OK;
}

// CreateRequest
status_t
CreateRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(name);
	return B_OK;
}

// ReadReply
status_t
ReadReply::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_ADDRESS(buffer);
	return B_OK;
}

// WriteRequest
status_t
WriteRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_ADDRESS(buffer);
	return B_OK;
}

// IOCtlRequest
status_t
IOCtlRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_ADDRESS(buffer);
	return B_OK;
}

// IOCtlReply
status_t
IOCtlReply::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_ADDRESS(buffer);
	return B_OK;
}

// LinkRequest
status_t
LinkRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(name);
	return B_OK;
}

// UnlinkRequest
status_t
UnlinkRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(name);
	return B_OK;
}

// SymlinkRequest
status_t
SymlinkRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(name);
	ADD_NON_NULL_STRING(target);
	return B_OK;
}

// ReadLinkReply
status_t
ReadLinkReply::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_STRING(buffer);
	return B_OK;
}

// RenameRequest
status_t
RenameRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(oldName);
	ADD_NON_NULL_STRING(newName);
	return B_OK;
}

// MkDirRequest
status_t
MkDirRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(name);
	return B_OK;
}

// RmDirRequest
status_t
RmDirRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(name);
	return B_OK;
}

// ReadDirReply
status_t
ReadDirReply::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_ADDRESS(buffer);
	return B_OK;
}

// WalkRequest
status_t
WalkRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(entryName);
	return B_OK;
}

// WalkReply
status_t
WalkReply::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_STRING(resolvedPath);
	return B_OK;
}

// ReadAttrDirReply
status_t
ReadAttrDirReply::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_ADDRESS(buffer);
	return B_OK;
}

// ReadAttrRequest
status_t
ReadAttrRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(name);
	return B_OK;
}

// ReadAttrReply
status_t
ReadAttrReply::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_ADDRESS(buffer);
	return B_OK;
}

// WriteAttrRequest
status_t
WriteAttrRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(name);
	ADD_ADDRESS(buffer);
	return B_OK;
}

// RemoveAttrRequest
status_t
RemoveAttrRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(name);
	return B_OK;
}

// RenameAttrRequest
status_t
RenameAttrRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(oldName);
	ADD_NON_NULL_STRING(newName);
	return B_OK;
}

// StatAttrRequest
status_t
StatAttrRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(name);
	return B_OK;
}

// ReadIndexDirReply
status_t
ReadIndexDirReply::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_ADDRESS(buffer);
	return B_OK;
}

// CreateIndexRequest
status_t
CreateIndexRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(name);
	return B_OK;
}

// RemoveIndexRequest
status_t
RemoveIndexRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(name);
	return B_OK;
}

// RenameIndexRequest
status_t
RenameIndexRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(oldName);
	ADD_NON_NULL_STRING(newName);
	return B_OK;
}

// StatIndexRequest
status_t
StatIndexRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(name);
	return B_OK;
}

// OpenQueryRequest
status_t
OpenQueryRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(queryString);
	return B_OK;
}

// ReadQueryReply
status_t
ReadQueryReply::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_ADDRESS(buffer);
	return B_OK;
}

// NotifyListenerRequest
status_t
NotifyListenerRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_STRING(name);
	return B_OK;
}

// SendNotificationRequest
status_t
SendNotificationRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_STRING(name);
	return B_OK;
}


// #pragma mark -

// RequestAddressInfoGetter
struct RequestAddressInfoGetter {
	RequestAddressInfoGetter(AddressInfo* infos, int32* count)
		: fInfos(infos),
		  fCount(count)
	{
	}

	template<typename R> status_t operator()(R* request)
	{
		return request->GetAddressInfos(fInfos, fCount);
	}

private:
	AddressInfo*	fInfos;
	int32*			fCount;
};

// get_request_address_infos
status_t
UserlandFSUtil::get_request_address_infos(Request* request, AddressInfo* infos,
	int32* count)
{
	if (!infos || !count)
		return B_BAD_VALUE;
	*count = 0;
	RequestAddressInfoGetter task(infos, count);
	return do_for_request(request, task);
}

// RequestChecker
struct RequestChecker {
	template<typename R> status_t operator()(R* request)
	{
		return request->Check();
	}
};

// check_request
status_t
UserlandFSUtil::check_request(Request* request)
{
	RequestChecker task;
	return do_for_request(request, task);
}


// is_error_reply
static inline
bool
is_error_reply(Request* request)
{
	return false;
}

// is_error_reply
static inline
bool
is_error_reply(ReplyRequest* request)
{
	return (request->error != B_OK);
}

// RequestRelocator
struct RequestRelocator {
	RequestRelocator(int32 requestBufferSize, area_id* areas, int32* count)
		: fRequestBufferSize(requestBufferSize),
		  fAreas(areas),
		  fAreaCount(count)
	{
		*fAreaCount = 0;
	}

	~RequestRelocator()
	{
		if (!fSuccess) {
			for (int32 i = 0; i < *fAreaCount; i++)
				delete_area(fAreas[i]);
		}
	}

	template<typename R> status_t operator()(R* request)
	{
		// check the request buffer size
		if (fRequestBufferSize < (int32)sizeof(R))
			RETURN_ERROR(B_BAD_DATA);
		// no need to relocate the addresses of a reply that indicates an error
		if (is_error_reply(request)) {
			fSuccess = true;
			return B_OK;
		}
		// get the address infos
		AddressInfo infos[MAX_REQUEST_ADDRESS_COUNT];
		int32 count = 0;
		status_t error = request->GetAddressInfos(infos, &count);
		if (error != B_OK)
			RETURN_ERROR(error);
		// check and relocate the addresses
		for (int32 i = 0; i < count; i++) {
			// check
			Address* address = infos[i].address;
			int32 size = address->GetSize();
			int32 offset = address->GetOffset();
//PRINT(("  relocating address: area: %ld, offset: %ld, size: %ld...\n",
//address->GetArea(), offset, size));
			if (offset < 0 || size < 0 || size > infos[i].max_size)
				RETURN_ERROR(B_BAD_DATA);
			if ((infos[i].flags & ADDRESS_NOT_NULL) && size == 0)
				RETURN_ERROR(B_BAD_DATA);
			// relocate
			area_id area = address->GetArea();
			if (area < 0) {
				// data in the buffer itself
				if (offset == 0 && size == 0) {
//PRINT(("    -> relocated address: NULL\n"));
					address->SetRelocatedAddress(NULL);
				} else {
					if (offset < (int32)sizeof(R)
						|| offset + size > fRequestBufferSize) {
						RETURN_ERROR(B_BAD_DATA);
					}
//PRINT(("    -> relocated address: %p\n", (uint8*)request + offset));
					address->SetRelocatedAddress((uint8*)request + offset);
				}
			} else {
				// clone the area
				void* data;
				area = clone_area("cloned request data", &data,
#ifdef _KERNEL_MODE
					B_ANY_KERNEL_ADDRESS,
#else
					B_ANY_ADDRESS,
#endif
					B_READ_AREA, area);
				if (area < 0)
					RETURN_ERROR(area);
				fAreas[(*fAreaCount)++] = area;
				// check offset and size
				area_info areaInfo;
				error = get_area_info(area, &areaInfo);
				if (error != B_OK)
					RETURN_ERROR(error);
				if (offset + size > (int32)areaInfo.size)
					RETURN_ERROR(B_BAD_DATA);
//PRINT(("    -> relocated address: %p\n", (uint8*)data + offset));
				address->SetRelocatedAddress((uint8*)data + offset);
			}
		}
		// finally let the request check its integrity
		error = request->Check();
		if (error != B_OK)
			RETURN_ERROR(error);
		fSuccess = true;
//PRINT(("RequestRelocator done: success\n"));
		return B_OK;
	}

private:
	int32		fRequestBufferSize;
	area_id*	fAreas;
	int32*		fAreaCount;
	bool		fSuccess;
};

// relocate_request
status_t
UserlandFSUtil::relocate_request(Request* request, int32 requestBufferSize,
	area_id* areas, int32* count)
{
	if (!request || !areas || !count)
		return B_BAD_VALUE;
	RequestRelocator task(requestBufferSize, areas, count);
	return do_for_request(request, task);
}

// is_kernel_request
bool
UserlandFSUtil::is_kernel_request(uint32 type)
{
	switch (type) {
		// kernel -> userland requests
		// administrative
		case UFS_DISCONNECT_REQUEST:
		case FS_CONNECT_REQUEST:
			return true;
		case FS_CONNECT_REPLY:
			return false;
		// FS
		case MOUNT_VOLUME_REQUEST:
		case UNMOUNT_VOLUME_REQUEST:
		case INITIALIZE_VOLUME_REQUEST:
		case SYNC_VOLUME_REQUEST:
		case READ_FS_STAT_REQUEST:
		case WRITE_FS_STAT_REQUEST:
			return true;
		case MOUNT_VOLUME_REPLY:
		case UNMOUNT_VOLUME_REPLY:
		case INITIALIZE_VOLUME_REPLY:
		case SYNC_VOLUME_REPLY:
		case READ_FS_STAT_REPLY:
		case WRITE_FS_STAT_REPLY:
			return false;
		// vnodes
		case READ_VNODE_REQUEST:
		case WRITE_VNODE_REQUEST:
		case FS_REMOVE_VNODE_REQUEST:
			return true;
		case READ_VNODE_REPLY:
		case WRITE_VNODE_REPLY:
		case FS_REMOVE_VNODE_REPLY:
			return false;
		// nodes
		case FSYNC_REQUEST:
		case READ_STAT_REQUEST:
		case WRITE_STAT_REQUEST:
		case ACCESS_REQUEST:
			return true;
		case FSYNC_REPLY:
		case READ_STAT_REPLY:
		case WRITE_STAT_REPLY:
		case ACCESS_REPLY:
			return false;
		// files
		case CREATE_REQUEST:
		case OPEN_REQUEST:
		case CLOSE_REQUEST:
		case FREE_COOKIE_REQUEST:
		case READ_REQUEST:
		case WRITE_REQUEST:
		case IOCTL_REQUEST:
		case SET_FLAGS_REQUEST:
		case SELECT_REQUEST:
		case DESELECT_REQUEST:
			return true;
		case CREATE_REPLY:
		case OPEN_REPLY:
		case CLOSE_REPLY:
		case FREE_COOKIE_REPLY:
		case READ_REPLY:
		case WRITE_REPLY:
		case IOCTL_REPLY:
		case SET_FLAGS_REPLY:
		case SELECT_REPLY:
		case DESELECT_REPLY:
			return false;
		// hard links / symlinks
		case LINK_REQUEST:
		case UNLINK_REQUEST:
		case SYMLINK_REQUEST:
		case READ_LINK_REQUEST:
		case RENAME_REQUEST:
			return true;
		case LINK_REPLY:
		case UNLINK_REPLY:
		case SYMLINK_REPLY:
		case READ_LINK_REPLY:
		case RENAME_REPLY:
			return false;
		// directories
		case MKDIR_REQUEST:
		case RMDIR_REQUEST:
		case OPEN_DIR_REQUEST:
		case CLOSE_DIR_REQUEST:
		case FREE_DIR_COOKIE_REQUEST:
		case READ_DIR_REQUEST:
		case REWIND_DIR_REQUEST:
		case WALK_REQUEST:
			return true;
		case MKDIR_REPLY:
		case RMDIR_REPLY:
		case OPEN_DIR_REPLY:
		case CLOSE_DIR_REPLY:
		case FREE_DIR_COOKIE_REPLY:
		case READ_DIR_REPLY:
		case REWIND_DIR_REPLY:
		case WALK_REPLY:
			return false;
		// attributes
		case OPEN_ATTR_DIR_REQUEST:
		case CLOSE_ATTR_DIR_REQUEST:
		case FREE_ATTR_DIR_COOKIE_REQUEST:
		case READ_ATTR_DIR_REQUEST:
		case REWIND_ATTR_DIR_REQUEST:
		case READ_ATTR_REQUEST:
		case WRITE_ATTR_REQUEST:
		case REMOVE_ATTR_REQUEST:
		case RENAME_ATTR_REQUEST:
		case STAT_ATTR_REQUEST:
			return true;
		case OPEN_ATTR_DIR_REPLY:
		case CLOSE_ATTR_DIR_REPLY:
		case FREE_ATTR_DIR_COOKIE_REPLY:
		case READ_ATTR_DIR_REPLY:
		case REWIND_ATTR_DIR_REPLY:
		case READ_ATTR_REPLY:
		case WRITE_ATTR_REPLY:
		case REMOVE_ATTR_REPLY:
		case RENAME_ATTR_REPLY:
		case STAT_ATTR_REPLY:
			return false;
		// indices
		case OPEN_INDEX_DIR_REQUEST:
		case CLOSE_INDEX_DIR_REQUEST:
		case FREE_INDEX_DIR_COOKIE_REQUEST:
		case READ_INDEX_DIR_REQUEST:
		case REWIND_INDEX_DIR_REQUEST:
		case CREATE_INDEX_REQUEST:
		case REMOVE_INDEX_REQUEST:
		case RENAME_INDEX_REQUEST:
		case STAT_INDEX_REQUEST:
			return true;
		case OPEN_INDEX_DIR_REPLY:
		case CLOSE_INDEX_DIR_REPLY:
		case FREE_INDEX_DIR_COOKIE_REPLY:
		case READ_INDEX_DIR_REPLY:
		case REWIND_INDEX_DIR_REPLY:
		case CREATE_INDEX_REPLY:
		case REMOVE_INDEX_REPLY:
		case RENAME_INDEX_REPLY:
		case STAT_INDEX_REPLY:
			return false;
		// queries
		case OPEN_QUERY_REQUEST:
		case CLOSE_QUERY_REQUEST:
		case FREE_QUERY_COOKIE_REQUEST:
		case READ_QUERY_REQUEST:
			return true;
		case OPEN_QUERY_REPLY:
		case CLOSE_QUERY_REPLY:
		case FREE_QUERY_COOKIE_REPLY:
		case READ_QUERY_REPLY:
			return false;

		// userland -> kernel requests
		// notifications
		case NOTIFY_LISTENER_REQUEST:
		case NOTIFY_SELECT_EVENT_REQUEST:
		case SEND_NOTIFICATION_REQUEST:
			return false;
		case NOTIFY_LISTENER_REPLY:
		case NOTIFY_SELECT_EVENT_REPLY:
		case SEND_NOTIFICATION_REPLY:
			return true;
		// vnodes
		case GET_VNODE_REQUEST:
		case PUT_VNODE_REQUEST:
		case NEW_VNODE_REQUEST:
		case REMOVE_VNODE_REQUEST:
		case UNREMOVE_VNODE_REQUEST:
		case IS_VNODE_REMOVED_REQUEST:
			return false;
		case GET_VNODE_REPLY:
		case PUT_VNODE_REPLY:
		case NEW_VNODE_REPLY:
		case REMOVE_VNODE_REPLY:
		case UNREMOVE_VNODE_REPLY:
		case IS_VNODE_REMOVED_REPLY:
			return true;

		// general reply
		case RECEIPT_ACK_REPLY:
			return true;
		default:
			return false;
	}
}

// is_userland_request
bool
UserlandFSUtil::is_userland_request(uint32 type)
{
	switch (type) {
		// kernel -> userland requests
		// administrative
		case UFS_DISCONNECT_REQUEST:
		case FS_CONNECT_REQUEST:
			return false;
		case FS_CONNECT_REPLY:
			return true;
		// FS
		case MOUNT_VOLUME_REQUEST:
		case UNMOUNT_VOLUME_REQUEST:
		case INITIALIZE_VOLUME_REQUEST:
		case SYNC_VOLUME_REQUEST:
		case READ_FS_STAT_REQUEST:
		case WRITE_FS_STAT_REQUEST:
			return false;
		case MOUNT_VOLUME_REPLY:
		case UNMOUNT_VOLUME_REPLY:
		case INITIALIZE_VOLUME_REPLY:
		case SYNC_VOLUME_REPLY:
		case READ_FS_STAT_REPLY:
		case WRITE_FS_STAT_REPLY:
			return true;
		// vnodes
		case READ_VNODE_REQUEST:
		case WRITE_VNODE_REQUEST:
		case FS_REMOVE_VNODE_REQUEST:
			return false;
		case READ_VNODE_REPLY:
		case WRITE_VNODE_REPLY:
		case FS_REMOVE_VNODE_REPLY:
			return true;
		// nodes
		case FSYNC_REQUEST:
		case READ_STAT_REQUEST:
		case WRITE_STAT_REQUEST:
		case ACCESS_REQUEST:
			return false;
		case FSYNC_REPLY:
		case READ_STAT_REPLY:
		case WRITE_STAT_REPLY:
		case ACCESS_REPLY:
			return true;
		// files
		case CREATE_REQUEST:
		case OPEN_REQUEST:
		case CLOSE_REQUEST:
		case FREE_COOKIE_REQUEST:
		case READ_REQUEST:
		case WRITE_REQUEST:
		case IOCTL_REQUEST:
		case SET_FLAGS_REQUEST:
		case SELECT_REQUEST:
		case DESELECT_REQUEST:
			return false;
		case CREATE_REPLY:
		case OPEN_REPLY:
		case CLOSE_REPLY:
		case FREE_COOKIE_REPLY:
		case READ_REPLY:
		case WRITE_REPLY:
		case IOCTL_REPLY:
		case SET_FLAGS_REPLY:
		case SELECT_REPLY:
		case DESELECT_REPLY:
			return true;
		// hard links / symlinks
		case LINK_REQUEST:
		case UNLINK_REQUEST:
		case SYMLINK_REQUEST:
		case READ_LINK_REQUEST:
		case RENAME_REQUEST:
			return false;
		case LINK_REPLY:
		case UNLINK_REPLY:
		case SYMLINK_REPLY:
		case READ_LINK_REPLY:
		case RENAME_REPLY:
			return true;
		// directories
		case MKDIR_REQUEST:
		case RMDIR_REQUEST:
		case OPEN_DIR_REQUEST:
		case CLOSE_DIR_REQUEST:
		case FREE_DIR_COOKIE_REQUEST:
		case READ_DIR_REQUEST:
		case REWIND_DIR_REQUEST:
		case WALK_REQUEST:
			return false;
		case MKDIR_REPLY:
		case RMDIR_REPLY:
		case OPEN_DIR_REPLY:
		case CLOSE_DIR_REPLY:
		case FREE_DIR_COOKIE_REPLY:
		case READ_DIR_REPLY:
		case REWIND_DIR_REPLY:
		case WALK_REPLY:
			return true;
		// attributes
		case OPEN_ATTR_DIR_REQUEST:
		case CLOSE_ATTR_DIR_REQUEST:
		case FREE_ATTR_DIR_COOKIE_REQUEST:
		case READ_ATTR_DIR_REQUEST:
		case REWIND_ATTR_DIR_REQUEST:
		case READ_ATTR_REQUEST:
		case WRITE_ATTR_REQUEST:
		case REMOVE_ATTR_REQUEST:
		case RENAME_ATTR_REQUEST:
		case STAT_ATTR_REQUEST:
			return false;
		case OPEN_ATTR_DIR_REPLY:
		case CLOSE_ATTR_DIR_REPLY:
		case FREE_ATTR_DIR_COOKIE_REPLY:
		case READ_ATTR_DIR_REPLY:
		case REWIND_ATTR_DIR_REPLY:
		case READ_ATTR_REPLY:
		case WRITE_ATTR_REPLY:
		case REMOVE_ATTR_REPLY:
		case RENAME_ATTR_REPLY:
		case STAT_ATTR_REPLY:
			return true;
		// indices
		case OPEN_INDEX_DIR_REQUEST:
		case CLOSE_INDEX_DIR_REQUEST:
		case FREE_INDEX_DIR_COOKIE_REQUEST:
		case READ_INDEX_DIR_REQUEST:
		case REWIND_INDEX_DIR_REQUEST:
		case CREATE_INDEX_REQUEST:
		case REMOVE_INDEX_REQUEST:
		case RENAME_INDEX_REQUEST:
		case STAT_INDEX_REQUEST:
			return false;
		case OPEN_INDEX_DIR_REPLY:
		case CLOSE_INDEX_DIR_REPLY:
		case FREE_INDEX_DIR_COOKIE_REPLY:
		case READ_INDEX_DIR_REPLY:
		case REWIND_INDEX_DIR_REPLY:
		case CREATE_INDEX_REPLY:
		case REMOVE_INDEX_REPLY:
		case RENAME_INDEX_REPLY:
		case STAT_INDEX_REPLY:
			return true;
		// queries
		case OPEN_QUERY_REQUEST:
		case CLOSE_QUERY_REQUEST:
		case FREE_QUERY_COOKIE_REQUEST:
		case READ_QUERY_REQUEST:
			return false;
		case OPEN_QUERY_REPLY:
		case CLOSE_QUERY_REPLY:
		case FREE_QUERY_COOKIE_REPLY:
		case READ_QUERY_REPLY:
			return true;

		// userland -> kernel requests
		// notifications
		case NOTIFY_LISTENER_REQUEST:
		case NOTIFY_SELECT_EVENT_REQUEST:
		case SEND_NOTIFICATION_REQUEST:
			return true;
		case NOTIFY_LISTENER_REPLY:
		case NOTIFY_SELECT_EVENT_REPLY:
		case SEND_NOTIFICATION_REPLY:
			return false;
		// vnodes
		case GET_VNODE_REQUEST:
		case PUT_VNODE_REQUEST:
		case NEW_VNODE_REQUEST:
		case REMOVE_VNODE_REQUEST:
		case UNREMOVE_VNODE_REQUEST:
		case IS_VNODE_REMOVED_REQUEST:
			return true;
		case GET_VNODE_REPLY:
		case PUT_VNODE_REPLY:
		case NEW_VNODE_REPLY:
		case REMOVE_VNODE_REPLY:
		case UNREMOVE_VNODE_REPLY:
		case IS_VNODE_REMOVED_REPLY:
			return false;

		// general reply
		case RECEIPT_ACK_REPLY:
			return true;
		default:
			return false;
	}
}
