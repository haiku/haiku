// Requests.h

#ifndef USERLAND_FS_REQUESTS_H
#define USERLAND_FS_REQUESTS_H

#include <fs_attr.h>
#include <fs_index.h>
#include <fs_info.h>
#include <fs_interface.h>

#include "Compatibility.h"
#include "Request.h"


enum {
	MAX_REQUEST_ADDRESS_COUNT	= 4,
};

// request types
enum {
	// kernel -> userland requests

	// administrative
	UFS_DISCONNECT_REQUEST = 0,
	FS_CONNECT_REQUEST,
	FS_CONNECT_REPLY,

	// FS
	MOUNT_VOLUME_REQUEST,
	MOUNT_VOLUME_REPLY,
	UNMOUNT_VOLUME_REQUEST,
	UNMOUNT_VOLUME_REPLY,
//	INITIALIZE_VOLUME_REQUEST,
//	INITIALIZE_VOLUME_REPLY,
	SYNC_VOLUME_REQUEST,
	SYNC_VOLUME_REPLY,
	READ_FS_INFO_REQUEST,
	READ_FS_INFO_REPLY,
	WRITE_FS_INFO_REQUEST,
	WRITE_FS_INFO_REPLY,

	// vnodes
	LOOKUP_REQUEST,
	LOOKUP_REPLY,
	READ_VNODE_REQUEST,
	READ_VNODE_REPLY,
	WRITE_VNODE_REQUEST,
	WRITE_VNODE_REPLY,
	FS_REMOVE_VNODE_REQUEST,
	FS_REMOVE_VNODE_REPLY,

	// nodes
	IOCTL_REQUEST,
	IOCTL_REPLY,
	SET_FLAGS_REQUEST,
	SET_FLAGS_REPLY,
	SELECT_REQUEST,
	SELECT_REPLY,
	DESELECT_REQUEST,
	DESELECT_REPLY,
	FSYNC_REQUEST,
	FSYNC_REPLY,

	READ_SYMLINK_REQUEST,
	READ_SYMLINK_REPLY,
	CREATE_SYMLINK_REQUEST,
	CREATE_SYMLINK_REPLY,
	LINK_REQUEST,
	LINK_REPLY,
	UNLINK_REQUEST,
	UNLINK_REPLY,
	RENAME_REQUEST,
	RENAME_REPLY,

	ACCESS_REQUEST,
	ACCESS_REPLY,
	READ_STAT_REQUEST,
	READ_STAT_REPLY,
	WRITE_STAT_REQUEST,
	WRITE_STAT_REPLY,

	// files
	CREATE_REQUEST,
	CREATE_REPLY,
	OPEN_REQUEST,
	OPEN_REPLY,
	CLOSE_REQUEST,
	CLOSE_REPLY,
	FREE_COOKIE_REQUEST,
	FREE_COOKIE_REPLY,
	READ_REQUEST,
	READ_REPLY,
	WRITE_REQUEST,
	WRITE_REPLY,

	// directories
	CREATE_DIR_REQUEST,
	CREATE_DIR_REPLY,
	REMOVE_DIR_REQUEST,
	REMOVE_DIR_REPLY,
	OPEN_DIR_REQUEST,
	OPEN_DIR_REPLY,
	CLOSE_DIR_REQUEST,
	CLOSE_DIR_REPLY,
	FREE_DIR_COOKIE_REQUEST,
	FREE_DIR_COOKIE_REPLY,
	READ_DIR_REQUEST,
	READ_DIR_REPLY,
	REWIND_DIR_REQUEST,
	REWIND_DIR_REPLY,

	// attribute directories
	OPEN_ATTR_DIR_REQUEST,
	OPEN_ATTR_DIR_REPLY,
	CLOSE_ATTR_DIR_REQUEST,
	CLOSE_ATTR_DIR_REPLY,
	FREE_ATTR_DIR_COOKIE_REQUEST,
	FREE_ATTR_DIR_COOKIE_REPLY,
	READ_ATTR_DIR_REQUEST,
	READ_ATTR_DIR_REPLY,
	REWIND_ATTR_DIR_REQUEST,
	REWIND_ATTR_DIR_REPLY,

	// attributes
	READ_ATTR_REQUEST,
	READ_ATTR_REPLY,
	WRITE_ATTR_REQUEST,
	WRITE_ATTR_REPLY,
	READ_ATTR_STAT_REQUEST,
	READ_ATTR_STAT_REPLY,
	RENAME_ATTR_REQUEST,
	RENAME_ATTR_REPLY,
	REMOVE_ATTR_REQUEST,
	REMOVE_ATTR_REPLY,

	// indices
	OPEN_INDEX_DIR_REQUEST,
	OPEN_INDEX_DIR_REPLY,
	CLOSE_INDEX_DIR_REQUEST,
	CLOSE_INDEX_DIR_REPLY,
	FREE_INDEX_DIR_COOKIE_REQUEST,
	FREE_INDEX_DIR_COOKIE_REPLY,
	READ_INDEX_DIR_REQUEST,
	READ_INDEX_DIR_REPLY,
	REWIND_INDEX_DIR_REQUEST,
	REWIND_INDEX_DIR_REPLY,
	CREATE_INDEX_REQUEST,
	CREATE_INDEX_REPLY,
	REMOVE_INDEX_REQUEST,
	REMOVE_INDEX_REPLY,
	READ_INDEX_STAT_REQUEST,
	READ_INDEX_STAT_REPLY,

	// queries
	OPEN_QUERY_REQUEST,
	OPEN_QUERY_REPLY,
	CLOSE_QUERY_REQUEST,
	CLOSE_QUERY_REPLY,
	FREE_QUERY_COOKIE_REQUEST,
	FREE_QUERY_COOKIE_REPLY,
	READ_QUERY_REQUEST,
	READ_QUERY_REPLY,

	// userland -> kernel requests
	// notifications
	NOTIFY_LISTENER_REQUEST,
	NOTIFY_LISTENER_REPLY,
	NOTIFY_SELECT_EVENT_REQUEST,
	NOTIFY_SELECT_EVENT_REPLY,
	SEND_NOTIFICATION_REQUEST,
	SEND_NOTIFICATION_REPLY,

	// vnodes
	GET_VNODE_REQUEST,
	GET_VNODE_REPLY,
	PUT_VNODE_REQUEST,
	PUT_VNODE_REPLY,
	NEW_VNODE_REQUEST,
	NEW_VNODE_REPLY,
	REMOVE_VNODE_REQUEST,
	REMOVE_VNODE_REPLY,
	UNREMOVE_VNODE_REQUEST,
	UNREMOVE_VNODE_REPLY,
	IS_VNODE_REMOVED_REQUEST,
	IS_VNODE_REMOVED_REPLY,

	// general reply
	RECEIPT_ACK_REPLY,

	// invalid request ID (e.g. for request handlers)
	NO_REQUEST,
};

namespace UserlandFSUtil {

// ReplyRequest
class ReplyRequest : public Request {
public:
	ReplyRequest(uint32 type) : Request(type) {}

	status_t	error;
};


// #pragma mark - kernel requests


// VolumeRequest
class VolumeRequest : public Request {
public:
	VolumeRequest(uint32 type) : Request(type) {}

	fs_volume	volume;
};

// NodeRequest
class NodeRequest : public VolumeRequest {
public:
	NodeRequest(uint32 type) : VolumeRequest(type) {}

	fs_vnode	node;
};

// FileRequest
class FileRequest : public NodeRequest {
public:
	FileRequest(uint32 type) : NodeRequest(type) {}

