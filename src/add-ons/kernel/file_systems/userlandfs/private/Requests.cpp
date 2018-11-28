// Requests.cpp

#include <limits.h>

#include "Debug.h"
#include "Requests.h"

#define _ADD_ADDRESS(_address, _flags)				\
	if (*count >= MAX_REQUEST_ADDRESS_COUNT)		\
		return B_BAD_VALUE;							\
	infos[*count].address = &_address;				\
	infos[*count].flags = _flags;					\
	infos[(*count)++].max_size = INT32_MAX;	// TODO:...

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

//// InitializeVolumeRequest
//status_t
//InitializeVolumeRequest::GetAddressInfos(AddressInfo* infos, int32* count)
//{
//	ADD_STRING(device);
//	ADD_STRING(parameters);
//	return B_OK;
//}

// GetVNodeNameReply
status_t
GetVNodeNameReply::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_STRING(buffer);
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

// CreateSymlinkRequest
status_t
CreateSymlinkRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(name);
	ADD_NON_NULL_STRING(target);
	return B_OK;
}

// ReadSymlinkReply
status_t
ReadSymlinkReply::GetAddressInfos(AddressInfo* infos, int32* count)
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

// CreateDirRequest
status_t
CreateDirRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(name);
	return B_OK;
}

// RemoveDirRequest
status_t
RemoveDirRequest::GetAddressInfos(AddressInfo* infos, int32* count)
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

// LookupRequest
status_t
LookupRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(entryName);
	return B_OK;
}

// ReadAttrDirReply
status_t
ReadAttrDirReply::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_ADDRESS(buffer);
	return B_OK;
}

// CreateAttrRequest
status_t
CreateAttrRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_NON_NULL_STRING(name);
	return B_OK;
}

// OpenAttrRequest
status_t
OpenAttrRequest::GetAddressInfos(AddressInfo* infos, int32* count)
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

// ReadIndexStatRequest
status_t
ReadIndexStatRequest::GetAddressInfos(AddressInfo* infos, int32* count)
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

// NodeMonitoringEventRequest
status_t
NodeMonitoringEventRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_ADDRESS(event);
	return B_OK;
}

// NotifyListenerRequest
status_t
NotifyListenerRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_STRING(oldName);
	ADD_STRING(name);
	return B_OK;
}

// NotifyQueryRequest
status_t
NotifyQueryRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_STRING(name);
	return B_OK;
}

// FileCacheReadReply
status_t
FileCacheReadReply::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_ADDRESS(buffer);
	return B_OK;
}

// FileCacheWriteRequest
status_t
FileCacheWriteRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_ADDRESS(buffer);
	return B_OK;
}

// ReadFromIORequestReply
status_t
ReadFromIORequestReply::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_ADDRESS(buffer);
	return B_OK;
}

// WriteToIORequestRequest
status_t
WriteToIORequestRequest::GetAddressInfos(AddressInfo* infos, int32* count)
{
	ADD_ADDRESS(buffer);
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
					B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA,
#else
					B_ANY_ADDRESS, B_READ_AREA,
#endif
					area);
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
//		case INITIALIZE_VOLUME_REQUEST:
		case SYNC_VOLUME_REQUEST:
		case READ_FS_INFO_REQUEST:
		case WRITE_FS_INFO_REQUEST:
			return true;
		case MOUNT_VOLUME_REPLY:
		case UNMOUNT_VOLUME_REPLY:
