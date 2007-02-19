// Requests.cpp

#include "Requests.h"

// constructor
RequestVisitor::RequestVisitor()
{
}

// destructor
RequestVisitor::~RequestVisitor()
{
}

// VisitConnectionBrokenRequest
status_t
RequestVisitor::VisitConnectionBrokenRequest(ConnectionBrokenRequest* request)
{
	return VisitAny(request);
}

// VisitInitConnectionRequest
status_t
RequestVisitor::VisitInitConnectionRequest(InitConnectionRequest* request)
{
	return VisitAny(request);
}

// VisitInitConnectionReply
status_t
RequestVisitor::VisitInitConnectionReply(InitConnectionReply* request)
{
	return VisitAny(request);
}

// VisitMountRequest
status_t
RequestVisitor::VisitMountRequest(MountRequest* request)
{
	return VisitAny(request);
}

// VisitMountReply
status_t
RequestVisitor::VisitMountReply(MountReply* request)
{
	return VisitAny(request);
}

// VisitUnmountRequest
status_t
RequestVisitor::VisitUnmountRequest(UnmountRequest* request)
{
	return VisitAny(request);
}

// VisitReadVNodeRequest
status_t
RequestVisitor::VisitReadVNodeRequest(ReadVNodeRequest* request)
{
	return VisitAny(request);
}

// VisitReadVNodeReply
status_t
RequestVisitor::VisitReadVNodeReply(ReadVNodeReply* request)
{
	return VisitAny(request);
}

// VisitWriteStatRequest
status_t
RequestVisitor::VisitWriteStatRequest(WriteStatRequest* request)
{
	return VisitAny(request);
}

// VisitWriteStatReply
status_t
RequestVisitor::VisitWriteStatReply(WriteStatReply* request)
{
	return VisitAny(request);
}

// VisitCreateFileRequest
status_t
RequestVisitor::VisitCreateFileRequest(CreateFileRequest* request)
{
	return VisitAny(request);
}

// VisitCreateFileReply
status_t
RequestVisitor::VisitCreateFileReply(CreateFileReply* request)
{
	return VisitAny(request);
}

// VisitOpenRequest
status_t
RequestVisitor::VisitOpenRequest(OpenRequest* request)
{
	return VisitAny(request);
}

// VisitOpenReply
status_t
RequestVisitor::VisitOpenReply(OpenReply* request)
{
	return VisitAny(request);
}

// VisitCloseRequest
status_t
RequestVisitor::VisitCloseRequest(CloseRequest* request)
{
	return VisitAny(request);
}

// VisitCloseReply
status_t
RequestVisitor::VisitCloseReply(CloseReply* request)
{
	return VisitAny(request);
}

// VisitReadRequest
status_t
RequestVisitor::VisitReadRequest(ReadRequest* request)
{
	return VisitAny(request);
}

// VisitReadReply
status_t
RequestVisitor::VisitReadReply(ReadReply* request)
{
	return VisitAny(request);
}

// VisitWriteRequest
status_t
RequestVisitor::VisitWriteRequest(WriteRequest* request)
{
	return VisitAny(request);
}

// VisitWriteReply
status_t
RequestVisitor::VisitWriteReply(WriteReply* request)
{
	return VisitAny(request);
}

// VisitCreateLinkRequest
status_t
RequestVisitor::VisitCreateLinkRequest(CreateLinkRequest* request)
{
	return VisitAny(request);
}

// VisitCreateLinkReply
status_t
RequestVisitor::VisitCreateLinkReply(CreateLinkReply* request)
{
	return VisitAny(request);
}

// VisitUnlinkRequest
status_t
RequestVisitor::VisitUnlinkRequest(UnlinkRequest* request)
{
	return VisitAny(request);
}

// VisitUnlinkReply
status_t
RequestVisitor::VisitUnlinkReply(UnlinkReply* request)
{
	return VisitAny(request);
}

// VisitCreateSymlinkRequest
status_t
RequestVisitor::VisitCreateSymlinkRequest(CreateSymlinkRequest* request)
{
	return VisitAny(request);
}

// VisitCreateSymlinkReply
status_t
RequestVisitor::VisitCreateSymlinkReply(CreateSymlinkReply* request)
{
	return VisitAny(request);
}

// VisitReadLinkRequest
status_t
RequestVisitor::VisitReadLinkRequest(ReadLinkRequest* request)
{
	return VisitAny(request);
}

// VisitReadLinkReply
status_t
RequestVisitor::VisitReadLinkReply(ReadLinkReply* request)
{
	return VisitAny(request);
}

// VisitRenameRequest
status_t
RequestVisitor::VisitRenameRequest(RenameRequest* request)
{
	return VisitAny(request);
}

// VisitRenameReply
status_t
RequestVisitor::VisitRenameReply(RenameReply* request)
{
	return VisitAny(request);
}

// VisitMakeDirRequest
status_t
RequestVisitor::VisitMakeDirRequest(MakeDirRequest* request)
{
	return VisitAny(request);
}

// VisitMakeDirReply
status_t
RequestVisitor::VisitMakeDirReply(MakeDirReply* request)
{
	return VisitAny(request);
}

// VisitRemoveDirRequest
status_t
RequestVisitor::VisitRemoveDirRequest(RemoveDirRequest* request)
{
	return VisitAny(request);
}

// VisitRemoveDirReply
status_t
RequestVisitor::VisitRemoveDirReply(RemoveDirReply* request)
{
	return VisitAny(request);
}

// VisitOpenDirRequest
status_t
RequestVisitor::VisitOpenDirRequest(OpenDirRequest* request)
{
	return VisitAny(request);
}