	fs_cookie	fileCookie;
};

// DirRequest
class DirRequest : public NodeRequest {
public:
	DirRequest(uint32 type) : NodeRequest(type) {}

	fs_cookie	dirCookie;
};

// AttrDirRequest
class AttrDirRequest : public NodeRequest {
public:
	AttrDirRequest(uint32 type) : NodeRequest(type) {}

	fs_cookie	attrDirCookie;
};

// AttributeRequest
class AttributeRequest : public NodeRequest {
public:
	AttributeRequest(uint32 type) : NodeRequest(type) {}

	fs_cookie	attrCookie;
};

// IndexDirRequest
class IndexDirRequest : public VolumeRequest {
public:
	IndexDirRequest(uint32 type) : VolumeRequest(type) {}

	fs_cookie	indexDirCookie;
};

// QueryRequest
class QueryRequest : public VolumeRequest {
public:
	QueryRequest(uint32 type) : VolumeRequest(type) {}

	fs_cookie	queryCookie;
};


// #pragma mark - administrative


// UFSDisconnectRequest
class UFSDisconnectRequest : public Request {
public:
	UFSDisconnectRequest() : Request(UFS_DISCONNECT_REQUEST) {}
};

// FSConnectRequest
class FSConnectRequest : public Request {
public:
	FSConnectRequest() : Request(FS_CONNECT_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	Address		fsName;
};

// FSConnectReply
class FSConnectReply : public ReplyRequest {
public:
	FSConnectReply() : ReplyRequest(FS_CONNECT_REPLY) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	Address		portInfos;
	int32		portInfoCount;
};


// #pragma mark - FS


// MountVolumeRequest
class MountVolumeRequest : public Request {
public:
	MountVolumeRequest() : Request(MOUNT_VOLUME_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	mount_id	nsid;
	Address		cwd;			// current working dir of the mount() caller
	Address		device;
	uint32		flags;
	Address		parameters;
};

// MountVolumeReply
class MountVolumeReply : public ReplyRequest {
public:
	MountVolumeReply() : ReplyRequest(MOUNT_VOLUME_REPLY) {}

	vnode_id	rootID;
	fs_volume	volume;
};

// UnmountVolumeRequest
class UnmountVolumeRequest : public VolumeRequest {
public:
	UnmountVolumeRequest() : VolumeRequest(UNMOUNT_VOLUME_REQUEST) {}
};

// UnmountVolumeReply
class UnmountVolumeReply : public ReplyRequest {
public:
	UnmountVolumeReply() : ReplyRequest(UNMOUNT_VOLUME_REPLY) {}
};

/*// InitializeVolumeRequest
class InitializeVolumeRequest : public Request {
public:
	InitializeVolumeRequest() : Request(INITIALIZE_VOLUME_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	Address		device;
	Address		parameters;
};

// InitializeVolumeReply
class InitializeVolumeReply : public ReplyRequest {
public:
	InitializeVolumeReply() : ReplyRequest(INITIALIZE_VOLUME_REPLY) {}
};*/

// SyncVolumeRequest
class SyncVolumeRequest : public VolumeRequest {
public:
	SyncVolumeRequest() : VolumeRequest(SYNC_VOLUME_REQUEST) {}
};

// SyncVolumeReply
class SyncVolumeReply : public ReplyRequest {
public:
	SyncVolumeReply() : ReplyRequest(SYNC_VOLUME_REPLY) {}
};

// ReadFSInfoRequest
class ReadFSInfoRequest : public VolumeRequest {
public:
	ReadFSInfoRequest() : VolumeRequest(READ_FS_INFO_REQUEST) {}
};

// ReadFSInfoReply
class ReadFSInfoReply : public ReplyRequest {
public:
	ReadFSInfoReply() : ReplyRequest(READ_FS_INFO_REPLY) {}

	fs_info		info;
};

// WriteFSInfoRequest
class WriteFSInfoRequest : public VolumeRequest {
public:
	WriteFSInfoRequest() : VolumeRequest(WRITE_FS_INFO_REQUEST) {}

	fs_info		info;
	long		mask;
};

// WriteFSInfoReply
class WriteFSInfoReply : public ReplyRequest {
public:
	WriteFSInfoReply() : ReplyRequest(WRITE_FS_INFO_REPLY) {}
};


// #pragma mark - vnodes


// LookupRequest
class LookupRequest : public NodeRequest {
public:
	LookupRequest() : NodeRequest(LOOKUP_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	Address		entryName;
	bool		traverseLink;
};

// LookupReply
class LookupReply : public ReplyRequest {
public:
	LookupReply() : ReplyRequest(LOOKUP_REPLY) {}

	Address		resolvedPath;
	vnode_id	vnid;
	int			type;
};

// ReadVNodeRequest
class ReadVNodeRequest : public VolumeRequest {
public:
	ReadVNodeRequest() : VolumeRequest(READ_VNODE_REQUEST) {}

	vnode_id	vnid;
	bool		reenter;
};

// ReadVNodeReply
class ReadVNodeReply : public ReplyRequest {
public:
	ReadVNodeReply() : ReplyRequest(READ_VNODE_REPLY) {}

	fs_vnode	node;
};

// WriteVNodeRequest
class WriteVNodeRequest : public NodeRequest {
public:
	WriteVNodeRequest() : NodeRequest(WRITE_VNODE_REQUEST) {}

	bool		reenter;
};

// WriteVNodeReply
class WriteVNodeReply : public ReplyRequest {
public:
	WriteVNodeReply() : ReplyRequest(WRITE_VNODE_REPLY) {}
};

// FSRemoveVNodeRequest
class FSRemoveVNodeRequest : public NodeRequest {
public:
	FSRemoveVNodeRequest() : NodeRequest(FS_REMOVE_VNODE_REQUEST) {}

	bool		reenter;
};

// FSRemoveVNodeReply
class FSRemoveVNodeReply : public ReplyRequest {
public:
	FSRemoveVNodeReply() : ReplyRequest(FS_REMOVE_VNODE_REPLY) {}
};


// #pragma mark - nodes


// IOCtlRequest
class IOCtlRequest : public FileRequest {
public:
	IOCtlRequest() : FileRequest(IOCTL_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	uint32		command;
	void*		bufferParameter;
	size_t		lenParameter;
	bool		isBuffer;	// if false, just pass bufferParameter
							// otherwise use buffer
	Address		buffer;
	int32		writeSize;	// ignored unless isBuffer -- then
							// it indicates the size of the buffer to allocate
};

// IOCtlReply
class IOCtlReply : public ReplyRequest {
public:
	IOCtlReply() : ReplyRequest(IOCTL_REPLY) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	status_t	ioctlError;		// we need a special handling since error
								// may be a part of the client FS protocol
	Address		buffer;
};

// SetFlagsRequest
class SetFlagsRequest : public FileRequest {
public:
	SetFlagsRequest() : FileRequest(SET_FLAGS_REQUEST) {}

	int			flags;
};

// SetFlagsReply
class SetFlagsReply : public ReplyRequest {
public:
	SetFlagsReply() : ReplyRequest(SET_FLAGS_REPLY) {}
};

// SelectRequest
class SelectRequest : public FileRequest {
public:
	SelectRequest() : FileRequest(SELECT_REQUEST) {}

