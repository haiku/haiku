// Requests.h

#ifndef NET_FS_REQUESTS_H
#define NET_FS_REQUESTS_H

#include "AttrDirInfo.h"
#include "Compatibility.h"
#include "DebugSupport.h"
#include "EntryInfo.h"
#include "NodeInfo.h"
#include "Request.h"
#include "RequestMemberArray.h"
#include "ServerInfo.h"
#include "ServerNodeID.h"

// request types
enum {
	CONNECTION_BROKEN_REQUEST	= -1,
	INIT_CONNECTION_REQUEST		= 0,
	INIT_CONNECTION_REPLY,

	MOUNT_REQUEST,
	MOUNT_REPLY,
	UNMOUNT_REQUEST,
	READ_VNODE_REQUEST,
	READ_VNODE_REPLY,
	WRITE_STAT_REQUEST,
	WRITE_STAT_REPLY,
	CREATE_FILE_REQUEST,
	CREATE_FILE_REPLY,
	OPEN_REQUEST,
	OPEN_REPLY,
	CLOSE_REQUEST,	// common for all kinds of cookies
	CLOSE_REPLY,
	READ_REQUEST,
	READ_REPLY,
	WRITE_REQUEST,
	WRITE_REPLY,
	CREATE_LINK_REQUEST,
	CREATE_LINK_REPLY,
	UNLINK_REQUEST,
	UNLINK_REPLY,
	CREATE_SYMLINK_REQUEST,
	CREATE_SYMLINK_REPLY,
	READ_LINK_REQUEST,
	READ_LINK_REPLY,
	RENAME_REQUEST,
	RENAME_REPLY,
	MAKE_DIR_REQUEST,
	MAKE_DIR_REPLY,
	REMOVE_DIR_REQUEST,
	REMOVE_DIR_REPLY,
	OPEN_DIR_REQUEST,
	OPEN_DIR_REPLY,
	READ_DIR_REQUEST,
	READ_DIR_REPLY,
	WALK_REQUEST,
	WALK_REPLY,
	MULTI_WALK_REQUEST,
	MULTI_WALK_REPLY,

	OPEN_ATTR_DIR_REQUEST,
	OPEN_ATTR_DIR_REPLY,
	READ_ATTR_DIR_REQUEST,
	READ_ATTR_DIR_REPLY,
	READ_ATTR_REQUEST,
	READ_ATTR_REPLY,
	WRITE_ATTR_REQUEST,
	WRITE_ATTR_REPLY,
	REMOVE_ATTR_REQUEST,
	REMOVE_ATTR_REPLY,
	RENAME_ATTR_REQUEST,
	RENAME_ATTR_REPLY,
	STAT_ATTR_REQUEST,
	STAT_ATTR_REPLY,

	OPEN_QUERY_REQUEST,
	OPEN_QUERY_REPLY,
	READ_QUERY_REQUEST,
	READ_QUERY_REPLY,

	// node monitoring notifications
	ENTRY_CREATED_REQUEST,
	ENTRY_REMOVED_REQUEST,
	ENTRY_MOVED_REQUEST,
	ENTRY_STAT_CHANGED_REQUEST,
	ENTRY_ATTRIBUTE_CHANGED_REQUEST,

	// server info
	SERVER_INFO_REQUEST,
};

struct ConnectionBrokenRequest;
struct InitConnectionRequest;
struct InitConnectionReply;

struct MountRequest;
struct MountReply;
struct UnmountRequest;
struct ReadVNodeRequest;
struct ReadVNodeReply;
struct WriteStatRequest;
struct WriteStatReply;
struct CreateFileRequest;
struct CreateFileReply;
struct OpenRequest;
struct OpenReply;
struct CloseRequest;
struct CloseReply;
struct ReadRequest;
struct ReadReply;
struct WriteRequest;
struct WriteReply;
struct CreateLinkRequest;
struct CreateLinkReply;
struct UnlinkRequest;
struct UnlinkReply;
struct CreateSymlinkRequest;
struct CreateSymlinkReply;
struct ReadLinkRequest;
struct ReadLinkReply;
struct RenameRequest;
struct RenameReply;
struct RemoveDirRequest;
struct RemoveDirReply;
struct MakeDirRequest;
struct MakeDirReply;
struct OpenDirRequest;
struct OpenDirReply;
struct ReadDirRequest;
struct ReadDirReply;
struct WalkRequest;
struct WalkReply;
struct MultiWalkRequest;
struct MultiWalkReply;