// VisitOpenDirReply
status_t
RequestVisitor::VisitOpenDirReply(OpenDirReply* request)
{
	return VisitAny(request);
}

// VisitReadDirRequest
status_t
RequestVisitor::VisitReadDirRequest(ReadDirRequest* request)
{
	return VisitAny(request);
}

// VisitReadDirReply
status_t
RequestVisitor::VisitReadDirReply(ReadDirReply* request)
{
	return VisitAny(request);
}

// VisitWalkRequest
status_t
RequestVisitor::VisitWalkRequest(WalkRequest* request)
{
	return VisitAny(request);
}

// VisitWalkReply
status_t
RequestVisitor::VisitWalkReply(WalkReply* request)
{
	return VisitAny(request);
}

// VisitMultiWalkRequest
status_t
RequestVisitor::VisitMultiWalkRequest(MultiWalkRequest* request)
{
	return VisitAny(request);
}

// VisitMultiWalkReply
status_t
RequestVisitor::VisitMultiWalkReply(MultiWalkReply* request)
{
	return VisitAny(request);
}

// VisitOpenAttrDirRequest
status_t
RequestVisitor::VisitOpenAttrDirRequest(OpenAttrDirRequest* request)
{
	return VisitAny(request);
}

// VisitOpenAttrDirReply
status_t
RequestVisitor::VisitOpenAttrDirReply(OpenAttrDirReply* request)
{
	return VisitAny(request);
}

// VisitReadAttrDirRequest
status_t
RequestVisitor::VisitReadAttrDirRequest(ReadAttrDirRequest* request)
{
	return VisitAny(request);
}

// VisitReadAttrDirReply
status_t
RequestVisitor::VisitReadAttrDirReply(ReadAttrDirReply* request)
{
	return VisitAny(request);
}

// VisitReadAttrRequest
status_t
RequestVisitor::VisitReadAttrRequest(ReadAttrRequest* request)
{
	return VisitAny(request);
}

// VisitReadAttrReply
status_t
RequestVisitor::VisitReadAttrReply(ReadAttrReply* request)
{
	return VisitAny(request);
}

// VisitWriteAttrRequest
status_t
RequestVisitor::VisitWriteAttrRequest(WriteAttrRequest* request)
{
	return VisitAny(request);
}

// VisitWriteAttrReply
status_t
RequestVisitor::VisitWriteAttrReply(WriteAttrReply* request)
{
	return VisitAny(request);
}

// VisitRemoveAttrRequest
status_t
RequestVisitor::VisitRemoveAttrRequest(RemoveAttrRequest* request)
{
	return VisitAny(request);
}

// VisitRemoveAttrReply
status_t
RequestVisitor::VisitRemoveAttrReply(RemoveAttrReply* request)
{
	return VisitAny(request);
}

// VisitRenameAttrRequest
status_t
RequestVisitor::VisitRenameAttrRequest(RenameAttrRequest* request)
{
	return VisitAny(request);
}

// VisitRenameAttrReply
status_t
RequestVisitor::VisitRenameAttrReply(RenameAttrReply* request)
{
	return VisitAny(request);
}

// VisitStatAttrRequest
status_t
RequestVisitor::VisitStatAttrRequest(StatAttrRequest* request)
{
	return VisitAny(request);
}

// VisitStatAttrReply
status_t
RequestVisitor::VisitStatAttrReply(StatAttrReply* request)
{
	return VisitAny(request);
}

// VisitOpenQueryRequest
status_t
RequestVisitor::VisitOpenQueryRequest(OpenQueryRequest* request)
{
	return VisitAny(request);
}

// VisitOpenQueryReply
status_t
RequestVisitor::VisitOpenQueryReply(OpenQueryReply* request)
{
	return VisitAny(request);
}

// VisitReadQueryRequest
status_t
RequestVisitor::VisitReadQueryRequest(ReadQueryRequest* request)
{
	return VisitAny(request);
}

// VisitReadQueryReply
status_t
RequestVisitor::VisitReadQueryReply(ReadQueryReply* request)
{
	return VisitAny(request);
}

// VisitNodeMonitoringRequest
status_t
RequestVisitor::VisitNodeMonitoringRequest(NodeMonitoringRequest* request)
{
	return VisitAny(request);
}

// VisitEntryCreatedRequest
status_t
RequestVisitor::VisitEntryCreatedRequest(EntryCreatedRequest* request)
{
	return VisitNodeMonitoringRequest(request);
}

// VisitEntryRemovedRequest
status_t
RequestVisitor::VisitEntryRemovedRequest(EntryRemovedRequest* request)
{
	return VisitNodeMonitoringRequest(request);
}

// VisitEntryMovedRequest
status_t
RequestVisitor::VisitEntryMovedRequest(EntryMovedRequest* request)
{
	return VisitNodeMonitoringRequest(request);
}

// VisitStatChangedRequest
status_t
RequestVisitor::VisitStatChangedRequest(StatChangedRequest* request)
{
	return VisitNodeMonitoringRequest(request);
}

// VisitAttributeChangedRequest
status_t
RequestVisitor::VisitAttributeChangedRequest(AttributeChangedRequest* request)
{
	return VisitNodeMonitoringRequest(request);
}

// VisitServerInfoRequest
status_t
RequestVisitor::VisitServerInfoRequest(ServerInfoRequest* request)
{
	return VisitAny(request);
}

// VisitAny
status_t
RequestVisitor::VisitAny(Request* request)
{
	return B_OK;
}