	uint8		event;
	uint32		ref;
	selectsync*	sync;
};

// SelectReply
class SelectReply : public ReplyRequest {
public:
	SelectReply() : ReplyRequest(SELECT_REPLY) {}
};

// DeselectRequest
class DeselectRequest : public FileRequest {
public:
	DeselectRequest() : FileRequest(DESELECT_REQUEST) {}

	uint8		event;
	selectsync*	sync;
};

// DeselectReply
class DeselectReply : public ReplyRequest {
public:
	DeselectReply() : ReplyRequest(DESELECT_REPLY) {}
};

// FSyncRequest
class FSyncRequest : public NodeRequest {
public:
	FSyncRequest() : NodeRequest(FSYNC_REQUEST) {}
};

// FSyncReply
class FSyncReply : public ReplyRequest {
public:
	FSyncReply() : ReplyRequest(FSYNC_REPLY) {}
};

// ReadSymlinkRequest
class ReadSymlinkRequest : public NodeRequest {
public:
	ReadSymlinkRequest() : NodeRequest(READ_SYMLINK_REQUEST) {}

	size_t		size;
};

// ReadSymlinkReply
class ReadSymlinkReply : public ReplyRequest {
public:
	ReadSymlinkReply() : ReplyRequest(READ_SYMLINK_REPLY) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	Address		buffer;
	size_t		bytesRead;
};

// CreateSymlinkRequest
class CreateSymlinkRequest : public NodeRequest {
public:
	CreateSymlinkRequest() : NodeRequest(CREATE_SYMLINK_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	Address		name;
	Address		target;
	int			mode;
};

// CreateSymlinkReply
class CreateSymlinkReply : public ReplyRequest {
public:
	CreateSymlinkReply() : ReplyRequest(CREATE_SYMLINK_REPLY) {}
};

// LinkRequest
class LinkRequest : public NodeRequest {
public:
	LinkRequest() : NodeRequest(LINK_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	Address		name;
	fs_vnode	target;
};

// LinkReply
class LinkReply : public ReplyRequest {
public:
	LinkReply() : ReplyRequest(LINK_REPLY) {}
};

// UnlinkRequest
class UnlinkRequest : public NodeRequest {
public:
	UnlinkRequest() : NodeRequest(UNLINK_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	Address		name;
};

// UnlinkReply
class UnlinkReply : public ReplyRequest {
public:
	UnlinkReply() : ReplyRequest(UNLINK_REPLY) {}
};

// RenameRequest
class RenameRequest : public VolumeRequest {
public:
	RenameRequest() : VolumeRequest(RENAME_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	fs_vnode	oldDir;
	Address		oldName;
	fs_vnode	newDir;
	Address		newName;
};

// RenameReply
class RenameReply : public ReplyRequest {
public:
	RenameReply() : ReplyRequest(RENAME_REPLY) {}
};

// AccessRequest
class AccessRequest : public NodeRequest {
public:
	AccessRequest() : NodeRequest(ACCESS_REQUEST) {}

	int			mode;
};

// AccessReply
class AccessReply : public ReplyRequest {
public:
	AccessReply() : ReplyRequest(ACCESS_REPLY) {}
};

// ReadStatRequest
class ReadStatRequest : public NodeRequest {
public:
	ReadStatRequest() : NodeRequest(READ_STAT_REQUEST) {}
};

// ReadStatReply
class ReadStatReply : public ReplyRequest {
public:
	ReadStatReply() : ReplyRequest(READ_STAT_REPLY) {}

	struct stat	st;
};

// WriteStatRequest
class WriteStatRequest : public NodeRequest {
public:
	WriteStatRequest() : NodeRequest(WRITE_STAT_REQUEST) {}

	struct stat	st;
	uint32		mask;
};

// WriteStatReply
class WriteStatReply : public ReplyRequest {
public:
	WriteStatReply() : ReplyRequest(WRITE_STAT_REPLY) {}
};


// #pragma mark - files


// CreateRequest
class CreateRequest : public NodeRequest {
public:
	CreateRequest() : NodeRequest(CREATE_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	Address		name;
	int			openMode;
	int			mode;
};

// CreateReply
class CreateReply : public ReplyRequest {
public:
	CreateReply() : ReplyRequest(CREATE_REPLY) {}

	vnode_id	vnid;
	fs_cookie	fileCookie;
};

// OpenRequest
class OpenRequest : public NodeRequest {
public:
	OpenRequest() : NodeRequest(OPEN_REQUEST) {}

	int			openMode;
};

// OpenReply
class OpenReply : public ReplyRequest {
public:
	OpenReply() : ReplyRequest(OPEN_REPLY) {}

	fs_cookie	fileCookie;
};

// CloseRequest
class CloseRequest : public FileRequest {
public:
	CloseRequest() : FileRequest(CLOSE_REQUEST) {}
};

// CloseReply
class CloseReply : public ReplyRequest {
public:
	CloseReply() : ReplyRequest(CLOSE_REPLY) {}
};

// FreeCookieRequest
class FreeCookieRequest : public FileRequest {
public:
	FreeCookieRequest() : FileRequest(FREE_COOKIE_REQUEST) {}
};

// FreeCookieReply
class FreeCookieReply : public ReplyRequest {
public:
	FreeCookieReply() : ReplyRequest(FREE_COOKIE_REPLY) {}
};

// ReadRequest
class ReadRequest : public FileRequest {
public:
	ReadRequest() : FileRequest(READ_REQUEST) {}

	off_t		pos;
	size_t		size;
};

// ReadReply
class ReadReply : public ReplyRequest {
public:
	ReadReply() : ReplyRequest(READ_REPLY) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	Address		buffer;
	size_t		bytesRead;
};

// WriteRequest
class WriteRequest : public FileRequest {
public:
	WriteRequest() : FileRequest(WRITE_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	Address		buffer;
	off_t		pos;
};

// WriteReply
class WriteReply : public ReplyRequest {
public:
	WriteReply() : ReplyRequest(WRITE_REPLY) {}

	size_t		bytesWritten;
};


// #pragma mark - directories


// CreateDirRequest
class CreateDirRequest : public NodeRequest {
public:
	CreateDirRequest() : NodeRequest(CREATE_DIR_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	Address		name;
	int			mode;
};

// CreateDirReply
class CreateDirReply : public ReplyRequest {
public:
	CreateDirReply() : ReplyRequest(CREATE_DIR_REPLY) {}

	vnode_id	newDir;
};

// RemoveDirRequest
class RemoveDirRequest : public NodeRequest {
public:
	RemoveDirRequest() : NodeRequest(REMOVE_DIR_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	Address		name;
};

// RemoveDirReply
class RemoveDirReply : public ReplyRequest {
public:
	RemoveDirReply() : ReplyRequest(REMOVE_DIR_REPLY) {}
};

// OpenDirRequest
class OpenDirRequest : public NodeRequest {
public:
	OpenDirRequest() : NodeRequest(OPEN_DIR_REQUEST) {}
};

// OpenDirReply
class OpenDirReply : public ReplyRequest {
public:
	OpenDirReply() : ReplyRequest(OPEN_DIR_REPLY) {}

	fs_cookie	dirCookie;
};

// CloseDirRequest
class CloseDirRequest : public DirRequest {
public:
	CloseDirRequest() : DirRequest(CLOSE_DIR_REQUEST) {}
};

// CloseDirReply
class CloseDirReply : public ReplyRequest {
public:
	CloseDirReply() : ReplyRequest(CLOSE_DIR_REPLY) {}
};

// FreeDirCookieRequest
class FreeDirCookieRequest : public DirRequest {
public:
	FreeDirCookieRequest() : DirRequest(FREE_DIR_COOKIE_REQUEST) {}
};

// FreeDirCookieReply
class FreeDirCookieReply : public ReplyRequest {
public:
	FreeDirCookieReply() : ReplyRequest(FREE_DIR_COOKIE_REPLY) {}
};

// ReadDirRequest
class ReadDirRequest : public DirRequest {
public:
	ReadDirRequest() : DirRequest(READ_DIR_REQUEST) {}