struct OpenAttrDirRequest;
struct OpenAttrDirReply;
struct ReadAttrDirRequest;
struct ReadAttrDirReply;
struct ReadAttrRequest;
struct ReadAttrReply;
struct WriteAttrRequest;
struct WriteAttrReply;
struct RemoveAttrRequest;
struct RemoveAttrReply;
struct RenameAttrRequest;
struct RenameAttrReply;
struct StatAttrRequest;
struct StatAttrReply;

struct OpenQueryRequest;
struct OpenQueryReply;
struct ReadQueryRequest;
struct ReadQueryReply;

struct NodeMonitoringRequest;
struct EntryCreatedRequest;
struct EntryRemovedRequest;
struct EntryMovedRequest;
struct StatChangedRequest;
struct AttributeChangedRequest;

struct ServerInfoRequest;

// RequestVisitor
class RequestVisitor {
public:
								RequestVisitor();
	virtual						~RequestVisitor();

	virtual	status_t			VisitConnectionBrokenRequest(
									ConnectionBrokenRequest* request);
	virtual	status_t			VisitInitConnectionRequest(
									InitConnectionRequest* request);
	virtual	status_t			VisitInitConnectionReply(
									InitConnectionReply* request);

	virtual	status_t			VisitMountRequest(MountRequest* request);
	virtual	status_t			VisitMountReply(MountReply* request);
	virtual	status_t			VisitUnmountRequest(UnmountRequest* request);
	virtual	status_t			VisitReadVNodeRequest(
									ReadVNodeRequest* request);
	virtual	status_t			VisitReadVNodeReply(ReadVNodeReply* request);
	virtual	status_t			VisitWriteStatRequest(
									WriteStatRequest* request);
	virtual	status_t			VisitWriteStatReply(WriteStatReply* request);
	virtual	status_t			VisitCreateFileRequest(
									CreateFileRequest* request);
	virtual	status_t			VisitCreateFileReply(CreateFileReply* request);
	virtual	status_t			VisitOpenRequest(OpenRequest* request);
	virtual	status_t			VisitOpenReply(OpenReply* request);
	virtual	status_t			VisitCloseRequest(CloseRequest* request);
	virtual	status_t			VisitCloseReply(CloseReply* request);
	virtual	status_t			VisitReadRequest(ReadRequest* request);
	virtual	status_t			VisitReadReply(ReadReply* request);
	virtual	status_t			VisitWriteRequest(WriteRequest* request);
	virtual	status_t			VisitWriteReply(WriteReply* request);
	virtual	status_t			VisitCreateLinkRequest(
									CreateLinkRequest* request);
	virtual	status_t			VisitCreateLinkReply(CreateLinkReply* request);
	virtual	status_t			VisitUnlinkRequest(UnlinkRequest* request);
	virtual	status_t			VisitUnlinkReply(UnlinkReply* request);
	virtual	status_t			VisitCreateSymlinkRequest(
									CreateSymlinkRequest* request);
	virtual	status_t			VisitCreateSymlinkReply(
									CreateSymlinkReply* request);
	virtual	status_t			VisitReadLinkRequest(ReadLinkRequest* request);
	virtual	status_t			VisitReadLinkReply(ReadLinkReply* request);
	virtual	status_t			VisitRenameRequest(RenameRequest* request);
	virtual	status_t			VisitRenameReply(RenameReply* request);
	virtual	status_t			VisitMakeDirRequest(MakeDirRequest* request);
	virtual	status_t			VisitMakeDirReply(MakeDirReply* request);
	virtual	status_t			VisitRemoveDirRequest(
									RemoveDirRequest* request);
	virtual	status_t			VisitRemoveDirReply(RemoveDirReply* request);
	virtual	status_t			VisitOpenDirRequest(OpenDirRequest* request);
	virtual	status_t			VisitOpenDirReply(OpenDirReply* request);
	virtual	status_t			VisitReadDirRequest(ReadDirRequest* request);
	virtual	status_t			VisitReadDirReply(ReadDirReply* request);
	virtual	status_t			VisitWalkRequest(WalkRequest* request);
	virtual	status_t			VisitWalkReply(WalkReply* request);
	virtual	status_t			VisitMultiWalkRequest(
									MultiWalkRequest* request);
	virtual	status_t			VisitMultiWalkReply(MultiWalkReply* request);