//		case INITIALIZE_VOLUME_REPLY:
		case SYNC_VOLUME_REPLY:
		case READ_FS_INFO_REPLY:
		case WRITE_FS_INFO_REPLY:
			return false;
		// vnodes
		case LOOKUP_REQUEST:
		case GET_VNODE_NAME_REQUEST:
		case READ_VNODE_REQUEST:
		case WRITE_VNODE_REQUEST:
		case FS_REMOVE_VNODE_REQUEST:
			return true;
		case LOOKUP_REPLY:
		case GET_VNODE_NAME_REPLY:
		case READ_VNODE_REPLY:
		case WRITE_VNODE_REPLY:
		case FS_REMOVE_VNODE_REPLY:
			return false;
		// asynchronous I/O
		case DO_IO_REQUEST:
		case CANCEL_IO_REQUEST:
		case ITERATIVE_IO_GET_VECS_REQUEST:
		case ITERATIVE_IO_FINISHED_REQUEST:
			return true;
		case DO_IO_REPLY:
		case CANCEL_IO_REPLY:
		case ITERATIVE_IO_GET_VECS_REPLY:
		case ITERATIVE_IO_FINISHED_REPLY:
			return false;
		// nodes
		case IOCTL_REQUEST:
		case SET_FLAGS_REQUEST:
		case SELECT_REQUEST:
		case DESELECT_REQUEST:
		case FSYNC_REQUEST:
		case READ_SYMLINK_REQUEST:
		case CREATE_SYMLINK_REQUEST:
		case LINK_REQUEST:
		case UNLINK_REQUEST:
		case RENAME_REQUEST:
		case ACCESS_REQUEST:
		case READ_STAT_REQUEST:
		case WRITE_STAT_REQUEST:
			return true;
		case IOCTL_REPLY:
		case SET_FLAGS_REPLY:
		case SELECT_REPLY:
		case DESELECT_REPLY:
		case FSYNC_REPLY:
		case READ_SYMLINK_REPLY:
		case CREATE_SYMLINK_REPLY:
		case LINK_REPLY:
		case UNLINK_REPLY:
		case RENAME_REPLY:
		case ACCESS_REPLY:
		case READ_STAT_REPLY:
		case WRITE_STAT_REPLY:
			return false;
		// files
		case CREATE_REQUEST:
		case OPEN_REQUEST:
		case CLOSE_REQUEST:
		case FREE_COOKIE_REQUEST:
		case READ_REQUEST:
		case WRITE_REQUEST:
			return true;
		case CREATE_REPLY:
		case OPEN_REPLY:
		case CLOSE_REPLY:
		case FREE_COOKIE_REPLY:
		case READ_REPLY:
		case WRITE_REPLY:
			return false;
		// directories
		case CREATE_DIR_REQUEST:
		case REMOVE_DIR_REQUEST:
		case OPEN_DIR_REQUEST:
		case CLOSE_DIR_REQUEST:
		case FREE_DIR_COOKIE_REQUEST:
		case READ_DIR_REQUEST:
		case REWIND_DIR_REQUEST:
			return true;
		case CREATE_DIR_REPLY:
		case REMOVE_DIR_REPLY:
		case OPEN_DIR_REPLY:
		case CLOSE_DIR_REPLY:
		case FREE_DIR_COOKIE_REPLY:
		case READ_DIR_REPLY:
		case REWIND_DIR_REPLY:
			return false;
		// attribute directories
		case OPEN_ATTR_DIR_REQUEST:
		case CLOSE_ATTR_DIR_REQUEST:
		case FREE_ATTR_DIR_COOKIE_REQUEST:
		case READ_ATTR_DIR_REQUEST:
		case REWIND_ATTR_DIR_REQUEST:
			return true;
		case OPEN_ATTR_DIR_REPLY:
		case CLOSE_ATTR_DIR_REPLY:
		case FREE_ATTR_DIR_COOKIE_REPLY:
		case READ_ATTR_DIR_REPLY:
		case REWIND_ATTR_DIR_REPLY:
			return false;
		// attributes
		case CREATE_ATTR_REQUEST:
		case OPEN_ATTR_REQUEST:
		case CLOSE_ATTR_REQUEST:
		case FREE_ATTR_COOKIE_REQUEST:
		case READ_ATTR_REQUEST:
		case WRITE_ATTR_REQUEST:
		case READ_ATTR_STAT_REQUEST:
		case WRITE_ATTR_STAT_REQUEST:
		case RENAME_ATTR_REQUEST:
		case REMOVE_ATTR_REQUEST:
			return true;
		case CREATE_ATTR_REPLY:
		case OPEN_ATTR_REPLY:
		case CLOSE_ATTR_REPLY:
		case FREE_ATTR_COOKIE_REPLY:
		case READ_ATTR_REPLY:
		case WRITE_ATTR_REPLY:
		case READ_ATTR_STAT_REPLY:
		case WRITE_ATTR_STAT_REPLY:
		case RENAME_ATTR_REPLY:
		case REMOVE_ATTR_REPLY:
			return false;
		// indices
		case OPEN_INDEX_DIR_REQUEST:
		case CLOSE_INDEX_DIR_REQUEST:
		case FREE_INDEX_DIR_COOKIE_REQUEST:
		case READ_INDEX_DIR_REQUEST:
		case REWIND_INDEX_DIR_REQUEST:
		case CREATE_INDEX_REQUEST:
		case REMOVE_INDEX_REQUEST:
		case READ_INDEX_STAT_REQUEST:
			return true;
		case OPEN_INDEX_DIR_REPLY:
		case CLOSE_INDEX_DIR_REPLY:
		case FREE_INDEX_DIR_COOKIE_REPLY:
		case READ_INDEX_DIR_REPLY:
		case REWIND_INDEX_DIR_REPLY:
		case CREATE_INDEX_REPLY:
		case REMOVE_INDEX_REPLY:
		case READ_INDEX_STAT_REPLY:
			return false;
		// queries
		case OPEN_QUERY_REQUEST:
		case CLOSE_QUERY_REQUEST:
		case FREE_QUERY_COOKIE_REQUEST:
		case READ_QUERY_REQUEST:
		case REWIND_QUERY_REQUEST:
			return true;
		case OPEN_QUERY_REPLY:
		case CLOSE_QUERY_REPLY:
		case FREE_QUERY_COOKIE_REPLY:
		case READ_QUERY_REPLY:
		case REWIND_QUERY_REPLY:
			return false;
		// node monitoring
		case NODE_MONITORING_EVENT_REQUEST:
			return true;
		case NODE_MONITORING_EVENT_REPLY:
			return false;

		// userland -> kernel requests
		// notifications
		case NOTIFY_LISTENER_REQUEST:
		case NOTIFY_SELECT_EVENT_REQUEST:
		case NOTIFY_QUERY_REQUEST:
			return false;
		case NOTIFY_LISTENER_REPLY:
		case NOTIFY_SELECT_EVENT_REPLY:
		case NOTIFY_QUERY_REPLY:
			return true;
		// vnodes
		case GET_VNODE_REQUEST:
		case PUT_VNODE_REQUEST:
		case ACQUIRE_VNODE_REQUEST:
		case NEW_VNODE_REQUEST:
		case PUBLISH_VNODE_REQUEST:
		case REMOVE_VNODE_REQUEST:
		case UNREMOVE_VNODE_REQUEST:
		case GET_VNODE_REMOVED_REQUEST:
			return false;
		case GET_VNODE_REPLY:
		case PUT_VNODE_REPLY:
		case ACQUIRE_VNODE_REPLY:
		case NEW_VNODE_REPLY:
		case PUBLISH_VNODE_REPLY:
		case REMOVE_VNODE_REPLY:
		case UNREMOVE_VNODE_REPLY:
		case GET_VNODE_REMOVED_REPLY:
			return true;
		// file cache
		case FILE_CACHE_CREATE_REQUEST:
		case FILE_CACHE_DELETE_REQUEST:
		case FILE_CACHE_SET_ENABLED_REQUEST:
		case FILE_CACHE_SET_SIZE_REQUEST:
		case FILE_CACHE_SYNC_REQUEST:
		case FILE_CACHE_READ_REQUEST:
		case FILE_CACHE_WRITE_REQUEST:
			return false;
		case FILE_CACHE_CREATE_REPLY:
		case FILE_CACHE_DELETE_REPLY:
		case FILE_CACHE_SET_ENABLED_REPLY:
		case FILE_CACHE_SET_SIZE_REPLY:
		case FILE_CACHE_SYNC_REPLY:
		case FILE_CACHE_READ_REPLY:
		case FILE_CACHE_WRITE_REPLY:
			return true;
		// I/O
		case DO_ITERATIVE_FD_IO_REQUEST:
		case READ_FROM_IO_REQUEST_REQUEST:
		case WRITE_TO_IO_REQUEST_REQUEST:
		case NOTIFY_IO_REQUEST_REQUEST:
			return false;
		case DO_ITERATIVE_FD_IO_REPLY:
		case NOTIFY_IO_REQUEST_REPLY:
		case READ_FROM_IO_REQUEST_REPLY:
		case WRITE_TO_IO_REQUEST_REPLY:
			return true;
		// node monitoring
		case ADD_NODE_LISTENER_REQUEST:
		case REMOVE_NODE_LISTENER_REQUEST:
			return false;
		case ADD_NODE_LISTENER_REPLY:
		case REMOVE_NODE_LISTENER_REPLY:
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
//		case INITIALIZE_VOLUME_REQUEST:
		case SYNC_VOLUME_REQUEST:
		case READ_FS_INFO_REQUEST:
		case WRITE_FS_INFO_REQUEST:
			return false;
		case MOUNT_VOLUME_REPLY:
		case UNMOUNT_VOLUME_REPLY:
//		case INITIALIZE_VOLUME_REPLY:
		case SYNC_VOLUME_REPLY:
		case READ_FS_INFO_REPLY:
		case WRITE_FS_INFO_REPLY:
			return true;
		// vnodes
		case LOOKUP_REQUEST:
		case GET_VNODE_NAME_REQUEST:
		case READ_VNODE_REQUEST:
		case WRITE_VNODE_REQUEST:
		case FS_REMOVE_VNODE_REQUEST:
			return false;
		case LOOKUP_REPLY:
		case GET_VNODE_NAME_REPLY:
		case READ_VNODE_REPLY:
		case WRITE_VNODE_REPLY:
		case FS_REMOVE_VNODE_REPLY:
			return true;
		// asynchronous I/O
		case DO_IO_REQUEST:
		case CANCEL_IO_REQUEST:
		case ITERATIVE_IO_GET_VECS_REQUEST:
		case ITERATIVE_IO_FINISHED_REQUEST:
			return false;
		case DO_IO_REPLY:
		case CANCEL_IO_REPLY:
		case ITERATIVE_IO_GET_VECS_REPLY:
		case ITERATIVE_IO_FINISHED_REPLY:
			return true;
		// nodes
		case IOCTL_REQUEST:
		case SET_FLAGS_REQUEST:
		case SELECT_REQUEST:
		case DESELECT_REQUEST:
		case FSYNC_REQUEST:
		case READ_SYMLINK_REQUEST:
		case CREATE_SYMLINK_REQUEST:
		case LINK_REQUEST:
		case UNLINK_REQUEST:
		case RENAME_REQUEST:
		case ACCESS_REQUEST:
		case READ_STAT_REQUEST:
		case WRITE_STAT_REQUEST:
			return false;
		case IOCTL_REPLY:
		case SET_FLAGS_REPLY:
		case SELECT_REPLY:
		case DESELECT_REPLY:
		case FSYNC_REPLY:
		case READ_SYMLINK_REPLY:
		case CREATE_SYMLINK_REPLY:
		case LINK_REPLY:
		case UNLINK_REPLY:
		case RENAME_REPLY:
		case ACCESS_REPLY:
		case READ_STAT_REPLY:
		case WRITE_STAT_REPLY:
			return true;
		// files
		case CREATE_REQUEST:
		case OPEN_REQUEST:
		case CLOSE_REQUEST:
		case FREE_COOKIE_REQUEST:
		case READ_REQUEST:
		case WRITE_REQUEST:
			return false;
		case CREATE_REPLY:
		case OPEN_REPLY:
		case CLOSE_REPLY:
		case FREE_COOKIE_REPLY:
		case READ_REPLY:
		case WRITE_REPLY:
			return true;
		// directories
		case CREATE_DIR_REQUEST:
		case REMOVE_DIR_REQUEST:
		case OPEN_DIR_REQUEST:
		case CLOSE_DIR_REQUEST:
		case FREE_DIR_COOKIE_REQUEST:
		case READ_DIR_REQUEST:
		case REWIND_DIR_REQUEST:
			return false;
		case CREATE_DIR_REPLY:
		case REMOVE_DIR_REPLY:
		case OPEN_DIR_REPLY:
		case CLOSE_DIR_REPLY:
		case FREE_DIR_COOKIE_REPLY:
		case READ_DIR_REPLY:
		case REWIND_DIR_REPLY:
			return true;
		// attribute directories
		case OPEN_ATTR_DIR_REQUEST:
		case CLOSE_ATTR_DIR_REQUEST:
		case FREE_ATTR_DIR_COOKIE_REQUEST:
		case READ_ATTR_DIR_REQUEST:
		case REWIND_ATTR_DIR_REQUEST:
			return false;
		case OPEN_ATTR_DIR_REPLY:
		case CLOSE_ATTR_DIR_REPLY:
		case FREE_ATTR_DIR_COOKIE_REPLY:
		case READ_ATTR_DIR_REPLY:
		case REWIND_ATTR_DIR_REPLY:
			return true;
		// attributes
		case CREATE_ATTR_REQUEST:
		case OPEN_ATTR_REQUEST:
		case CLOSE_ATTR_REQUEST:
		case FREE_ATTR_COOKIE_REQUEST:
		case READ_ATTR_REQUEST:
		case WRITE_ATTR_REQUEST:
		case RENAME_ATTR_REQUEST:
		case READ_ATTR_STAT_REQUEST:
		case WRITE_ATTR_STAT_REQUEST:
		case REMOVE_ATTR_REQUEST:
			return false;
		case CREATE_ATTR_REPLY:
		case OPEN_ATTR_REPLY:
		case CLOSE_ATTR_REPLY:
		case FREE_ATTR_COOKIE_REPLY:
		case READ_ATTR_REPLY:
		case WRITE_ATTR_REPLY:
		case READ_ATTR_STAT_REPLY:
		case WRITE_ATTR_STAT_REPLY:
		case RENAME_ATTR_REPLY:
		case REMOVE_ATTR_REPLY:
			return true;
		// indices
		case OPEN_INDEX_DIR_REQUEST:
		case CLOSE_INDEX_DIR_REQUEST:
		case FREE_INDEX_DIR_COOKIE_REQUEST:
		case READ_INDEX_DIR_REQUEST:
		case REWIND_INDEX_DIR_REQUEST:
		case CREATE_INDEX_REQUEST:
		case REMOVE_INDEX_REQUEST:
		case READ_INDEX_STAT_REQUEST:
			return false;
		case OPEN_INDEX_DIR_REPLY:
		case CLOSE_INDEX_DIR_REPLY:
		case FREE_INDEX_DIR_COOKIE_REPLY:
		case READ_INDEX_DIR_REPLY:
		case REWIND_INDEX_DIR_REPLY:
		case CREATE_INDEX_REPLY:
		case REMOVE_INDEX_REPLY:
		case READ_INDEX_STAT_REPLY:
			return true;
		// queries
		case OPEN_QUERY_REQUEST:
		case CLOSE_QUERY_REQUEST:
		case FREE_QUERY_COOKIE_REQUEST:
		case READ_QUERY_REQUEST:
		case REWIND_QUERY_REQUEST:
			return false;
		case OPEN_QUERY_REPLY:
		case CLOSE_QUERY_REPLY:
		case FREE_QUERY_COOKIE_REPLY:
		case READ_QUERY_REPLY:
		case REWIND_QUERY_REPLY:
			return true;
		// node monitoring
		case NODE_MONITORING_EVENT_REQUEST:
			return false;
		case NODE_MONITORING_EVENT_REPLY:
			return true;

		// userland -> kernel requests
		// notifications
		case NOTIFY_LISTENER_REQUEST:
		case NOTIFY_SELECT_EVENT_REQUEST:
		case NOTIFY_QUERY_REQUEST:
			return true;
		case NOTIFY_LISTENER_REPLY:
		case NOTIFY_SELECT_EVENT_REPLY:
		case NOTIFY_QUERY_REPLY:
			return false;
		// vnodes
		case GET_VNODE_REQUEST:
		case PUT_VNODE_REQUEST:
		case ACQUIRE_VNODE_REQUEST:
		case NEW_VNODE_REQUEST:
		case PUBLISH_VNODE_REQUEST:
		case REMOVE_VNODE_REQUEST:
		case UNREMOVE_VNODE_REQUEST:
		case GET_VNODE_REMOVED_REQUEST:
			return true;
		case GET_VNODE_REPLY:
		case PUT_VNODE_REPLY:
		case ACQUIRE_VNODE_REPLY:
		case NEW_VNODE_REPLY:
		case PUBLISH_VNODE_REPLY:
		case REMOVE_VNODE_REPLY:
		case UNREMOVE_VNODE_REPLY:
		case GET_VNODE_REMOVED_REPLY:
			return false;
		// file cache
		case FILE_CACHE_CREATE_REQUEST:
		case FILE_CACHE_DELETE_REQUEST:
		case FILE_CACHE_SET_ENABLED_REQUEST:
		case FILE_CACHE_SET_SIZE_REQUEST:
		case FILE_CACHE_SYNC_REQUEST:
		case FILE_CACHE_READ_REQUEST:
		case FILE_CACHE_WRITE_REQUEST:
			return true;
		case FILE_CACHE_CREATE_REPLY:
		case FILE_CACHE_DELETE_REPLY:
		case FILE_CACHE_SET_ENABLED_REPLY:
		case FILE_CACHE_SET_SIZE_REPLY:
		case FILE_CACHE_SYNC_REPLY:
		case FILE_CACHE_READ_REPLY:
		case FILE_CACHE_WRITE_REPLY:
			return false;
		// I/O
		case DO_ITERATIVE_FD_IO_REQUEST:
		case NOTIFY_IO_REQUEST_REQUEST:
		case READ_FROM_IO_REQUEST_REQUEST:
		case WRITE_TO_IO_REQUEST_REQUEST:
			return true;
		case DO_ITERATIVE_FD_IO_REPLY:
		case NOTIFY_IO_REQUEST_REPLY:
		case READ_FROM_IO_REQUEST_REPLY:
		case WRITE_TO_IO_REQUEST_REPLY:
			return false;
		// node monitoring
		case ADD_NODE_LISTENER_REQUEST:
		case REMOVE_NODE_LISTENER_REQUEST:
			return true;
		case ADD_NODE_LISTENER_REPLY:
		case REMOVE_NODE_LISTENER_REPLY:
			return false;

		// general reply
		case RECEIPT_ACK_REPLY:
			return true;
		default:
			return false;
	}
}