	size_t		bufferSize;
	uint32		count;
};

// ReadDirReply
class ReadDirReply : public ReplyRequest {
public:
	ReadDirReply() : ReplyRequest(READ_DIR_REPLY) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	uint32		count;
	Address		buffer;
};

// RewindDirRequest
class RewindDirRequest : public DirRequest {
public:
	RewindDirRequest() : DirRequest(REWIND_DIR_REQUEST) {}
};

// RewindDirReply
class RewindDirReply : public ReplyRequest {
public:
	RewindDirReply() : ReplyRequest(REWIND_DIR_REPLY) {}
};


// #pragma mark - attribute directories


// OpenAttrDirRequest
class OpenAttrDirRequest : public NodeRequest {
public:
	OpenAttrDirRequest() : NodeRequest(OPEN_ATTR_DIR_REQUEST) {}
};

// OpenAttrDirReply
class OpenAttrDirReply : public ReplyRequest {
public:
	OpenAttrDirReply() : ReplyRequest(OPEN_ATTR_DIR_REPLY) {}

	fs_cookie	attrDirCookie;
};

// CloseAttrDirRequest
class CloseAttrDirRequest : public AttrDirRequest {
public:
	CloseAttrDirRequest() : AttrDirRequest(CLOSE_ATTR_DIR_REQUEST) {}
};

// CloseAttrDirReply
class CloseAttrDirReply : public ReplyRequest {
public:
	CloseAttrDirReply() : ReplyRequest(CLOSE_ATTR_DIR_REPLY) {}
};

// FreeAttrDirCookieRequest
class FreeAttrDirCookieRequest : public AttrDirRequest {
public:
	FreeAttrDirCookieRequest() : AttrDirRequest(FREE_ATTR_DIR_COOKIE_REQUEST) {}
};

// FreeAttrDirCookieReply
class FreeAttrDirCookieReply : public ReplyRequest {
public:
	FreeAttrDirCookieReply() : ReplyRequest(FREE_ATTR_DIR_COOKIE_REPLY) {}
};

// ReadAttrDirRequest
class ReadAttrDirRequest : public AttrDirRequest {
public:
	ReadAttrDirRequest() : AttrDirRequest(READ_ATTR_DIR_REQUEST) {}

	size_t		bufferSize;
	uint32		count;
};

// ReadAttrDirReply
class ReadAttrDirReply : public ReplyRequest {
public:
	ReadAttrDirReply() : ReplyRequest(READ_ATTR_DIR_REPLY) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	uint32		count;
	Address		buffer;
};

// RewindAttrDirRequest
class RewindAttrDirRequest : public AttrDirRequest {
public:
	RewindAttrDirRequest() : AttrDirRequest(REWIND_ATTR_DIR_REQUEST) {}
};

// RewindAttrDirReply
class RewindAttrDirReply : public ReplyRequest {
public:
	RewindAttrDirReply() : ReplyRequest(REWIND_ATTR_DIR_REPLY) {}
};


// #pragma mark - attributes


// ReadAttrRequest
class ReadAttrRequest : public AttributeRequest {
public:
	ReadAttrRequest() : AttributeRequest(READ_ATTR_REQUEST) {}

	off_t		pos;
	size_t		size;
};

// ReadAttrReply
class ReadAttrReply : public ReplyRequest {
public:
	ReadAttrReply() : ReplyRequest(READ_ATTR_REPLY) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	Address		buffer;
	size_t		bytesRead;
};

// WriteAttrRequest
class WriteAttrRequest : public AttributeRequest {
public:
	WriteAttrRequest() : AttributeRequest(WRITE_ATTR_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	Address		buffer;
	off_t		pos;
	size_t		size;
};

// WriteAttrReply
class WriteAttrReply : public ReplyRequest {
public:
	WriteAttrReply() : ReplyRequest(WRITE_ATTR_REPLY) {}

	size_t		bytesWritten;
};

// ReadAttrStatRequest
class ReadAttrStatRequest : public AttributeRequest {
public:
	ReadAttrStatRequest() : AttributeRequest(READ_ATTR_STAT_REQUEST) {}
};

// ReadAttrStatReply
class ReadAttrStatReply : public ReplyRequest {
public:
	ReadAttrStatReply() : ReplyRequest(READ_ATTR_STAT_REPLY) {}

	struct stat	st;
};

// RenameAttrRequest
class RenameAttrRequest : public VolumeRequest {
public:
	RenameAttrRequest() : VolumeRequest(RENAME_ATTR_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	fs_vnode	oldNode;
	fs_vnode	newNode;
	Address		oldName;
	Address		newName;
};

// RenameAttrReply
class RenameAttrReply : public ReplyRequest {
public:
	RenameAttrReply() : ReplyRequest(RENAME_ATTR_REPLY) {}
};

// RemoveAttrRequest
class RemoveAttrRequest : public NodeRequest {
public:
	RemoveAttrRequest() : NodeRequest(REMOVE_ATTR_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	Address		name;
};

// RemoveAttrReply
class RemoveAttrReply : public ReplyRequest {
public:
	RemoveAttrReply() : ReplyRequest(REMOVE_ATTR_REPLY) {}
};


// #pragma mark - indices


// OpenIndexDirRequest
class OpenIndexDirRequest : public VolumeRequest {
public:
	OpenIndexDirRequest() : VolumeRequest(OPEN_INDEX_DIR_REQUEST) {}
};

// OpenIndexDirReply
class OpenIndexDirReply : public ReplyRequest {
public:
	OpenIndexDirReply() : ReplyRequest(OPEN_INDEX_DIR_REPLY) {}

	fs_cookie	indexDirCookie;
};

// CloseIndexDirRequest
class CloseIndexDirRequest : public IndexDirRequest {
public:
	CloseIndexDirRequest() : IndexDirRequest(CLOSE_INDEX_DIR_REQUEST) {}
};

// CloseIndexDirReply
class CloseIndexDirReply : public ReplyRequest {
public:
	CloseIndexDirReply() : ReplyRequest(CLOSE_INDEX_DIR_REPLY) {}
};

// FreeIndexDirCookieRequest
class FreeIndexDirCookieRequest : public IndexDirRequest {
public:
	FreeIndexDirCookieRequest()
		: IndexDirRequest(FREE_INDEX_DIR_COOKIE_REQUEST) {}
};

// FreeIndexDirCookieReply
class FreeIndexDirCookieReply : public ReplyRequest {
public:
	FreeIndexDirCookieReply() : ReplyRequest(FREE_INDEX_DIR_COOKIE_REPLY) {}
};

// ReadIndexDirRequest
class ReadIndexDirRequest : public IndexDirRequest {
public:
	ReadIndexDirRequest() : IndexDirRequest(READ_INDEX_DIR_REQUEST) {}