	virtual	status_t			VisitOpenAttrDirRequest(
									OpenAttrDirRequest* request);
	virtual	status_t			VisitOpenAttrDirReply(
									OpenAttrDirReply* request);
	virtual	status_t			VisitReadAttrDirRequest(
									ReadAttrDirRequest* request);
	virtual	status_t			VisitReadAttrDirReply(
									ReadAttrDirReply* request);
	virtual	status_t			VisitReadAttrRequest(ReadAttrRequest* request);
	virtual	status_t			VisitReadAttrReply(ReadAttrReply* request);
	virtual	status_t			VisitWriteAttrRequest(
									WriteAttrRequest* request);
	virtual	status_t			VisitWriteAttrReply(WriteAttrReply* request);
	virtual	status_t			VisitRemoveAttrRequest(
									RemoveAttrRequest* request);
	virtual	status_t			VisitRemoveAttrReply(RemoveAttrReply* request);
	virtual	status_t			VisitRenameAttrRequest(
									RenameAttrRequest* request);
	virtual	status_t			VisitRenameAttrReply(RenameAttrReply* request);
	virtual	status_t			VisitStatAttrRequest(StatAttrRequest* request);
	virtual	status_t			VisitStatAttrReply(StatAttrReply* request);

	virtual	status_t			VisitOpenQueryRequest(
									OpenQueryRequest* request);
	virtual	status_t			VisitOpenQueryReply(OpenQueryReply* request);
	virtual	status_t			VisitReadQueryRequest(
									ReadQueryRequest* request);
	virtual	status_t			VisitReadQueryReply(ReadQueryReply* request);

	virtual	status_t			VisitNodeMonitoringRequest(
									NodeMonitoringRequest* request);
	virtual	status_t			VisitEntryCreatedRequest(
									EntryCreatedRequest* request);
	virtual	status_t			VisitEntryRemovedRequest(
									EntryRemovedRequest* request);
	virtual	status_t			VisitEntryMovedRequest(
									EntryMovedRequest* request);
	virtual	status_t			VisitStatChangedRequest(
									StatChangedRequest* request);
	virtual	status_t			VisitAttributeChangedRequest(
									AttributeChangedRequest* request);

	virtual	status_t			VisitServerInfoRequest(
									ServerInfoRequest* request);

	virtual	status_t			VisitAny(Request* request);
};

// ReplyRequest
struct ReplyRequest : Request {
	ReplyRequest(uint32 type) : Request(type) {}

	status_t		error;
};

// VolumeRequest
struct VolumeRequest : Request {
	VolumeRequest(uint32 type) : Request(type) {}

	int32			volumeID;
};

// ConnectionBrokenRequest
struct ConnectionBrokenRequest : Request {
	ConnectionBrokenRequest() : Request(CONNECTION_BROKEN_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitConnectionBrokenRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
	}

	status_t		error;
};

// InitConnectionRequest
struct InitConnectionRequest : Request {
	InitConnectionRequest() : Request(INIT_CONNECTION_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitInitConnectionRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, bigEndian);
	}

	bool			bigEndian;
};

// InitConnectionReply
struct InitConnectionReply : ReplyRequest {
	InitConnectionReply() : ReplyRequest(INIT_CONNECTION_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitInitConnectionReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
	}
};

// MountRequest
struct MountRequest : Request {
	MountRequest() : Request(MOUNT_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitMountRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, share);
		visitor->Visit(this, user);
		visitor->Visit(this, password);
		visitor->Visit(this, flags);
	}

	StringData		share;
	StringData		user;		// can be NULL, if the connection supports
	StringData		password;	// authentication; otherwise NULL means anonymous
	uint32			flags;
};

// MountReply
struct MountReply : ReplyRequest {
	MountReply() : ReplyRequest(MOUNT_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitMountReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
		visitor->Visit(this, volumeID);
		visitor->Visit(this, sharePermissions);
		visitor->Visit(this, nodeInfo);
		visitor->Visit(this, noPermission);
	}

