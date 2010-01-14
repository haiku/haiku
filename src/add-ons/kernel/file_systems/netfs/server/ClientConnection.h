// ClientConnection.h

#ifndef NET_FS_CLIENT_CONNECTION_H
#define NET_FS_CLIENT_CONNECTION_H

#include <OS.h>
#include <SupportDefs.h>

#include "ClientVolume.h"
#include "Locker.h"
#include "Permissions.h"
#include "QueryDomain.h"
#include "RequestHandler.h"

class AttrDirInfo;
class AttributeChangedEvent;
class AttributeDirectory;
class AttributeInfo;
class ClientConnectionListener;
class Connection;
class EntryCreatedEvent;
class EntryMovedEvent;
class EntryRemovedEvent;
class NodeHandleMap;
class RequestConnection;
class SecurityContext;
class StatChangedEvent;
class User;
class Share;
class VolumeManager;

// ClientConnection
class ClientConnection : private RequestHandler,
	private ClientVolume::NodeMonitoringProcessor,
	private QueryDomain {
public:
								ClientConnection(Connection* connection,
									SecurityContext* securityContext,
									User* user,
									ClientConnectionListener* listener);
								~ClientConnection();

			status_t			Init();

			void				Close();

			bool				GetReference();
			void				PutReference();

			void				UserRemoved(User* user);
			void				ShareRemoved(Share* share);
			void				UserPermissionsChanged(Share* share,
									User* user, Permissions permissions);

private:
			class ConnectionReference;
			struct VolumeMap;
			class ClientVolumePutter;
			friend class ClientVolumePutter;
			struct VolumeNodeMonitoringEvent;
			struct NodeMonitoringEventQueue;
			struct QueryHandleUnlocker;
			friend struct QueryHandleUnlocker;
			struct ClientVolumeFilter;
			struct HasQueryPermissionClientVolumeFilter;

	virtual	status_t			VisitConnectionBrokenRequest(
									ConnectionBrokenRequest* request);
	virtual	status_t			VisitInitConnectionRequest(
									InitConnectionRequest* request);
	virtual	status_t			VisitMountRequest(MountRequest* request);
	virtual	status_t			VisitUnmountRequest(UnmountRequest* request);
	virtual	status_t			VisitReadVNodeRequest(
									ReadVNodeRequest* request);
	virtual	status_t			VisitWriteStatRequest(
									WriteStatRequest* request);
	virtual	status_t			VisitCreateFileRequest(
									CreateFileRequest* request);
	virtual	status_t			VisitOpenRequest(OpenRequest* request);
	virtual	status_t			VisitCloseRequest(CloseRequest* request);
	virtual	status_t			VisitReadRequest(ReadRequest* request);
	virtual	status_t			VisitWriteRequest(WriteRequest* request);
	virtual	status_t			VisitCreateLinkRequest(
									CreateLinkRequest* request);
	virtual	status_t			VisitUnlinkRequest(UnlinkRequest* request);
	virtual	status_t			VisitCreateSymlinkRequest(
									CreateSymlinkRequest* request);
	virtual	status_t			VisitReadLinkRequest(ReadLinkRequest* request);
	virtual	status_t			VisitRenameRequest(RenameRequest* request);
	virtual	status_t			VisitMakeDirRequest(MakeDirRequest* request);
	virtual	status_t			VisitRemoveDirRequest(
									RemoveDirRequest* request);
	virtual	status_t			VisitOpenDirRequest(OpenDirRequest* request);
	virtual	status_t			VisitReadDirRequest(ReadDirRequest* request);
	virtual	status_t			VisitWalkRequest(WalkRequest* request);
	virtual	status_t			VisitMultiWalkRequest(
									MultiWalkRequest* request);
	virtual	status_t			VisitOpenAttrDirRequest(
									OpenAttrDirRequest* request);
	virtual	status_t			VisitReadAttrDirRequest(
									ReadAttrDirRequest* request);
	virtual	status_t			VisitReadAttrRequest(ReadAttrRequest* request);
	virtual	status_t			VisitWriteAttrRequest(
									WriteAttrRequest* request);
	virtual	status_t			VisitRemoveAttrRequest(
									RemoveAttrRequest* request);
	virtual	status_t			VisitRenameAttrRequest(
									RenameAttrRequest* request);
	virtual	status_t			VisitStatAttrRequest(StatAttrRequest* request);
	virtual	status_t			VisitOpenQueryRequest(
									OpenQueryRequest* request);
	virtual	status_t			VisitReadQueryRequest(
									ReadQueryRequest* request);

	// ClientVolume::NodeMonitoringProcessor
	virtual	void				ProcessNodeMonitoringEvent(int32 volumeID,
									NodeMonitoringEvent* event);
			void				CloseNodeMonitoringEventQueue();

	// QueryDomain
	virtual	bool				QueryDomainIntersectsWith(Volume* volume);
	virtual	void				ProcessQueryEvent(NodeMonitoringEvent* event);

			void				_Close();
			void				_MarkClosed(bool error);

			void				_GetNodeInfo(Node* node, NodeInfo* info);
			void				_GetEntryInfo(Entry* entry, EntryInfo* info);
			status_t			_GetAttrInfo(Request* request,
									const char* name, const attr_info& attrInfo,
									const void* data, AttributeInfo* info);
			status_t			_GetAttrDirInfo(Request* request,
									AttributeDirectory* attrDir,
									AttrDirInfo* info);

			status_t			_CreateVolume(ClientVolume** volume);
			ClientVolume*		_GetVolume(int32 id);
			void				_PutVolume(ClientVolume* volume);
			void				_UnmountVolume(ClientVolume* volume);
			void				_UnmountAllVolumes();

	static	int32				_NodeMonitoringProcessorEntry(void* data);
			int32				_NodeMonitoringProcessor();

			status_t			_PushNodeMonitoringEvent(int32 volumeID,
									NodeMonitoringEvent* event);

			status_t			_EntryCreated(ClientVolume* volume,
									EntryCreatedEvent* event,
									NodeMonitoringRequest*& request);
			status_t			_EntryRemoved(ClientVolume* volume,
									EntryRemovedEvent* event,
									NodeMonitoringRequest*& request);
			status_t			_EntryMoved(ClientVolume* volume,
									EntryMovedEvent* event,
									NodeMonitoringRequest*& request);
			status_t			_NodeStatChanged(ClientVolume* volume,
									StatChangedEvent* event,
									NodeMonitoringRequest*& request);
			status_t			_NodeAttributeChanged(ClientVolume* volume,
									AttributeChangedEvent* event,
									NodeMonitoringRequest*& request);

			bool				_KnownAttributeType(type_code type);
			void				_ConvertAttribute(const attr_info& info,
									void* buffer);

			status_t			_OpenQuery(const char* queryString,
									uint32 flags, port_id remotePort,
									int32 remoteToken, QueryHandle** handle);
			status_t			_CloseQuery(QueryHandle* handle);
			status_t			_LockQueryHandle(int32 cookie,
									QueryHandle** handle);
			void				_UnlockQueryHandle(NodeHandle* nodeHandle);

			int32				_GetAllClientVolumeIDs(int32* volumeIDs,
									int32 arraySize,
									ClientVolumeFilter* filter = NULL);
			int32				_GetContainingClientVolumes(
									Directory* directory, int32* volumeIDs,
									int32 arraySize,
									ClientVolumeFilter* filter = NULL);

private:
			RequestConnection*	fConnection;
			SecurityContext*	fSecurityContext;
			Locker				fSecurityContextLock;
			User*				fUser;
			VolumeMap*			fVolumes;
			NodeHandleMap*		fQueryHandles;
			ClientConnectionListener* fListener;
			NodeMonitoringEventQueue* fNodeMonitoringEvents;
			thread_id			fNodeMonitoringProcessor;
			Locker				fLock;
			int32				fReferenceCount;
			vint32				fInitialized;
			bool				fClosed;
			bool				fError;
			bool				fInverseClientEndianess;
};

// ClientConnectionListener
class ClientConnectionListener {
public:
								ClientConnectionListener();
	virtual						~ClientConnectionListener();

	virtual	void				ClientConnectionClosed(
									ClientConnection* connection,
									bool broken) = 0;
};

#endif	// NET_FS_CLIENT_CONNECTION_H