	size_t		bufferSize;
	uint32		count;
};

// ReadIndexDirReply
class ReadIndexDirReply : public ReplyRequest {
public:
	ReadIndexDirReply() : ReplyRequest(READ_INDEX_DIR_REPLY) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	uint32		count;
	Address		buffer;
};

// RewindIndexDirRequest
class RewindIndexDirRequest : public IndexDirRequest {
public:
	RewindIndexDirRequest() : IndexDirRequest(REWIND_INDEX_DIR_REQUEST) {}
};

// RewindIndexDirReply
class RewindIndexDirReply : public ReplyRequest {
public:
	RewindIndexDirReply() : ReplyRequest(REWIND_INDEX_DIR_REPLY) {}
};

// CreateIndexRequest
class CreateIndexRequest : public VolumeRequest {
public:
	CreateIndexRequest() : VolumeRequest(CREATE_INDEX_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	Address		name;
	uint32		type;
	uint32		flags;
};

// CreateIndexReply
class CreateIndexReply : public ReplyRequest {
public:
	CreateIndexReply() : ReplyRequest(CREATE_INDEX_REPLY) {}
};

// RemoveIndexRequest
class RemoveIndexRequest : public VolumeRequest {
public:
	RemoveIndexRequest() : VolumeRequest(REMOVE_INDEX_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	Address		name;
};

// RemoveIndexReply
class RemoveIndexReply : public ReplyRequest {
public:
	RemoveIndexReply() : ReplyRequest(REMOVE_INDEX_REPLY) {}
};

// ReadIndexStatRequest
class ReadIndexStatRequest : public VolumeRequest {
public:
	ReadIndexStatRequest() : VolumeRequest(READ_INDEX_STAT_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	Address		name;
};

// ReadIndexStatReply
class ReadIndexStatReply : public ReplyRequest {
public:
	ReadIndexStatReply() : ReplyRequest(READ_INDEX_STAT_REPLY) {}

	struct stat	st;
};


// #pragma mark - queries


// OpenQueryRequest
class OpenQueryRequest : public VolumeRequest {
public:
	OpenQueryRequest() : VolumeRequest(OPEN_QUERY_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	Address		queryString;
	uint32		flags;
	port_id		port;
	uint32		token;
};

// OpenQueryReply
class OpenQueryReply : public ReplyRequest {
public:
	OpenQueryReply() : ReplyRequest(OPEN_QUERY_REPLY) {}

	fs_cookie	queryCookie;
};

// CloseQueryRequest
class CloseQueryRequest : public QueryRequest {
public:
	CloseQueryRequest() : QueryRequest(CLOSE_QUERY_REQUEST) {}
};

// CloseQueryReply
class CloseQueryReply : public ReplyRequest {
public:
	CloseQueryReply() : ReplyRequest(CLOSE_QUERY_REPLY) {}
};

// FreeQueryCookieRequest
class FreeQueryCookieRequest : public QueryRequest {
public:
	FreeQueryCookieRequest() : QueryRequest(FREE_QUERY_COOKIE_REQUEST) {}
};

// FreeQueryCookieReply
class FreeQueryCookieReply : public ReplyRequest {
public:
	FreeQueryCookieReply() : ReplyRequest(FREE_QUERY_COOKIE_REPLY) {}
};

// ReadQueryRequest
class ReadQueryRequest : public QueryRequest {
public:
	ReadQueryRequest() : QueryRequest(READ_QUERY_REQUEST) {}

	size_t		bufferSize;
	uint32		count;
};

// ReadQueryReply
class ReadQueryReply : public ReplyRequest {
public:
	ReadQueryReply() : ReplyRequest(READ_QUERY_REPLY) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	uint32		count;
	Address		buffer;
};


// #pragma mark -
// #pragma mark ----- userland requests -----

// #pragma mark -
// #pragma mark ----- notifications -----

// TODO: notify_listener() and send_notifications() are obsolete!

// NotifyListenerRequest
class NotifyListenerRequest : public Request {
public:
	NotifyListenerRequest() : Request(NOTIFY_LISTENER_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	int32		operation;
	mount_id	nsid;
	vnode_id	vnida;
	vnode_id	vnidb;
	vnode_id	vnidc;
	Address		name;
};

// NotifyListenerReply
class NotifyListenerReply : public ReplyRequest {
public:
	NotifyListenerReply() : ReplyRequest(NOTIFY_LISTENER_REPLY) {}
};

// NotifySelectRequest
class NotifySelectEventRequest : public Request {
public:
	NotifySelectEventRequest() : Request(NOTIFY_SELECT_EVENT_REQUEST) {}

	selectsync*	sync;
	uint32		ref;
	uint8		event;
};

// NotifySelectEventReply
class NotifySelectEventReply : public ReplyRequest {
public:
	NotifySelectEventReply() : ReplyRequest(NOTIFY_SELECT_EVENT_REPLY) {}
};

// SendNotificationRequest
class SendNotificationRequest : public Request {
public:
	SendNotificationRequest() : Request(SEND_NOTIFICATION_REQUEST) {}
	status_t GetAddressInfos(AddressInfo* infos, int32* count);

	port_id		port;
	int32		token;
	uint32		what;
	int32		operation;
	mount_id	nsida;
	mount_id	nsidb;
	vnode_id	vnida;
	vnode_id	vnidb;
	vnode_id	vnidc;
	Address		name;
};

// SendNotificationReply
class SendNotificationReply : public ReplyRequest {
public:
	SendNotificationReply() : ReplyRequest(SEND_NOTIFICATION_REPLY) {}
};


// #pragma mark -
// #pragma mark ----- vnodes -----

// GetVNodeRequest
class GetVNodeRequest : public Request {
public:
	GetVNodeRequest() : Request(GET_VNODE_REQUEST) {}

	mount_id	nsid;
	vnode_id	vnid;
};

// GetVNodeReply
class GetVNodeReply : public ReplyRequest {
public:
	GetVNodeReply() : ReplyRequest(GET_VNODE_REPLY) {}

	fs_vnode	node;
};

// PutVNodeRequest
class PutVNodeRequest : public Request {
public:
	PutVNodeRequest() : Request(PUT_VNODE_REQUEST) {}

	mount_id	nsid;
	vnode_id	vnid;
};

// PutVNodeReply
class PutVNodeReply : public ReplyRequest {
public:
	PutVNodeReply() : ReplyRequest(PUT_VNODE_REPLY) {}
};

// NewVNodeRequest
class NewVNodeRequest : public Request {
public:
	NewVNodeRequest() : Request(NEW_VNODE_REQUEST) {}

	mount_id	nsid;
	vnode_id	vnid;
	fs_vnode	node;
};

// NewVNodeReply
class NewVNodeReply : public ReplyRequest {
public:
	NewVNodeReply() : ReplyRequest(NEW_VNODE_REPLY) {}
};

// RemoveVNodeRequest
class RemoveVNodeRequest : public Request {
public:
	RemoveVNodeRequest() : Request(REMOVE_VNODE_REQUEST) {}

	mount_id	nsid;
	vnode_id	vnid;
};

// RemoveVNodeReply
class RemoveVNodeReply : public ReplyRequest {
public:
	RemoveVNodeReply() : ReplyRequest(REMOVE_VNODE_REPLY) {}
};

// UnremoveVNodeRequest
class UnremoveVNodeRequest : public Request {
public:
	UnremoveVNodeRequest() : Request(UNREMOVE_VNODE_REQUEST) {}

	mount_id	nsid;
	vnode_id	vnid;
};

// UnremoveVNodeReply
class UnremoveVNodeReply : public ReplyRequest {
public:
	UnremoveVNodeReply() : ReplyRequest(UNREMOVE_VNODE_REPLY) {}
};

// IsVNodeRemovedRequest
class IsVNodeRemovedRequest : public Request {
public:
	IsVNodeRemovedRequest() : Request(IS_VNODE_REMOVED_REQUEST) {}