	int32			volumeID;
	uint32			sharePermissions;
	NodeInfo		nodeInfo;
	bool			noPermission;	// always set (just as error)
};

// UnmountRequest
struct UnmountRequest : VolumeRequest {
	UnmountRequest() : VolumeRequest(UNMOUNT_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitUnmountRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
	}
};

// ReadVNodeRequest
struct ReadVNodeRequest : VolumeRequest {
	ReadVNodeRequest() : VolumeRequest(READ_VNODE_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitReadVNodeRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, nodeID);
	}

	ServerNodeID		nodeID;
};

// ReadVNodeReply
struct ReadVNodeReply : ReplyRequest {
	ReadVNodeReply() : ReplyRequest(READ_VNODE_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitReadVNodeReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
		visitor->Visit(this, nodeInfo);
	}

	NodeInfo		nodeInfo;
};

// WriteStatRequest
struct WriteStatRequest : VolumeRequest {
	WriteStatRequest() : VolumeRequest(WRITE_STAT_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitWriteStatRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, nodeID);
		visitor->Visit(this, nodeInfo);
		visitor->Visit(this, mask);
	}

	ServerNodeID	nodeID;
	NodeInfo		nodeInfo;
	uint32			mask;
};

// WriteStatReply
struct WriteStatReply : ReplyRequest {
	WriteStatReply() : ReplyRequest(WRITE_STAT_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitWriteStatReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
		visitor->Visit(this, nodeInfo);
		visitor->Visit(this, nodeInfoValid);
	}

	NodeInfo		nodeInfo;
	bool			nodeInfoValid;
};

// CreateFileRequest
struct CreateFileRequest : VolumeRequest {
	CreateFileRequest() : VolumeRequest(CREATE_FILE_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitCreateFileRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, directoryID);
		visitor->Visit(this, name);
		visitor->Visit(this, openMode);
		visitor->Visit(this, mode);
	}

	ServerNodeID	directoryID;
	StringData		name;
	int32			openMode;
	uint32			mode;
};

// CreateFileReply
struct CreateFileReply : ReplyRequest {
	CreateFileReply() : ReplyRequest(CREATE_FILE_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitCreateFileReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
		visitor->Visit(this, entryInfo);
		visitor->Visit(this, cookie);
	}

	EntryInfo		entryInfo;
	intptr_t		cookie;
};

// OpenRequest
struct OpenRequest : VolumeRequest {
	OpenRequest() : VolumeRequest(OPEN_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitOpenRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, nodeID);
		visitor->Visit(this, openMode);
	}

	ServerNodeID	nodeID;
	int32			openMode;
};

// OpenReply
struct OpenReply : ReplyRequest {
	OpenReply() : ReplyRequest(OPEN_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitOpenReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
		visitor->Visit(this, nodeInfo);
		visitor->Visit(this, cookie);
	}

	NodeInfo		nodeInfo;
	intptr_t		cookie;
};

// CloseRequest
struct CloseRequest : VolumeRequest {
	CloseRequest() : VolumeRequest(CLOSE_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitCloseRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, cookie);
	}

	intptr_t		cookie;
};

// CloseReply
struct CloseReply : ReplyRequest {
	CloseReply() : ReplyRequest(CLOSE_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitCloseReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
	}

};

// ReadRequest
struct ReadRequest : VolumeRequest {
	ReadRequest() : VolumeRequest(READ_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitReadRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, cookie);
		visitor->Visit(this, pos);
		visitor->Visit(this, size);
	}

	intptr_t		cookie;
	off_t			pos;
	int32			size;
};

// ReadReply
struct ReadReply : ReplyRequest {
	ReadReply() : ReplyRequest(READ_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitReadReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
		visitor->Visit(this, pos);
		visitor->Visit(this, data);
		visitor->Visit(this, moreToCome);
	}

	off_t			pos;
	Data			data;
	bool			moreToCome;
};

// WriteRequest
struct WriteRequest : VolumeRequest {
	WriteRequest() : VolumeRequest(WRITE_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitWriteRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, cookie);
		visitor->Visit(this, pos);
		visitor->Visit(this, data);
	}

	intptr_t		cookie;
	off_t			pos;
	Data			data;
};

// WriteReply
struct WriteReply : ReplyRequest {
	WriteReply() : ReplyRequest(WRITE_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitWriteReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
	}
};

// CreateLinkRequest
struct CreateLinkRequest : VolumeRequest {
	CreateLinkRequest() : VolumeRequest(CREATE_LINK_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitCreateLinkRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, directoryID);
		visitor->Visit(this, name);
		visitor->Visit(this, nodeID);
	}

	ServerNodeID	directoryID;
	StringData		name;
	ServerNodeID	nodeID;
};

// CreateLinkReply
struct CreateLinkReply : ReplyRequest {
	CreateLinkReply() : ReplyRequest(CREATE_LINK_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitCreateLinkReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
	}
};

// UnlinkRequest
struct UnlinkRequest : VolumeRequest {
	UnlinkRequest() : VolumeRequest(UNLINK_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitUnlinkRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, directoryID);
		visitor->Visit(this, name);
	}

	ServerNodeID	directoryID;
	StringData		name;
};

// UnlinkReply
struct UnlinkReply : ReplyRequest {
	UnlinkReply() : ReplyRequest(UNLINK_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitUnlinkReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
	}
};

// CreateSymlinkRequest
struct CreateSymlinkRequest : VolumeRequest {
	CreateSymlinkRequest() : VolumeRequest(CREATE_SYMLINK_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitCreateSymlinkRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, directoryID);
		visitor->Visit(this, name);
		visitor->Visit(this, target);
	}

	ServerNodeID	directoryID;
	StringData		name;
	StringData		target;
};

// CreateSymlinkReply
struct CreateSymlinkReply : ReplyRequest {
	CreateSymlinkReply() : ReplyRequest(CREATE_SYMLINK_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitCreateSymlinkReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
	}
};

// ReadLinkRequest
struct ReadLinkRequest : VolumeRequest {
	ReadLinkRequest() : VolumeRequest(READ_LINK_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitReadLinkRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, nodeID);
		visitor->Visit(this, maxSize);
	}

	ServerNodeID	nodeID;
	int32			maxSize;
};

// ReadLinkReply
struct ReadLinkReply : ReplyRequest {
	ReadLinkReply() : ReplyRequest(READ_LINK_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitReadLinkReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
		visitor->Visit(this, nodeInfo);
		visitor->Visit(this, data);
	}

	NodeInfo		nodeInfo;
	Data			data;
};

// RenameRequest
struct RenameRequest : VolumeRequest {
	RenameRequest() : VolumeRequest(RENAME_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitRenameRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, oldDirectoryID);
		visitor->Visit(this, oldName);
		visitor->Visit(this, newDirectoryID);
		visitor->Visit(this, newName);
	}

	ServerNodeID	oldDirectoryID;
	StringData		oldName;
	ServerNodeID	newDirectoryID;
	StringData		newName;
};

// RenameReply
struct RenameReply : ReplyRequest {
	RenameReply() : ReplyRequest(RENAME_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitRenameReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
	}
};

// MakeDirRequest
struct MakeDirRequest : VolumeRequest {
	MakeDirRequest() : VolumeRequest(MAKE_DIR_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitMakeDirRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, directoryID);
		visitor->Visit(this, name);
		visitor->Visit(this, mode);
	}

	ServerNodeID	directoryID;
	StringData		name;
	uint32			mode;
};

// MakeDirReply
struct MakeDirReply : ReplyRequest {
	MakeDirReply() : ReplyRequest(MAKE_DIR_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitMakeDirReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
	}
};

// RemoveDirRequest
struct RemoveDirRequest : VolumeRequest {
	RemoveDirRequest() : VolumeRequest(REMOVE_DIR_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitRemoveDirRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, directoryID);
		visitor->Visit(this, name);
	}

	ServerNodeID	directoryID;
	StringData		name;
};

// RemoveDirReply
struct RemoveDirReply : ReplyRequest {
	RemoveDirReply() : ReplyRequest(REMOVE_DIR_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitRemoveDirReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
	}
};

// OpenDirRequest
struct OpenDirRequest : VolumeRequest {
	OpenDirRequest() : VolumeRequest(OPEN_DIR_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitOpenDirRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, nodeID);
	}

	ServerNodeID	nodeID;
};