	mount_id	nsid;
	vnode_id	vnid;
};

// IsVNodeRemovedReply
class IsVNodeRemovedReply : public ReplyRequest {
public:
	IsVNodeRemovedReply() : ReplyRequest(IS_VNODE_REMOVED_REPLY) {}

	int			result;
};


//////////////////
// General Reply

// ReceiptAckReply
class ReceiptAckReply : public ReplyRequest {
public:
	ReceiptAckReply() : ReplyRequest(RECEIPT_ACK_REPLY) {}
};


///////////////////
// Request Checks

// do_for_request
template<class Task>
static
status_t
do_for_request(Request* request, Task& task)
{
	if (!request)
		return B_BAD_VALUE;
	switch (request->GetType()) {
		// kernel -> userland requests
		// administrative
		case UFS_DISCONNECT_REQUEST:
			return task((UFSDisconnectRequest*)request);
		case FS_CONNECT_REQUEST:
			return task((FSConnectRequest*)request);
		case FS_CONNECT_REPLY:
			return task((FSConnectReply*)request);
		// FS
		case MOUNT_VOLUME_REQUEST:
			return task((MountVolumeRequest*)request);
		case MOUNT_VOLUME_REPLY:
			return task((MountVolumeReply*)request);
		case UNMOUNT_VOLUME_REQUEST:
			return task((UnmountVolumeRequest*)request);
		case UNMOUNT_VOLUME_REPLY:
			return task((UnmountVolumeReply*)request);
//		case INITIALIZE_VOLUME_REQUEST:
//			return task((InitializeVolumeRequest*)request);
//		case INITIALIZE_VOLUME_REPLY:
//			return task((InitializeVolumeReply*)request);
		case SYNC_VOLUME_REQUEST:
			return task((SyncVolumeRequest*)request);
		case SYNC_VOLUME_REPLY:
			return task((SyncVolumeReply*)request);
		case READ_FS_INFO_REQUEST:
			return task((ReadFSInfoRequest*)request);
		case READ_FS_INFO_REPLY:
			return task((ReadFSInfoReply*)request);
		case WRITE_FS_INFO_REQUEST:
			return task((WriteFSInfoRequest*)request);
		case WRITE_FS_INFO_REPLY:
			return task((WriteFSInfoReply*)request);
		// vnodes
		case LOOKUP_REQUEST:
			return task((LookupRequest*)request);
		case LOOKUP_REPLY:
			return task((LookupReply*)request);
		case READ_VNODE_REQUEST:
			return task((ReadVNodeRequest*)request);
		case READ_VNODE_REPLY:
			return task((ReadVNodeReply*)request);
		case WRITE_VNODE_REQUEST:
			return task((WriteVNodeRequest*)request);
		case WRITE_VNODE_REPLY:
			return task((WriteVNodeReply*)request);
		case FS_REMOVE_VNODE_REQUEST:
			return task((FSRemoveVNodeRequest*)request);
		case FS_REMOVE_VNODE_REPLY:
			return task((FSRemoveVNodeReply*)request);
		// nodes
		case IOCTL_REQUEST:
			return task((IOCtlRequest*)request);
		case IOCTL_REPLY:
			return task((IOCtlReply*)request);
		case SET_FLAGS_REQUEST:
			return task((SetFlagsRequest*)request);
		case SET_FLAGS_REPLY:
			return task((SetFlagsReply*)request);
		case SELECT_REQUEST:
			return task((SelectRequest*)request);
		case SELECT_REPLY:
			return task((SelectReply*)request);
		case DESELECT_REQUEST:
			return task((DeselectRequest*)request);
		case DESELECT_REPLY:
			return task((DeselectReply*)request);
		case FSYNC_REQUEST:
			return task((FSyncRequest*)request);
		case FSYNC_REPLY:
			return task((FSyncReply*)request);
		case READ_SYMLINK_REQUEST:
			return task((ReadSymlinkRequest*)request);
		case READ_SYMLINK_REPLY:
			return task((ReadSymlinkReply*)request);
		case CREATE_SYMLINK_REQUEST:
			return task((CreateSymlinkRequest*)request);
		case CREATE_SYMLINK_REPLY:
			return task((CreateSymlinkReply*)request);
		case LINK_REQUEST:
			return task((LinkRequest*)request);
		case LINK_REPLY:
			return task((LinkReply*)request);
		case UNLINK_REQUEST:
			return task((UnlinkRequest*)request);
		case UNLINK_REPLY:
			return task((UnlinkReply*)request);
		case RENAME_REQUEST:
			return task((RenameRequest*)request);
		case RENAME_REPLY:
			return task((RenameReply*)request);
		case ACCESS_REQUEST:
			return task((AccessRequest*)request);
		case ACCESS_REPLY:
			return task((AccessReply*)request);
		case READ_STAT_REQUEST:
			return task((ReadStatRequest*)request);
		case READ_STAT_REPLY:
			return task((ReadStatReply*)request);
		case WRITE_STAT_REQUEST:
			return task((WriteStatRequest*)request);
		case WRITE_STAT_REPLY:
			return task((WriteStatReply*)request);
		// files
		case CREATE_REQUEST:
			return task((CreateRequest*)request);
		case CREATE_REPLY:
			return task((CreateReply*)request);
		case OPEN_REQUEST:
			return task((OpenRequest*)request);
		case OPEN_REPLY:
			return task((OpenReply*)request);
		case CLOSE_REQUEST:
			return task((CloseRequest*)request);
		case CLOSE_REPLY:
			return task((CloseReply*)request);
		case FREE_COOKIE_REQUEST:
			return task((FreeCookieRequest*)request);
		case FREE_COOKIE_REPLY:
			return task((FreeCookieReply*)request);
		case READ_REQUEST:
			return task((ReadRequest*)request);
		case READ_REPLY:
			return task((ReadReply*)request);
		case WRITE_REQUEST:
			return task((WriteRequest*)request);
		case WRITE_REPLY:
			return task((WriteReply*)request);
		// directories
		case CREATE_DIR_REQUEST:
			return task((CreateDirRequest*)request);
		case CREATE_DIR_REPLY:
			return task((CreateDirReply*)request);
		case REMOVE_DIR_REQUEST:
			return task((RemoveDirRequest*)request);
		case REMOVE_DIR_REPLY:
			return task((RemoveDirReply*)request);
		case OPEN_DIR_REQUEST:
			return task((OpenDirRequest*)request);
		case OPEN_DIR_REPLY:
			return task((OpenDirReply*)request);
		case CLOSE_DIR_REQUEST:
			return task((CloseDirRequest*)request);
		case CLOSE_DIR_REPLY:
			return task((CloseDirReply*)request);
		case FREE_DIR_COOKIE_REQUEST:
			return task((FreeDirCookieRequest*)request);
		case FREE_DIR_COOKIE_REPLY:
			return task((FreeDirCookieReply*)request);
		case READ_DIR_REQUEST:
			return task((ReadDirRequest*)request);
		case READ_DIR_REPLY:
			return task((ReadDirReply*)request);
		case REWIND_DIR_REQUEST:
			return task((RewindDirRequest*)request);
		case REWIND_DIR_REPLY:
			return task((RewindDirReply*)request);
		// attribute directories
		case OPEN_ATTR_DIR_REQUEST:
			return task((OpenAttrDirRequest*)request);
		case OPEN_ATTR_DIR_REPLY:
			return task((OpenAttrDirReply*)request);
		case CLOSE_ATTR_DIR_REQUEST:
			return task((CloseAttrDirRequest*)request);
		case CLOSE_ATTR_DIR_REPLY:
			return task((CloseAttrDirReply*)request);
		case FREE_ATTR_DIR_COOKIE_REQUEST:
			return task((FreeAttrDirCookieRequest*)request);
		case FREE_ATTR_DIR_COOKIE_REPLY:
			return task((FreeAttrDirCookieReply*)request);
		case READ_ATTR_DIR_REQUEST:
			return task((ReadAttrDirRequest*)request);
		case READ_ATTR_DIR_REPLY:
			return task((ReadAttrDirReply*)request);
		case REWIND_ATTR_DIR_REQUEST:
			return task((RewindAttrDirRequest*)request);
		case REWIND_ATTR_DIR_REPLY:
			return task((RewindAttrDirReply*)request);
		// attributes
		case READ_ATTR_REQUEST:
			return task((ReadAttrRequest*)request);
		case READ_ATTR_REPLY:
			return task((ReadAttrReply*)request);
		case WRITE_ATTR_REQUEST:
			return task((WriteAttrRequest*)request);
		case WRITE_ATTR_REPLY:
			return task((WriteAttrReply*)request);
		case READ_ATTR_STAT_REQUEST:
			return task((ReadAttrStatRequest*)request);
		case READ_ATTR_STAT_REPLY:
			return task((ReadAttrStatReply*)request);
		case RENAME_ATTR_REQUEST:
			return task((RenameAttrRequest*)request);
		case RENAME_ATTR_REPLY:
			return task((RenameAttrReply*)request);
		case REMOVE_ATTR_REQUEST:
			return task((RemoveAttrRequest*)request);
		case REMOVE_ATTR_REPLY:
			return task((RemoveAttrReply*)request);
		// indices
		case OPEN_INDEX_DIR_REQUEST:
			return task((OpenIndexDirRequest*)request);
		case OPEN_INDEX_DIR_REPLY:
			return task((OpenIndexDirReply*)request);
		case CLOSE_INDEX_DIR_REQUEST:
			return task((CloseIndexDirRequest*)request);
		case CLOSE_INDEX_DIR_REPLY:
			return task((CloseIndexDirReply*)request);
		case FREE_INDEX_DIR_COOKIE_REQUEST:
			return task((FreeIndexDirCookieRequest*)request);
		case FREE_INDEX_DIR_COOKIE_REPLY:
			return task((FreeIndexDirCookieReply*)request);
		case READ_INDEX_DIR_REQUEST:
			return task((ReadIndexDirRequest*)request);
		case READ_INDEX_DIR_REPLY:
			return task((ReadIndexDirReply*)request);
		case REWIND_INDEX_DIR_REQUEST:
			return task((RewindIndexDirRequest*)request);
		case REWIND_INDEX_DIR_REPLY:
			return task((RewindIndexDirReply*)request);
		case CREATE_INDEX_REQUEST:
			return task((CreateIndexRequest*)request);
		case CREATE_INDEX_REPLY:
			return task((CreateIndexReply*)request);
		case REMOVE_INDEX_REQUEST:
			return task((RemoveIndexRequest*)request);
		case REMOVE_INDEX_REPLY:
			return task((RemoveIndexReply*)request);
		case READ_INDEX_STAT_REQUEST:
			return task((ReadIndexStatRequest*)request);
		case READ_INDEX_STAT_REPLY:
			return task((ReadIndexStatReply*)request);
		// queries
		case OPEN_QUERY_REQUEST:
			return task((OpenQueryRequest*)request);
		case OPEN_QUERY_REPLY:
			return task((OpenQueryReply*)request);
		case CLOSE_QUERY_REQUEST:
			return task((CloseQueryRequest*)request);
		case CLOSE_QUERY_REPLY:
			return task((CloseQueryReply*)request);
		case FREE_QUERY_COOKIE_REQUEST:
			return task((FreeQueryCookieRequest*)request);
		case FREE_QUERY_COOKIE_REPLY:
			return task((FreeQueryCookieReply*)request);
		case READ_QUERY_REQUEST:
			return task((ReadQueryRequest*)request);
		case READ_QUERY_REPLY:
			return task((ReadQueryReply*)request);

		// userland -> kernel requests
		// notifications
		case NOTIFY_LISTENER_REQUEST:
			return task((NotifyListenerRequest*)request);
		case NOTIFY_LISTENER_REPLY:
			return task((NotifyListenerReply*)request);
		case NOTIFY_SELECT_EVENT_REQUEST:
			return task((NotifySelectEventRequest*)request);
		case NOTIFY_SELECT_EVENT_REPLY:
			return task((NotifySelectEventReply*)request);
		case SEND_NOTIFICATION_REQUEST:
			return task((SendNotificationRequest*)request);
		case SEND_NOTIFICATION_REPLY:
			return task((SendNotificationReply*)request);
		// vnodes
		case GET_VNODE_REQUEST:
			return task((GetVNodeRequest*)request);
		case GET_VNODE_REPLY:
			return task((GetVNodeReply*)request);
		case PUT_VNODE_REQUEST:
			return task((PutVNodeRequest*)request);
		case PUT_VNODE_REPLY:
			return task((PutVNodeReply*)request);
		case NEW_VNODE_REQUEST:
			return task((NewVNodeRequest*)request);
		case NEW_VNODE_REPLY:
			return task((NewVNodeReply*)request);
		case REMOVE_VNODE_REQUEST:
			return task((RemoveVNodeRequest*)request);
		case REMOVE_VNODE_REPLY:
			return task((RemoveVNodeReply*)request);
		case UNREMOVE_VNODE_REQUEST:
			return task((UnremoveVNodeRequest*)request);
		case UNREMOVE_VNODE_REPLY:
			return task((UnremoveVNodeReply*)request);
		case IS_VNODE_REMOVED_REQUEST:
			return task((IsVNodeRemovedRequest*)request);
		case IS_VNODE_REMOVED_REPLY:
			return task((IsVNodeRemovedReply*)request);
		// general reply
		case RECEIPT_ACK_REPLY:
			return task((ReceiptAckReply*)request);
		default:
			return B_BAD_DATA;
	}
	return task(request);
}

status_t get_request_address_infos(Request* request, AddressInfo* infos,
	int32* count);
status_t check_request(Request* request);
status_t relocate_request(Request* request, int32 requestBufferSize,
	area_id* areas, int32* count);

}	// namespace UserlandFSUtil

using UserlandFSUtil::ReplyRequest;
using UserlandFSUtil::VolumeRequest;
using UserlandFSUtil::NodeRequest;
using UserlandFSUtil::FileRequest;
using UserlandFSUtil::DirRequest;
using UserlandFSUtil::AttrDirRequest;
using UserlandFSUtil::IndexDirRequest;

// kernel -> userland requests
// administrative
using UserlandFSUtil::UFSDisconnectRequest;
using UserlandFSUtil::FSConnectRequest;
using UserlandFSUtil::FSConnectReply;
// FS
using UserlandFSUtil::MountVolumeRequest;
using UserlandFSUtil::MountVolumeReply;
using UserlandFSUtil::UnmountVolumeRequest;
using UserlandFSUtil::UnmountVolumeReply;
//using UserlandFSUtil::InitializeVolumeRequest;
//using UserlandFSUtil::InitializeVolumeReply;
using UserlandFSUtil::SyncVolumeRequest;
using UserlandFSUtil::SyncVolumeReply;
using UserlandFSUtil::ReadFSInfoRequest;
using UserlandFSUtil::ReadFSInfoReply;
using UserlandFSUtil::WriteFSInfoRequest;
using UserlandFSUtil::WriteFSInfoReply;
// vnodes
using UserlandFSUtil::LookupRequest;
using UserlandFSUtil::LookupReply;
using UserlandFSUtil::ReadVNodeRequest;
using UserlandFSUtil::ReadVNodeReply;
using UserlandFSUtil::WriteVNodeRequest;
using UserlandFSUtil::WriteVNodeReply;
using UserlandFSUtil::FSRemoveVNodeRequest;
using UserlandFSUtil::FSRemoveVNodeReply;
// nodes
using UserlandFSUtil::IOCtlRequest;
using UserlandFSUtil::IOCtlReply;
using UserlandFSUtil::SetFlagsRequest;
using UserlandFSUtil::SetFlagsReply;
using UserlandFSUtil::SelectRequest;
using UserlandFSUtil::SelectReply;
using UserlandFSUtil::DeselectRequest;
using UserlandFSUtil::DeselectReply;
using UserlandFSUtil::FSyncRequest;
using UserlandFSUtil::FSyncReply;
using UserlandFSUtil::ReadSymlinkRequest;
using UserlandFSUtil::ReadSymlinkReply;
using UserlandFSUtil::CreateSymlinkRequest;
using UserlandFSUtil::CreateSymlinkReply;
using UserlandFSUtil::LinkRequest;
using UserlandFSUtil::LinkReply;
using UserlandFSUtil::UnlinkRequest;
using UserlandFSUtil::UnlinkReply;
using UserlandFSUtil::RenameRequest;
using UserlandFSUtil::RenameReply;
using UserlandFSUtil::AccessRequest;
using UserlandFSUtil::AccessReply;
using UserlandFSUtil::ReadStatRequest;
using UserlandFSUtil::ReadStatReply;
using UserlandFSUtil::WriteStatRequest;
using UserlandFSUtil::WriteStatReply;
// files
using UserlandFSUtil::CreateRequest;
using UserlandFSUtil::CreateReply;
using UserlandFSUtil::OpenRequest;
using UserlandFSUtil::OpenReply;
using UserlandFSUtil::CloseRequest;
using UserlandFSUtil::CloseReply;
using UserlandFSUtil::FreeCookieRequest;
using UserlandFSUtil::FreeCookieReply;
using UserlandFSUtil::ReadRequest;
using UserlandFSUtil::ReadReply;
using UserlandFSUtil::WriteRequest;
using UserlandFSUtil::WriteReply;
// directories
using UserlandFSUtil::CreateDirRequest;
using UserlandFSUtil::CreateDirReply;
using UserlandFSUtil::RemoveDirRequest;
using UserlandFSUtil::RemoveDirReply;
using UserlandFSUtil::OpenDirRequest;
using UserlandFSUtil::OpenDirReply;
using UserlandFSUtil::CloseDirRequest;
using UserlandFSUtil::CloseDirReply;
using UserlandFSUtil::FreeDirCookieRequest;
using UserlandFSUtil::FreeDirCookieReply;
using UserlandFSUtil::ReadDirRequest;
using UserlandFSUtil::ReadDirReply;
using UserlandFSUtil::RewindDirRequest;
using UserlandFSUtil::RewindDirReply;
// attribute directories
using UserlandFSUtil::OpenAttrDirRequest;
using UserlandFSUtil::OpenAttrDirReply;
using UserlandFSUtil::CloseAttrDirRequest;
using UserlandFSUtil::CloseAttrDirReply;
using UserlandFSUtil::FreeAttrDirCookieRequest;
using UserlandFSUtil::FreeAttrDirCookieReply;
using UserlandFSUtil::ReadAttrDirRequest;
using UserlandFSUtil::ReadAttrDirReply;
using UserlandFSUtil::RewindAttrDirRequest;
using UserlandFSUtil::RewindAttrDirReply;
// attributes
using UserlandFSUtil::ReadAttrRequest;
using UserlandFSUtil::ReadAttrReply;
using UserlandFSUtil::WriteAttrRequest;
using UserlandFSUtil::WriteAttrReply;
using UserlandFSUtil::ReadAttrStatRequest;
using UserlandFSUtil::ReadAttrStatReply;
using UserlandFSUtil::RenameAttrRequest;
using UserlandFSUtil::RenameAttrReply;
using UserlandFSUtil::RemoveAttrRequest;
using UserlandFSUtil::RemoveAttrReply;
// indices
using UserlandFSUtil::OpenIndexDirRequest;
using UserlandFSUtil::OpenIndexDirReply;
using UserlandFSUtil::CloseIndexDirRequest;
using UserlandFSUtil::CloseIndexDirReply;
using UserlandFSUtil::FreeIndexDirCookieRequest;
using UserlandFSUtil::FreeIndexDirCookieReply;
using UserlandFSUtil::ReadIndexDirRequest;
using UserlandFSUtil::ReadIndexDirReply;
using UserlandFSUtil::RewindIndexDirRequest;
using UserlandFSUtil::RewindIndexDirReply;
using UserlandFSUtil::CreateIndexRequest;
using UserlandFSUtil::CreateIndexReply;
using UserlandFSUtil::RemoveIndexRequest;
using UserlandFSUtil::RemoveIndexReply;
using UserlandFSUtil::ReadIndexStatRequest;
using UserlandFSUtil::ReadIndexStatReply;
// queries
using UserlandFSUtil::OpenQueryRequest;
using UserlandFSUtil::OpenQueryReply;
using UserlandFSUtil::CloseQueryRequest;
using UserlandFSUtil::CloseQueryReply;
using UserlandFSUtil::FreeQueryCookieRequest;
using UserlandFSUtil::FreeQueryCookieReply;
using UserlandFSUtil::ReadQueryRequest;
using UserlandFSUtil::ReadQueryReply;

// userland -> kernel requests
// notifications
using UserlandFSUtil::NotifyListenerRequest;
using UserlandFSUtil::NotifyListenerReply;
using UserlandFSUtil::NotifySelectEventRequest;
using UserlandFSUtil::NotifySelectEventReply;
using UserlandFSUtil::SendNotificationRequest;
using UserlandFSUtil::SendNotificationReply;
// vnodes
using UserlandFSUtil::GetVNodeRequest;
using UserlandFSUtil::GetVNodeReply;
using UserlandFSUtil::PutVNodeRequest;
using UserlandFSUtil::PutVNodeReply;
using UserlandFSUtil::NewVNodeRequest;
using UserlandFSUtil::NewVNodeReply;
using UserlandFSUtil::RemoveVNodeRequest;
using UserlandFSUtil::RemoveVNodeReply;
using UserlandFSUtil::UnremoveVNodeRequest;
using UserlandFSUtil::UnremoveVNodeReply;
using UserlandFSUtil::IsVNodeRemovedRequest;
using UserlandFSUtil::IsVNodeRemovedReply;
// general reply
using UserlandFSUtil::ReceiptAckReply;

using UserlandFSUtil::do_for_request;
using UserlandFSUtil::get_request_address_infos;
using UserlandFSUtil::check_request;
using UserlandFSUtil::relocate_request;

#endif	// USERLAND_FS_REQUESTS_H