// OpenDirReply
struct OpenDirReply : ReplyRequest {
	OpenDirReply() : ReplyRequest(OPEN_DIR_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitOpenDirReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
		visitor->Visit(this, nodeInfo);
		visitor->Visit(this, cookie);
	}

	NodeInfo		nodeInfo;
	intptr_t		cookie;
};

// ReadDirRequest
struct ReadDirRequest : VolumeRequest {
	ReadDirRequest() : VolumeRequest(READ_DIR_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitReadDirRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, cookie);
		visitor->Visit(this, count);
		visitor->Visit(this, rewind);
	}

	intptr_t		cookie;
	int32			count;
	bool			rewind;
};

// ReadDirReply
struct ReadDirReply : ReplyRequest {
	ReadDirReply() : ReplyRequest(READ_DIR_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitReadDirReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
		visitor->Visit(this, entryInfos);
		visitor->Visit(this, revision);
		visitor->Visit(this, done);
	}

	RequestMemberArray<EntryInfo> entryInfos;
	int64			revision;
	bool			done;
};

// WalkRequest
struct WalkRequest : VolumeRequest {
	WalkRequest() : VolumeRequest(WALK_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitWalkRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, nodeID);
		visitor->Visit(this, name);
		visitor->Visit(this, resolveLink);
	}

	ServerNodeID	nodeID;
	StringData		name;
	bool			resolveLink;
};

// WalkReply
struct WalkReply : ReplyRequest {
	WalkReply() : ReplyRequest(WALK_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitWalkReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
		visitor->Visit(this, entryInfo);
		visitor->Visit(this, linkPath);
	}

	EntryInfo		entryInfo;
	StringData		linkPath;
};

// MulitWalkRequest
struct MultiWalkRequest : VolumeRequest {
	MultiWalkRequest() : VolumeRequest(MULTI_WALK_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitMultiWalkRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, nodeID);
		visitor->Visit(this, names);
	}

	ServerNodeID	nodeID;
	RequestMemberArray<StringData> names;
};

// MultiWalkReply
struct MultiWalkReply : ReplyRequest {
	MultiWalkReply() : ReplyRequest(MULTI_WALK_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitMultiWalkReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
		visitor->Visit(this, entryInfos);
	}

	RequestMemberArray<EntryInfo> entryInfos;
};

// OpenAttrDirRequest
struct OpenAttrDirRequest : VolumeRequest {
	OpenAttrDirRequest() : VolumeRequest(OPEN_ATTR_DIR_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitOpenAttrDirRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, nodeID);
	}

	ServerNodeID	nodeID;
};

// OpenAttrDirReply
struct OpenAttrDirReply : ReplyRequest {
	OpenAttrDirReply() : ReplyRequest(OPEN_ATTR_DIR_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitOpenAttrDirReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
		visitor->Visit(this, attrDirInfo);
		visitor->Visit(this, cookie);
	}

	AttrDirInfo		attrDirInfo;
	intptr_t		cookie;
};

// ReadAttrDirRequest
struct ReadAttrDirRequest : VolumeRequest {
	ReadAttrDirRequest() : VolumeRequest(READ_ATTR_DIR_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitReadAttrDirRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, cookie);
		visitor->Visit(this, rewind);
	}

	intptr_t		cookie;
	int32			count;
	bool			rewind;
};

// ReadAttrDirReply
struct ReadAttrDirReply : ReplyRequest {
	ReadAttrDirReply() : ReplyRequest(READ_ATTR_DIR_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitReadAttrDirReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
		visitor->Visit(this, name);
		visitor->Visit(this, count);
	}

	StringData		name;	// TODO: This should be a list.
	int32			count;
};

// ReadAttrRequest
struct ReadAttrRequest : VolumeRequest {
	ReadAttrRequest() : VolumeRequest(READ_ATTR_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitReadAttrRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, nodeID);
		visitor->Visit(this, name);
		visitor->Visit(this, type);
		visitor->Visit(this, pos);
		visitor->Visit(this, size);
	}

	ServerNodeID	nodeID;
	StringData		name;
	uint32			type;
	off_t			pos;
	int32			size;
};

// ReadAttrReply
struct ReadAttrReply : ReplyRequest {
	ReadAttrReply() : ReplyRequest(READ_ATTR_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitReadAttrReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
		visitor->Visit(this, pos);
		visitor->Visit(this, data);
		visitor->Visit(this, moreToCome);
	}

	off_t			pos;
	Data			data;
	bool			moreToCome;
};

// WriteAttrRequest
struct WriteAttrRequest : VolumeRequest {
	WriteAttrRequest() : VolumeRequest(WRITE_ATTR_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitWriteAttrRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, nodeID);
		visitor->Visit(this, name);
		visitor->Visit(this, type);
		visitor->Visit(this, pos);
		visitor->Visit(this, data);
	}

	ServerNodeID	nodeID;
	StringData		name;
	uint32			type;
	off_t			pos;
	Data			data;
};

// WriteAttrReply
struct WriteAttrReply : ReplyRequest {
	WriteAttrReply() : ReplyRequest(WRITE_ATTR_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitWriteAttrReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
	}
};

// RemoveAttrRequest
struct RemoveAttrRequest : VolumeRequest {
	RemoveAttrRequest() : VolumeRequest(REMOVE_ATTR_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitRemoveAttrRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, nodeID);
		visitor->Visit(this, name);
	}

	ServerNodeID	nodeID;
	StringData		name;
};

// RemoveAttrReply
struct RemoveAttrReply : ReplyRequest {
	RemoveAttrReply() : ReplyRequest(REMOVE_ATTR_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitRemoveAttrReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
	}
};

// RenameAttrRequest
struct RenameAttrRequest : VolumeRequest {
	RenameAttrRequest() : VolumeRequest(RENAME_ATTR_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitRenameAttrRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, nodeID);
		visitor->Visit(this, oldName);
		visitor->Visit(this, newName);
	}

	ServerNodeID	nodeID;
	StringData		oldName;
	StringData		newName;
};

// RenameAttrReply
struct RenameAttrReply : ReplyRequest {
	RenameAttrReply() : ReplyRequest(RENAME_ATTR_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitRenameAttrReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
	}
};

// StatAttrRequest
struct StatAttrRequest : VolumeRequest {
	StatAttrRequest() : VolumeRequest(STAT_ATTR_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitStatAttrRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, nodeID);
		visitor->Visit(this, name);
	}

	ServerNodeID	nodeID;
	StringData		name;
};

// StatAttrReply
struct StatAttrReply : ReplyRequest {
	StatAttrReply() : ReplyRequest(STAT_ATTR_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitStatAttrReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
		visitor->Visit(this, attrInfo);
	}

	AttributeInfo	attrInfo;
};

// OpenQueryRequest
struct OpenQueryRequest : Request {
	OpenQueryRequest() : Request(OPEN_QUERY_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitOpenQueryRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, queryString);
		visitor->Visit(this, flags);
		visitor->Visit(this, port);
		visitor->Visit(this, token);
	}

	StringData		queryString;
	uint32			flags;
	int32			port;
	int32			token;
};

// OpenQueryReply
struct OpenQueryReply : ReplyRequest {
	OpenQueryReply() : ReplyRequest(OPEN_QUERY_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitOpenQueryReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
		visitor->Visit(this, cookie);
	}

	intptr_t		cookie;
};

// ReadQueryRequest
struct ReadQueryRequest : Request {
	ReadQueryRequest() : Request(READ_QUERY_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitReadQueryRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, cookie);
	}

	intptr_t		cookie;
	int32			count;
};

// ReadQueryReply
struct ReadQueryReply : ReplyRequest {
	ReadQueryReply() : ReplyRequest(READ_QUERY_REPLY) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitReadQueryReply(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, error);
		visitor->Visit(this, clientVolumeIDs);
		visitor->Visit(this, dirInfo);
		visitor->Visit(this, entryInfo);
		visitor->Visit(this, count);
	}

	RequestMemberArray<int32>	clientVolumeIDs;
	NodeInfo					dirInfo;	// TODO: This should be a list.
	EntryInfo					entryInfo;	//
	int32						count;
};

// NodeMonitoringRequest
struct NodeMonitoringRequest : VolumeRequest {
	NodeMonitoringRequest(uint32 type) : VolumeRequest(type) {}

	int32			opcode;
	int64			revision;
	ServerNodeID	nodeID;
	bool			queryUpdate;	// true, if this is a query update
	int32			port;			// for query updates
	int32			token;			//
};

// EntryCreatedRequest
struct EntryCreatedRequest : NodeMonitoringRequest {
	EntryCreatedRequest() : NodeMonitoringRequest(ENTRY_CREATED_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitEntryCreatedRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, opcode);
		visitor->Visit(this, revision);
		visitor->Visit(this, nodeID);
		visitor->Visit(this, queryUpdate);
		visitor->Visit(this, port);
		visitor->Visit(this, token);
		visitor->Visit(this, directoryID);
		visitor->Visit(this, name);
		visitor->Visit(this, entryInfo);
		visitor->Visit(this, entryInfoValid);
	}

	ServerNodeID	directoryID;
	StringData		name;
	EntryInfo		entryInfo;
	bool			entryInfoValid;
};

// EntryRemovedRequest
struct EntryRemovedRequest : NodeMonitoringRequest {
	EntryRemovedRequest() : NodeMonitoringRequest(ENTRY_REMOVED_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitEntryRemovedRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, opcode);
		visitor->Visit(this, revision);
		visitor->Visit(this, nodeID);
		visitor->Visit(this, queryUpdate);
		visitor->Visit(this, port);
		visitor->Visit(this, token);
		visitor->Visit(this, directoryID);
		visitor->Visit(this, name);
	}

	ServerNodeID	directoryID;
	StringData		name;
};

// EntryMovedRequest
struct EntryMovedRequest : NodeMonitoringRequest {
	EntryMovedRequest() : NodeMonitoringRequest(ENTRY_MOVED_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitEntryMovedRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, opcode);
		visitor->Visit(this, revision);
		visitor->Visit(this, nodeID);
		visitor->Visit(this, queryUpdate);
		visitor->Visit(this, port);
		visitor->Visit(this, token);
		visitor->Visit(this, fromDirectoryID);
		visitor->Visit(this, toDirectoryID);
		visitor->Visit(this, fromName);
		visitor->Visit(this, toName);
		visitor->Visit(this, entryInfo);
		visitor->Visit(this, entryInfoValid);
	}

	ServerNodeID	fromDirectoryID;
	ServerNodeID	toDirectoryID;
	StringData		fromName;
	StringData		toName;
	EntryInfo		entryInfo;
	bool			entryInfoValid;
};

// StatChangedRequest
struct StatChangedRequest : NodeMonitoringRequest {
	StatChangedRequest() : NodeMonitoringRequest(ENTRY_STAT_CHANGED_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitStatChangedRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, opcode);
		visitor->Visit(this, revision);
		visitor->Visit(this, nodeID);
		visitor->Visit(this, queryUpdate);
		visitor->Visit(this, port);
		visitor->Visit(this, token);
		visitor->Visit(this, nodeInfo);
	}

	NodeInfo		nodeInfo;
};

// AttributeChangedRequest
struct AttributeChangedRequest : NodeMonitoringRequest {
	AttributeChangedRequest()
		: NodeMonitoringRequest(ENTRY_ATTRIBUTE_CHANGED_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitAttributeChangedRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, volumeID);
		visitor->Visit(this, opcode);
		visitor->Visit(this, revision);
		visitor->Visit(this, nodeID);
		visitor->Visit(this, queryUpdate);
		visitor->Visit(this, port);
		visitor->Visit(this, token);
		visitor->Visit(this, attrDirInfo);
		visitor->Visit(this, attrInfo);
		visitor->Visit(this, valid);
		visitor->Visit(this, removed);
	}

	AttrDirInfo		attrDirInfo;
	AttributeInfo	attrInfo;
	bool			valid;
	bool			removed;
};

// ServerInfoRequest
struct ServerInfoRequest : Request {
	ServerInfoRequest() : Request(SERVER_INFO_REQUEST) {}
	virtual status_t Accept(RequestVisitor* visitor)
		{ return visitor->VisitServerInfoRequest(this); }
	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, serverInfo);
	}

	ServerInfo		serverInfo;
};

#endif	// NET_FS_REQUESTS_H
