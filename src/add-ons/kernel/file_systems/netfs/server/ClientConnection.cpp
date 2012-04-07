// ClientConnection.cpp

#include "ClientConnection.h"

#include <new>
#include <typeinfo>

#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <utime.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <Entry.h>
#include <fs_query.h>
#include <GraphicsDefs.h>
#include <HashMap.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Rect.h>
#include <Mime.h>

#include <fsproto.h>

#include "Compatibility.h"
#include "Connection.h"
#include "DebugSupport.h"
#include "Directory.h"
#include "Entry.h"
#include "FDManager.h"
#include "NodeHandle.h"
#include "NodeHandleMap.h"
#include "NodeMonitoringEvent.h"
#include "Path.h"
#include "RequestBufferReplacer.h"
#include "RequestChannel.h"
#include "RequestConnection.h"
#include "RequestDumper.h"
#include "RequestFlattener.h"
#include "Requests.h"
#include "SecurityContext.h"
#include "ServerNodeID.h"
#include "UserSecurityContext.h"
#include "Utils.h"
#include "Volume.h"
#include "VolumeManager.h"

static const int32 kMaxSaneReadLinkSize		= 10240;	// 10 KB
static const int32 kMaxReadBufferSize		= 10240;	// 10 KB
static const int32 kMaxReadDirBufferSize	= 10240;

// Locking:
//
// fLock: Guards fReferenceCount and fClosed.
// fSecurityContextLock: Guards fSecurityContext.
// fVolumes: Guards the map itself.


// #pragma mark -
// #pragma mark ----- ClientConnection -----

// ConnectionReference
class ClientConnection::ConnectionReference {
public:
	ConnectionReference(ClientConnection* connection)
		: fConnection(connection)
	{
		if (!fConnection || !fConnection->GetReference())
			fConnection = NULL;
	}

	~ConnectionReference()
	{
		if (fConnection)
			fConnection->PutReference();
	}

	bool IsValid() const
	{
		return fConnection;
	}

private:
	ClientConnection*	fConnection;
};

// VolumeMap
struct ClientConnection::VolumeMap
	: public SynchronizedHashMap<HashKey32<int32>, ClientVolume*> {
};

// ClientVolumePutter
class ClientConnection::ClientVolumePutter {
public:
	ClientVolumePutter(ClientConnection* connection, ClientVolume* volume)
		: fConnection(connection),
		  fVolume(volume)
	{
	}

	~ClientVolumePutter()
	{
		if (fConnection && fVolume)
			fConnection->_PutVolume(fVolume);
	}

	void Detach()
	{
		fConnection = NULL;
		fVolume = NULL;
	}

private:
	ClientConnection*	fConnection;
	ClientVolume*		fVolume;
};

// VolumeNodeMonitoringEvent
struct ClientConnection::VolumeNodeMonitoringEvent {
	VolumeNodeMonitoringEvent(int32 volumeID, NodeMonitoringEvent* event)
		: volumeID(volumeID),
		  event(event)
	{
		if (event)
			event->AcquireReference();
	}

	~VolumeNodeMonitoringEvent()
	{
		if (event)
			event->ReleaseReference();
	}

	int32					volumeID;
	NodeMonitoringEvent*	event;
};

// NodeMonitoringEventQueue
struct ClientConnection::NodeMonitoringEventQueue
	: BlockingQueue<NodeMonitoringRequest> {
	NodeMonitoringEventQueue()
		: BlockingQueue<NodeMonitoringRequest>("client NM requests")
	{
	}
};

// QueryHandleUnlocker
struct ClientConnection::QueryHandleUnlocker {
	QueryHandleUnlocker(ClientConnection* connection, NodeHandle* nodeHandle)
		: fConnection(connection),
		  fHandle(nodeHandle)
	{
	}

	~QueryHandleUnlocker()
	{
		if (fConnection && fHandle) {
			fConnection->_UnlockQueryHandle(fHandle);
			fConnection = NULL;
			fHandle = NULL;
		}
	}

private:
	ClientConnection*	fConnection;
	NodeHandle*			fHandle;
};

// ClientVolumeFilter
struct ClientConnection::ClientVolumeFilter {
	virtual ~ClientVolumeFilter() {}

	virtual bool FilterVolume(ClientConnection* connection,
		ClientVolume* volume) = 0;
};

// HasQueryPermissionClientVolumeFilter
struct ClientConnection::HasQueryPermissionClientVolumeFilter
	: ClientConnection::ClientVolumeFilter {
	virtual bool FilterVolume(ClientConnection* connection,
		ClientVolume* volume)
	{
		return volume->GetSharePermissions().ImpliesQuerySharePermission();
	}
};


// #pragma mark -

// constructor
ClientConnection::ClientConnection(Connection* connection,
	SecurityContext* securityContext, User* user,
	ClientConnectionListener* listener)
	: RequestHandler(),
	  ClientVolume::NodeMonitoringProcessor(),
	  fConnection(NULL),
	  fSecurityContext(securityContext),
	  fSecurityContextLock("security context lock"),
	  fUser(user),
	  fVolumes(NULL),
	  fQueryHandles(NULL),
	  fListener(listener),
	  fNodeMonitoringEvents(NULL),
	  fNodeMonitoringProcessor(-1),
	  fLock("client connection locker"),
	  fReferenceCount(0),
	  fInitialized(0),
	  fClosed(false),
	  fError(false),
	  fInverseClientEndianess(false)
{
	fConnection = new(std::nothrow) RequestConnection(connection, this);
	if (!fConnection)
		delete connection;
}

// destructor
ClientConnection::~ClientConnection()
{
	_Close();
	delete fConnection;

	// delete all volumes
	for (VolumeMap::Iterator it = fVolumes->GetIterator(); it.HasNext();)
		delete it.Next().value;
	delete fVolumes;

	delete fQueryHandles;
	delete fNodeMonitoringEvents;
}

// Init
status_t
ClientConnection::Init()
{
	// create a client volume map
	fVolumes = new(std::nothrow) VolumeMap;
	if (!fVolumes)
		return B_NO_MEMORY;
	status_t error = fVolumes->InitCheck();
	if (error != B_OK)
		return error;

	// create the query handle map
	fQueryHandles = new(std::nothrow) NodeHandleMap("query handles");
	if (!fQueryHandles)
		return B_NO_MEMORY;
	error = fQueryHandles->Init();
	if (error != B_OK)
		return error;

	// create the node monitoring event queue
	fNodeMonitoringEvents = new(std::nothrow) NodeMonitoringEventQueue;
	if (!fNodeMonitoringEvents)
		return B_NO_MEMORY;
	error = fNodeMonitoringEvents->InitCheck();
	if (error != B_OK)
		return error;

	// initialize the connection
	error = fConnection->Init();
	if (error != B_OK)
		return error;

	// start the node monitoring processor
	fNodeMonitoringProcessor = spawn_thread(_NodeMonitoringProcessorEntry,
		"client connection NM processor", B_NORMAL_PRIORITY, this);
	if (fNodeMonitoringProcessor < 0) {
		_Close();
		return fNodeMonitoringProcessor;
	}
	resume_thread(fNodeMonitoringProcessor);
	return B_OK;
}

// Close
/*!
	Called by the NetFSServer. Not for internal use. Waits for the connection
	to be closed (at least for the node monitoring thread to be gone).
*/
void
ClientConnection::Close()
{
	{
		ConnectionReference connectionReference(this);
		if (connectionReference.IsValid())
			_MarkClosed(false);
		fListener = NULL;
	}

	// Wait at least for the node monitoring processor; this is not perfect,
	// but not too bad either.
	if (fNodeMonitoringProcessor >= 0
		&& find_thread(NULL) != fNodeMonitoringProcessor) {
		int32 result;
		wait_for_thread(fNodeMonitoringProcessor, &result);
	}
}

// GetReference
bool
ClientConnection::GetReference()
{
	AutoLocker<Locker> _(fLock);
	if (fClosed || !atomic_or(&fInitialized, 0))
		return false;
	fReferenceCount++;
	return true;
}

// PutReference
void
ClientConnection::PutReference()
{
	bool close = false;
	{
		AutoLocker<Locker> _(fLock);
		--fReferenceCount;
		if (fClosed)
			close = (fReferenceCount == 0);
	}
	if (close)
		_Close();
}

// UserRemoved
void
ClientConnection::UserRemoved(User* user)
{
	// get all volumes
	ClientVolume** volumes = NULL;
	int32 volumeCount = 0;
	AutoLocker<VolumeMap> volumesLocker(fVolumes);
	volumes = new(std::nothrow) ClientVolume*[fVolumes->Size()];
	if (!volumes) {
		ERROR(("ClientConnection::UserRemoved(): ERROR: Failed to "
			"allocate memory for volume array.\n"));
		volumesLocker.Unlock();
		_UnmountAllVolumes();
		return;
	}
	for (VolumeMap::Iterator it = fVolumes->GetIterator(); it.HasNext();) {
		if (ClientVolume* volume = _GetVolume(it.Next().value->GetID()))
			volumes[volumeCount++] = volume;
	}
	volumesLocker.Unlock();

	// unmount the concerned volumes
	for (int32 i = 0; i < volumeCount; i++) {
		ClientVolume* volume = volumes[i];

		fSecurityContextLock.Lock();
		bool unmount = (volume->GetSecurityContext()->GetUser() == user);
		fSecurityContextLock.Unlock();

		if (unmount)
			_UnmountVolume(volume);
	}

	// put the volumes
	for (int32 i = 0; i < volumeCount; i++)
		_PutVolume(volumes[i]);
	delete[] volumes;
}

// ShareRemoved
void
ClientConnection::ShareRemoved(Share* share)
{
	// get all volumes
	ClientVolume** volumes = NULL;
	int32 volumeCount = 0;
	AutoLocker<VolumeMap> volumesLocker(fVolumes);
	volumes = new(std::nothrow) ClientVolume*[fVolumes->Size()];
	if (!volumes) {
		ERROR(("ClientConnection::ShareRemoved(): ERROR: Failed to "
			"allocate memory for volume array.\n"));
		volumesLocker.Unlock();
		_UnmountAllVolumes();
		return;
	}
	for (VolumeMap::Iterator it = fVolumes->GetIterator(); it.HasNext();) {
		if (ClientVolume* volume = _GetVolume(it.Next().value->GetID()))
			volumes[volumeCount++] = volume;
	}
	volumesLocker.Unlock();

	// unmount the concerned volumes
	for (int32 i = 0; i < volumeCount; i++) {
		ClientVolume* volume = volumes[i];

		fSecurityContextLock.Lock();
		bool unmount = (volume->GetShare() == share);
		fSecurityContextLock.Unlock();

		if (unmount)
			_UnmountVolume(volume);
	}

	// put the volumes
	for (int32 i = 0; i < volumeCount; i++)
		_PutVolume(volumes[i]);
	delete[] volumes;
}

// UserPermissionsChanged
void
ClientConnection::UserPermissionsChanged(Share* share, User* user,
	Permissions permissions)
{
	bool unmountAll = (!permissions.ImpliesMountSharePermission());

	// get all volumes
	ClientVolume** volumes = NULL;
	int32 volumeCount = 0;
	AutoLocker<VolumeMap> volumesLocker(fVolumes);
	volumes = new(std::nothrow) ClientVolume*[fVolumes->Size()];
	if (!volumes) {
		ERROR(("ClientConnection::ShareRemoved(): ERROR: Failed to "
			"allocate memory for volume array.\n"));
		volumesLocker.Unlock();
		_UnmountAllVolumes();
		return;
	}
	for (VolumeMap::Iterator it = fVolumes->GetIterator(); it.HasNext();) {
		if (ClientVolume* volume = _GetVolume(it.Next().value->GetID()))
			volumes[volumeCount++] = volume;
	}
	volumesLocker.Unlock();

	// update the concerned volumes
	for (int32 i = 0; i < volumeCount; i++) {
		ClientVolume* volume = volumes[i];

		fSecurityContextLock.Lock();
		bool concerned = (volume->GetShare() == share
			&& volume->GetSecurityContext()->GetUser() == user);
		fSecurityContextLock.Unlock();

		if (concerned) {
			// create a new user security context for the volume
			status_t error = B_OK;

			if (unmountAll) {
				_UnmountVolume(volume);
			} else {
				// create a new user security context
				AutoLocker<Locker> securityContextLocker(fSecurityContextLock);
				UserSecurityContext* userSecurityContext
					= new(std::nothrow) UserSecurityContext;

				// init it
				if (userSecurityContext) {
					error = fSecurityContext->GetUserSecurityContext(user,
						userSecurityContext);
				} else
					error = B_NO_MEMORY;
				if (error != B_OK) {
					delete userSecurityContext;
					securityContextLocker.Unlock();
					_UnmountVolume(volume);
					continue;
				}

				// set the volume's new user security context
				securityContextLocker.Unlock();
				volume->SetSecurityContext(userSecurityContext);
			}
		}
	}

	// put the volumes
	for (int32 i = 0; i < volumeCount; i++)
		_PutVolume(volumes[i]);
	delete[] volumes;
}


// #pragma mark -

// VisitConnectionBrokenRequest
status_t
ClientConnection::VisitConnectionBrokenRequest(ConnectionBrokenRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	_MarkClosed(true);
	return B_OK;
}

// VisitInitConnectionRequest
status_t
ClientConnection::VisitInitConnectionRequest(InitConnectionRequest* request)
{
	bool alreadyInitialized = atomic_or(&fInitialized, ~0);

	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	if (!alreadyInitialized)
		fInverseClientEndianess = (request->bigEndian != B_HOST_IS_BENDIAN);

	// prepare the reply
	InitConnectionReply reply;

	// send the reply
	reply.error = (alreadyInitialized ? B_BAD_VALUE : B_OK);
	status_t error = GetChannel()->SendRequest(&reply);

	// on error just close
	if (error != B_OK)
		_MarkClosed(true);
	return B_OK;
}

// VisitMountRequest
status_t
ClientConnection::VisitMountRequest(MountRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	status_t result = B_OK;
	const char* shareName = request->share.GetString();
	if (!shareName)
		SET_ERROR(result, B_BAD_DATA);

	// create a volume
	ClientVolume* volume = NULL;
	if (result == B_OK)
		result = _CreateVolume(&volume);
	ClientVolumePutter volumePutter(this, volume);

	// if we haven't been supplied with a user yet, use the info from the
	// mount request for authentication
	VolumeManagerLocker managerLocker;
	AutoLocker<Locker> securityContextLocker(fSecurityContextLock);
	const char* userName = request->user.GetString();
	User* user = fUser;
	BReference<User> userReference(user);
	bool noPermission = false;
	if (result == B_OK && !user) {
		if (userName) {
			SET_ERROR(result, fSecurityContext->AuthenticateUser(userName,
				request->password.GetString(), &user));
			if (result == B_OK)
				userReference.SetTo(user, true);
		} else
			result = B_PERMISSION_DENIED;
		if (result == B_PERMISSION_DENIED)
			noPermission = true;
	}

	// create a user security context
	UserSecurityContext* securityContext = NULL;
	if (result == B_OK) {
		securityContext = new(std::nothrow) UserSecurityContext;
		if (securityContext) {
			SET_ERROR(result, fSecurityContext->GetUserSecurityContext(user,
				securityContext));
		} else
			SET_ERROR(result, B_NO_MEMORY);
	}
	ObjectDeleter<UserSecurityContext> securityContextDeleter(securityContext);

	// get the share
	Share* share = NULL;
	Permissions sharePermissions;
	node_ref mountPoint;
	if (result == B_OK) {
		AutoLocker<SecurityContext> _(fSecurityContext);
		share = fSecurityContext->FindShare(shareName);
		if (share) {
			mountPoint = share->GetNodeRef();
			sharePermissions = securityContext->GetNodePermissions(
				mountPoint);
			if (!sharePermissions.ImpliesMountSharePermission()) {
				SET_ERROR(result, B_PERMISSION_DENIED);
				noPermission = true;
			}
		} else
			SET_ERROR(result, B_ENTRY_NOT_FOUND);
	}
	BReference<Share> shareReference(share, true);

	// mount the volume
	MountReply reply;
	if (result == B_OK) {
		SET_ERROR(result, volume->Mount(securityContext, share));
		securityContextDeleter.Detach();
	}
	if (result == B_OK) {
		_GetNodeInfo(volume->GetRootDirectory(), &reply.nodeInfo);
		reply.sharePermissions = sharePermissions.GetPermissions();
		reply.volumeID = volume->GetID();
	}

	// make sure, the volume is removed on error
	if (result != B_OK && volume) {
		AutoLocker<VolumeMap> volumeMapLocker(fVolumes);
		volume->MarkRemoved();
	}

	securityContextLocker.Unlock();
	managerLocker.Unlock();

	// send the reply
	reply.error = result;
	reply.noPermission = noPermission;
	return GetChannel()->SendRequest(&reply);
}

// VisitUnmountRequest
status_t
ClientConnection::VisitUnmountRequest(UnmountRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	if (ClientVolume* volume = _GetVolume(request->volumeID)) {
		_UnmountVolume(volume);
		_PutVolume(volume);
	}

	return B_OK;
}

// VisitReadVNodeRequest
status_t
ClientConnection::VisitReadVNodeRequest(ReadVNodeRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	VolumeManagerLocker managerLocker;

	// get the node
	Node* node = NULL;
	if (result == B_OK) {
		node = volume->GetNode(request->nodeID);
		if (!node)
			result = B_ENTRY_NOT_FOUND;
	}

	// prepare the reply
	ReadVNodeReply reply;
	if (result == B_OK)
		_GetNodeInfo(node, &reply.nodeInfo);

	managerLocker.Unlock();

	// send the reply
	reply.error = result;
	return GetChannel()->SendRequest(&reply);
}

// VisitWriteStatRequest
status_t
ClientConnection::VisitWriteStatRequest(WriteStatRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	VolumeManagerLocker managerLocker;

	// get the node
	Node* node = NULL;
	if (result == B_OK) {
		node = volume->GetNode(request->nodeID);
		if (!node)
			result = B_ENTRY_NOT_FOUND;
	}

	// check permissions
	if (result == B_OK) {
		if (!volume->GetNodePermissions(node).ImpliesWritePermission())
			result = B_PERMISSION_DENIED;
	}

	// get the path
	Path path;
	if (result == B_OK)
		result = node->GetPath(&path);

	// write the stat
	uint32 mask = request->mask;
	// size
	if (result == B_OK && (mask & WSTAT_SIZE)) {
		if (truncate(path.GetPath(), request->nodeInfo.st.st_size) < 0)
			result = errno;
	}
	// mode
	if (result == B_OK && (mask & WSTAT_MODE)) {
		if (chmod(path.GetPath(), request->nodeInfo.st.st_mode) < 0)
			result = errno;
	}
	// mtime
	if (result == B_OK && (mask & (WSTAT_ATIME | WSTAT_MTIME))) {
		utimbuf buffer;
		buffer.actime = (mask & WSTAT_ATIME)
			? request->nodeInfo.st.st_atime
			: node->GetStat().st_atime;
		buffer.modtime = (mask & WSTAT_MTIME)
			? request->nodeInfo.st.st_mtime
			: node->GetStat().st_mtime;
		if (utime(path.GetPath(), &buffer) < 0)
			result = errno;
	}
	// ignore WSTAT_CRTIME, WSTAT_UID, WSTAT_GID for the time being

	// prepare the reply
	WriteStatReply reply;
	// update the node stat
	reply.nodeInfoValid = false;
	if (node) {
		if (node->UpdateStat() == B_OK) {
			_GetNodeInfo(node, &reply.nodeInfo);
			reply.nodeInfoValid = true;
		}
	}

	managerLocker.Unlock();

	// send the reply
	reply.error = result;
	return GetChannel()->SendRequest(&reply);
}

// VisitCreateFileRequest
status_t
ClientConnection::VisitCreateFileRequest(CreateFileRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	VolumeManagerLocker managerLocker;

	// get the directory
	Directory* directory = NULL;
	if (result == B_OK) {
		Node* node = volume->GetNode(request->directoryID);
		if (node) {
			directory = dynamic_cast<Directory*>(node);
			if (!directory)
				result = B_NOT_A_DIRECTORY;
		} else
			result = B_ENTRY_NOT_FOUND;
	}

	// check permissions
	int openMode = request->openMode;
	if (result == B_OK) {
		if (!volume->GetNodePermissions(directory).ImpliesWritePermission())
			result = B_PERMISSION_DENIED;
	}

	// get the path
	Path path;
	if (result == B_OK) {
		result = directory->GetPath(&path);
		if (result == B_OK)
			result = path.Append(request->name.GetString());
	}

	// create the file
	if (result == B_OK) {
		int fd = -1;
		result = FDManager::Open(path.GetPath(),
			openMode | O_CREAT | O_NOTRAVERSE, request->mode, fd);
		if (result == B_OK)
			close(fd);
	}

	// load the new entry
	Entry* entry = NULL;
	if (result == B_OK) {
		VolumeManager* volumeManager = VolumeManager::GetDefault();

		// if there existed an entry before, we need to delete it, to avoid that
		// we open the wrong node
		entry = volumeManager->GetEntry(directory->GetVolumeID(),
			directory->GetID(), request->name.GetString());
		if (entry)
			volumeManager->DeleteEntry(entry, false);

		// load the new entry
		entry = NULL;
		result = volume->LoadEntry(directory, request->name.GetString(),
			&entry);
	}

	// open the node
	FileHandle* handle = NULL;
	if (result == B_OK) {
		openMode &= ~(O_CREAT | O_EXCL | O_TRUNC);
		result = volume->Open(entry->GetNode(), openMode, &handle);
	}
	NodeHandleUnlocker handleUnlocker(volume, handle);

	// prepare the reply
	CreateFileReply reply;
	if (result == B_OK) {
		_GetEntryInfo(entry, &reply.entryInfo);
		reply.cookie = handle->GetCookie();
	}

	managerLocker.Unlock();

	// send the reply
	reply.error = result;
	status_t error = GetChannel()->SendRequest(&reply);

	// close the handle, if a send error occurred
	if (error != B_OK && result == B_OK)
		volume->Close(handle);

	return error;
}

// VisitOpenRequest
status_t
ClientConnection::VisitOpenRequest(OpenRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	VolumeManagerLocker managerLocker;

	// get the node
	Node* node = NULL;
	if (result == B_OK) {
		node = volume->GetNode(request->nodeID);
		if (!node)
			result = B_ENTRY_NOT_FOUND;
	}

	// check permissions
	int openMode = request->openMode;
	if (result == B_OK) {
		Permissions permissions = volume->GetNodePermissions(node);
		if ((openMode & O_RWMASK) == O_RDWR) {
			// read+write: fall back to read/write only, if the other permission
			// is missing
			if (!permissions.ImpliesReadPermission())
				openMode = (openMode & ~O_RWMASK) | O_WRONLY;
			else if (!permissions.ImpliesWritePermission())
				openMode = (openMode & ~O_RWMASK) | O_RDONLY;
		}
		if ((openMode & O_RWMASK) == O_RDONLY) {
			if (!permissions.ImpliesReadPermission())
				result = B_PERMISSION_DENIED;
		} else if ((openMode & O_RWMASK) == O_WRONLY) {
			if (!permissions.ImpliesWritePermission())
				result = B_PERMISSION_DENIED;
		}
	}

	// open the node
	FileHandle* handle = NULL;
	if (result == B_OK)
		result = volume->Open(node, openMode, &handle);
	NodeHandleUnlocker handleUnlocker(volume, handle);

	// prepare the reply
	OpenReply reply;
	if (result == B_OK) {
		_GetNodeInfo(node, &reply.nodeInfo);
		reply.cookie = handle->GetCookie();
	}

	managerLocker.Unlock();

	// send the reply
	reply.error = result;
	status_t error = GetChannel()->SendRequest(&reply);

	// close the handle, if a send error occurred
	if (error != B_OK && result == B_OK)
		volume->Close(handle);

	return error;
}

// VisitCloseRequest
status_t
ClientConnection::VisitCloseRequest(CloseRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	status_t result = B_OK;

	if (request->volumeID >= 0) {
		// get the volume
		ClientVolume* volume = _GetVolume(request->volumeID);
		if (!volume)
			SET_ERROR(result, B_BAD_VALUE);
		ClientVolumePutter volumePutter(this, volume);

		// get the node handle
		NodeHandle* handle = NULL;
		if (result == B_OK)
			SET_ERROR(result, volume->LockNodeHandle(request->cookie, &handle));
		NodeHandleUnlocker handleUnlocker(volume, handle);

		VolumeManagerLocker managerLocker;

		// close it
		if (result == B_OK)
			SET_ERROR(result, volume->Close(handle));

		managerLocker.Unlock();
	} else {
		// no volume ID given, so this is a query handle
		// lock the handle
		QueryHandle* handle = NULL;
		if (result == B_OK)
			SET_ERROR(result, _LockQueryHandle(request->cookie, &handle));
		QueryHandleUnlocker handleUnlocker(this, handle);

		// close it
		if (result == B_OK)
			SET_ERROR(result, _CloseQuery(handle));
	}

	// send the reply
	CloseReply reply;
	reply.error = result;
	return GetChannel()->SendRequest(&reply);
}

// VisitReadRequest
status_t
ClientConnection::VisitReadRequest(ReadRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	// get the node handle
	NodeHandle* handle = NULL;
	if (result == B_OK)
		result = volume->LockNodeHandle(request->cookie, &handle);
	NodeHandleUnlocker handleUnlocker(volume, handle);

	// check if it is a file handle
	FileHandle* fileHandle = NULL;
	if (result == B_OK) {
		fileHandle = dynamic_cast<FileHandle*>(handle);
		if (!fileHandle)
			result = B_BAD_VALUE;
	}

	VolumeManagerLocker managerLocker;

	// check read permission
	if (result == B_OK) {
		Node* node = volume->GetNode(fileHandle->GetNodeRef());
		if (!node || !volume->GetNodePermissions(node).ImpliesReadPermission())
			result = B_PERMISSION_DENIED;
	}

	managerLocker.Unlock();

	off_t pos = request->pos;
	int32 size = request->size;
	int32 bufferSize = min(size, kMaxReadBufferSize);
	// allocate a buffer
	uint8* buffer = NULL;
	if (result == B_OK) {
		buffer = (uint8*)malloc(bufferSize);
		if (!buffer)
			result = B_NO_MEMORY;
	}
	MemoryDeleter bufferDeleter(buffer);

	// read as long as there are bytes left to read or an error occurs
	bool moreToRead = true;
	do {
		int32 bytesToRead = min(size, bufferSize);
		size_t bytesRead = 0;
		if (result == B_OK)
			result = fileHandle->Read(pos, buffer, bytesToRead, &bytesRead);
		moreToRead = (result == B_OK && bytesRead > 0
			&& (int32)bytesRead < size);

		// prepare the reply
		ReadReply reply;
		if (result == B_OK) {
			reply.pos = pos;
			reply.data.SetTo(buffer, bytesRead);
			reply.moreToCome = moreToRead;
			pos += bytesRead;
			size -= bytesRead;
		}

		// send the reply
		reply.error = result;
		status_t error = GetChannel()->SendRequest(&reply);
		if (error != B_OK)
			return error;
	} while (moreToRead);

	return B_OK;
}

// VisitWriteRequest
status_t
ClientConnection::VisitWriteRequest(WriteRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	// get the node handle
	NodeHandle* handle = NULL;
	if (result == B_OK)
		result = volume->LockNodeHandle(request->cookie, &handle);
	NodeHandleUnlocker handleUnlocker(volume, handle);

	// check if it is a file handle
	FileHandle* fileHandle = NULL;
	if (result == B_OK) {
		fileHandle = dynamic_cast<FileHandle*>(handle);
		if (!fileHandle)
			result = B_BAD_VALUE;
	}

	VolumeManagerLocker managerLocker;

	// check read permission
	if (result == B_OK) {
		Node* node = volume->GetNode(fileHandle->GetNodeRef());
		if (!node || !volume->GetNodePermissions(node).ImpliesWritePermission())
			result = B_PERMISSION_DENIED;
	}

	managerLocker.Unlock();

	// write until all has been written or an error occurs
	off_t pos = request->pos;
	int32 size = request->data.GetSize();
	const char* buffer = (const char*)request->data.GetData();
	while (result == B_OK && size > 0) {
		size_t bytesWritten;
		result = fileHandle->Write(pos, buffer, size, &bytesWritten);
		if (result == B_OK) {
			pos += bytesWritten;
			buffer += bytesWritten;
			size -= bytesWritten;
		}
	}

	// prepare the reply
	WriteReply reply;
	// send the reply
	reply.error = result;
	return GetChannel()->SendRequest(&reply);
}

// VisitCreateLinkRequest
status_t
ClientConnection::VisitCreateLinkRequest(CreateLinkRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	VolumeManagerLocker managerLocker;

	// get the target node
	Node* node = NULL;
	if (result == B_OK) {
		node = volume->GetNode(request->nodeID);
		if (!node)
			result = B_ENTRY_NOT_FOUND;
	}

	// get the target node path
	Path targetPath;
	if (result == B_OK)
		result = node->GetPath(&targetPath);

	// get the directory
	Directory* directory = NULL;
	if (result == B_OK) {
		Node* node = volume->GetNode(request->directoryID);
		if (node) {
			directory = dynamic_cast<Directory*>(node);
			if (!directory)
				result = B_NOT_A_DIRECTORY;
		} else
			result = B_ENTRY_NOT_FOUND;
	}

	// check permissions
	if (result == B_OK) {
		if (!volume->GetNodePermissions(directory).ImpliesWritePermission())
			result = B_PERMISSION_DENIED;
	}

	// get the new entry's path
	Path path;
	if (result == B_OK) {
		result = directory->GetPath(&path);
		if (result == B_OK)
			result = path.Append(request->name.GetString());
	}

	managerLocker.Unlock();

	// create the link
	if (result == B_OK) {
		if (link(targetPath.GetPath(), path.GetPath()) < 0)
			result = errno;
	}

	// prepare the reply
	CreateSymlinkReply reply;
	// send the reply
	reply.error = result;
	return GetChannel()->SendRequest(&reply);
}

// VisitUnlinkRequest
status_t
ClientConnection::VisitUnlinkRequest(UnlinkRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	VolumeManagerLocker managerLocker;

	// get the directory
	Directory* directory = NULL;
	if (result == B_OK) {
		Node* node = volume->GetNode(request->directoryID);
		if (node) {
			directory = dynamic_cast<Directory*>(node);
			if (!directory)
				result = B_NOT_A_DIRECTORY;
		} else
			result = B_ENTRY_NOT_FOUND;
	}

	// check permissions
	if (result == B_OK) {
		if (!volume->GetNodePermissions(directory).ImpliesWritePermission())
			result = B_PERMISSION_DENIED;
	}

	// get the entry's path
	Path path;
	if (result == B_OK) {
		result = directory->GetPath(&path);
		if (result == B_OK)
			result = path.Append(request->name.GetString());
	}

	managerLocker.Unlock();

	// remove the entry
	if (result == B_OK) {
		if (unlink(path.GetPath()) < 0)
			result = errno;
	}

	// prepare the reply
	UnlinkReply reply;
	// send the reply
	reply.error = result;
	return GetChannel()->SendRequest(&reply);
}

// VisitCreateSymlinkRequest
status_t
ClientConnection::VisitCreateSymlinkRequest(CreateSymlinkRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	VolumeManagerLocker managerLocker;

	// get the directory
	Directory* directory = NULL;
	if (result == B_OK) {
		Node* node = volume->GetNode(request->directoryID);
		if (node) {
			directory = dynamic_cast<Directory*>(node);
			if (!directory)
				result = B_NOT_A_DIRECTORY;
		} else
			result = B_ENTRY_NOT_FOUND;
	}

	// check permissions
	if (result == B_OK) {
		if (!volume->GetNodePermissions(directory).ImpliesWritePermission())
			result = B_PERMISSION_DENIED;
	}

	// get the new entry's path
	Path path;
	if (result == B_OK) {
		result = directory->GetPath(&path);
		if (result == B_OK)
			result = path.Append(request->name.GetString());
	}

	managerLocker.Unlock();

	// create the symlink
	if (result == B_OK) {
		if (symlink(request->target.GetString(), path.GetPath()) < 0)
			result = errno;
	}

	// prepare the reply
	CreateSymlinkReply reply;
	// send the reply
	reply.error = result;
	return GetChannel()->SendRequest(&reply);
}

// VisitReadLinkRequest
status_t
ClientConnection::VisitReadLinkRequest(ReadLinkRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	VolumeManagerLocker managerLocker;

	// get the node
	Node* node = NULL;
	if (result == B_OK) {
		node = volume->GetNode(request->nodeID);
		if (!node)
			result = B_ENTRY_NOT_FOUND;
	}

	int32 bufferSize = request->maxSize;

	// check read permission
	if (result == B_OK) {
		if (!volume->GetNodePermissions(node).ImpliesReadPermission())
			result = B_PERMISSION_DENIED;
	}

	// allocate a buffer
	void* buffer = NULL;
	if (result == B_OK) {
		if (bufferSize < 0 || bufferSize > kMaxSaneReadLinkSize)
			result = B_BAD_DATA;
	}
	if (result == B_OK) {
		buffer = malloc(bufferSize);
		if (!buffer)
			result = B_NO_MEMORY;
	}
	MemoryDeleter bufferDeleter(buffer);

	// read the link and prepare the reply
	ReadLinkReply reply;
	int32 bytesRead = 0;
	if (result == B_OK)
		result = node->ReadSymlink((char*)buffer, bufferSize, &bytesRead);
	if (result == B_OK) {
		reply.data.SetTo(buffer, bytesRead);
		_GetNodeInfo(node, &reply.nodeInfo);
	}

	managerLocker.Unlock();

	// send the reply
	reply.error = result;
	return GetChannel()->SendRequest(&reply);
}

// VisitRenameRequest
status_t
ClientConnection::VisitRenameRequest(RenameRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	VolumeManagerLocker managerLocker;

	// get the new directory
	Directory* newDirectory = NULL;
	if (result == B_OK) {
		Node* node = volume->GetNode(request->newDirectoryID);
		if (node) {
			newDirectory = dynamic_cast<Directory*>(node);
			if (!newDirectory)
				result = B_NOT_A_DIRECTORY;
		} else
			result = B_ENTRY_NOT_FOUND;
	}

	// check permissions
	if (result == B_OK) {
		if (!volume->GetNodePermissions(newDirectory).ImpliesWritePermission())
			result = B_PERMISSION_DENIED;
	}

	// get the new path
	Path newPath;
	if (result == B_OK) {
		result = newDirectory->GetPath(&newPath);
		if (result == B_OK)
			result = newPath.Append(request->newName.GetString());
	}

	// get the old directory
	Directory* oldDirectory = NULL;
	if (result == B_OK) {
		Node* node = volume->GetNode(request->oldDirectoryID);
		if (node) {
			oldDirectory = dynamic_cast<Directory*>(node);
			if (!oldDirectory)
				result = B_NOT_A_DIRECTORY;
		} else
			result = B_ENTRY_NOT_FOUND;
	}

	// check permissions
	if (result == B_OK) {
		if (!volume->GetNodePermissions(oldDirectory).ImpliesWritePermission())
			result = B_PERMISSION_DENIED;
	}

	// get the new path
	Path oldPath;
	if (result == B_OK) {
		result = oldDirectory->GetPath(&oldPath);
		if (result == B_OK)
			result = oldPath.Append(request->oldName.GetString());
	}

	managerLocker.Unlock();

	// rename the entry
	if (result == B_OK) {
		if (rename(oldPath.GetPath(), newPath.GetPath()) < 0)
			result = errno;
	}

	// prepare the reply
	RenameReply reply;
	// send the reply
	reply.error = result;
	return GetChannel()->SendRequest(&reply);
}

// VisitMakeDirRequest
status_t
ClientConnection::VisitMakeDirRequest(MakeDirRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	VolumeManagerLocker managerLocker;

	// get the directory
	Directory* directory = NULL;
	if (result == B_OK) {
		Node* node = volume->GetNode(request->directoryID);
		if (node) {
			directory = dynamic_cast<Directory*>(node);
			if (!directory)
				result = B_NOT_A_DIRECTORY;
		} else
			result = B_ENTRY_NOT_FOUND;
	}

	// check permissions
	if (result == B_OK) {
		if (!volume->GetNodePermissions(directory).ImpliesWritePermission())
			result = B_PERMISSION_DENIED;
	}

	// get the path
	Path path;
	if (result == B_OK) {
		result = directory->GetPath(&path);
		if (result == B_OK)
			result = path.Append(request->name.GetString());
	}

	managerLocker.Unlock();

	// create the directory
	if (result == B_OK) {
		if (mkdir(path.GetPath(), request->mode) < 0)
			result = errno;
	}

	// prepare the reply
	MakeDirReply reply;
	// send the reply
	reply.error = result;
	return GetChannel()->SendRequest(&reply);
}

// VisitRemoveDirRequest
status_t
ClientConnection::VisitRemoveDirRequest(RemoveDirRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	VolumeManagerLocker managerLocker;

	// get the directory
	Directory* directory = NULL;
	if (result == B_OK) {
		Node* node = volume->GetNode(request->directoryID);
		if (node) {
			directory = dynamic_cast<Directory*>(node);
			if (!directory)
				result = B_NOT_A_DIRECTORY;
		} else
			result = B_ENTRY_NOT_FOUND;
	}

	// check permissions
	if (result == B_OK) {
		if (!volume->GetNodePermissions(directory).ImpliesWritePermission())
			result = B_PERMISSION_DENIED;
	}

	// get the path
	Path path;
	if (result == B_OK) {
		result = directory->GetPath(&path);
		if (result == B_OK)
			result = path.Append(request->name.GetString());
	}

	managerLocker.Unlock();

	// remove the directory
	if (result == B_OK) {
		if (rmdir(path.GetPath()) < 0)
			result = errno;
	}

	// prepare the reply
	RemoveDirReply reply;
	// send the reply
	reply.error = result;
	return GetChannel()->SendRequest(&reply);
}

// VisitOpenDirRequest
status_t
ClientConnection::VisitOpenDirRequest(OpenDirRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	VolumeManagerLocker managerLocker;

	// get the directory
	Directory* directory = NULL;
	if (result == B_OK) {
		Node* node = volume->GetNode(request->nodeID);
		if (node) {
			directory = dynamic_cast<Directory*>(node);
			if (!directory)
				result = B_NOT_A_DIRECTORY;
		} else
			result = B_ENTRY_NOT_FOUND;
	}

	// check permission
	if (result == B_OK) {
		if (!volume->GetNodePermissions(directory).ImpliesReadDirPermission())
			result = B_PERMISSION_DENIED;
	}

	// open the directory
	DirIterator* handle = NULL;
	if (result == B_OK)
		result = volume->OpenDir(directory, &handle);
	NodeHandleUnlocker handleUnlocker(volume, handle);

	// prepare the reply
	OpenDirReply reply;
	if (result == B_OK) {
		_GetNodeInfo(directory, &reply.nodeInfo);
		reply.cookie = handle->GetCookie();
	}
else {
if (directory)
PRINT("OpenDir() failed: client volume: %ld, node: (%ld, %lld)\n",
volume->GetID(), directory->GetVolumeID(), directory->GetID());
}

	managerLocker.Unlock();

	// send the reply
	reply.error = result;
	status_t error = GetChannel()->SendRequest(&reply);

	// close the handle, if a send error occurred
	if (error != B_OK && result == B_OK)
		volume->Close(handle);

	return error;
}

// VisitReadDirRequest
status_t
ClientConnection::VisitReadDirRequest(ReadDirRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	// get the node handle
	NodeHandle* handle = NULL;
	if (result == B_OK)
		result = volume->LockNodeHandle(request->cookie, &handle);
	NodeHandleUnlocker handleUnlocker(volume, handle);

	// check if it is a directory iterator
	DirIterator* iterator = NULL;
	if (result == B_OK) {
		iterator = dynamic_cast<DirIterator*>(handle);
		if (!iterator)
			result = B_BAD_VALUE;
	}

	VolumeManagerLocker managerLocker;

	// get the directory
	Directory* directory = NULL;
	if (result == B_OK) {
		Node* node = volume->GetNode(iterator->GetNodeRef());
		if (node) {
			directory = dynamic_cast<Directory*>(node);
			if (!directory)
				result = B_NOT_A_DIRECTORY;
		} else
			result = B_ENTRY_NOT_FOUND;
	}

	// check read permission
	if (result == B_OK) {
		if (!volume->GetNodePermissions(directory).ImpliesReadDirPermission())
			result = B_PERMISSION_DENIED;
	}

if (result == B_OK) {
	PRINT("ReadDir: (%ld, %lld)\n", request->volumeID, directory->GetID());
}

	// rewind, if requested
	if (result == B_OK && request->rewind)
		iterator->Rewind();

	// read the directory
	bool done = false;
	ReadDirReply reply;
	int32 toRead = request->count;
	while (result == B_OK && toRead > 0) {
		// get the next entry
		Entry* entry = iterator->NextEntry();
		if (!entry) {
			done = true;
			break;
		}

		// get and add an entry info
		EntryInfo entryInfo;
		_GetEntryInfo(entry, &entryInfo);
		result = reply.entryInfos.Append(entryInfo);

		toRead--;
	}

	reply.revision = VolumeManager::GetDefault()->GetRevision();

//PRINT(("ReadDir: (%lld) -> (%lx, %ld, dir: %lld, node: %lld, `%s')\n",
//directoryID, reply.error, reply.entryInfos.CountElements(),
//reply.entryInfo.directoryID,
//reply.entryInfo.nodeID, reply.entryInfo.name.GetString()));
if (directory) {
PRINT("ReadDir done: volume: %ld, (%ld, %lld) -> (%lx, %ld)\n",
volume->GetID(), directory->GetVolumeID(), directory->GetID(), result,
reply.entryInfos.CountElements());
}

	managerLocker.Unlock();

	// send the reply
	reply.error = result;
	reply.done = (result != B_OK || done);
	return GetChannel()->SendRequest(&reply);
}

// VisitWalkRequest
status_t
ClientConnection::VisitWalkRequest(WalkRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	VolumeManagerLocker managerLocker;

	// get the directory
	Directory* directory = NULL;
	if (result == B_OK) {
		Node* node = volume->GetNode(request->nodeID);
		if (node) {
			directory = dynamic_cast<Directory*>(node);
			if (!directory)
				result = B_NOT_A_DIRECTORY;
		} else
			result = B_ENTRY_NOT_FOUND;
	}

	// check permission
	if (result == B_OK) {
		if (!volume->GetNodePermissions(directory)
				.ImpliesResolveDirEntryPermission()) {
			result = B_PERMISSION_DENIED;
		}
	}

	WalkReply reply;
	char linkPath[B_PATH_NAME_LENGTH];
	if (result == B_OK) {
		// load the entry
		Entry* entry;
		result = volume->LoadEntry(directory, request->name.GetString(),
			&entry);

		// fill in the reply
		if (result == B_OK) {
			_GetEntryInfo(entry, &reply.entryInfo);

			// resolve a symlink, if desired
			Node* node = entry->GetNode();
			if (request->resolveLink && node->IsSymlink()) {
				result = node->ReadSymlink(linkPath, B_PATH_NAME_LENGTH);
				if (result == B_OK)
					reply.linkPath.SetTo(linkPath);
			}
		}
	}

	managerLocker.Unlock();

	// send the reply
	reply.error = result;
	PRINT("Walk: (%ld, %lld, `%s') -> (%lx, (%ld, %lld), `%s')\n",
		request->nodeID.volumeID, request->nodeID.nodeID,
		request->name.GetString(), result,
		reply.entryInfo.nodeInfo.st.st_dev,
		reply.entryInfo.nodeInfo.st.st_ino, reply.linkPath.GetString());
	return GetChannel()->SendRequest(&reply);
}

// VisitMultiWalkRequest
status_t
ClientConnection::VisitMultiWalkRequest(MultiWalkRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	VolumeManagerLocker managerLocker;

	// get the directory
	Directory* directory = NULL;
	if (result == B_OK) {
		Node* node = volume->GetNode(request->nodeID);
		if (node) {
			directory = dynamic_cast<Directory*>(node);
			if (!directory)
				result = B_NOT_A_DIRECTORY;
		} else
			result = B_ENTRY_NOT_FOUND;
	}

	// check permission
	if (result == B_OK) {
		if (!volume->GetNodePermissions(directory)
				.ImpliesResolveDirEntryPermission()) {
			result = B_PERMISSION_DENIED;
		}
	}

	MultiWalkReply reply;
	StringData* names = request->names.GetElements();
	int32 count = request->names.CountElements();
	for (int32 i = 0; result == B_OK && i < count; i++) {
		// load the entry
		Entry* entry;
		if (volume->LoadEntry(directory, names[i].GetString(), &entry)
				== B_OK) {
			// add an entry info
			EntryInfo entryInfo;
			_GetEntryInfo(entry, &entryInfo);

			// append the info
			result = reply.entryInfos.Append(entryInfo);
		}
	}

	managerLocker.Unlock();

	// send the reply
	reply.error = result;
	PRINT("MultiWalk: (%ld, %lld, %ld) -> (%lx, %ld)\n",
		request->nodeID.volumeID, request->nodeID.nodeID, count,
		result, reply.entryInfos.CountElements());
	return GetChannel()->SendRequest(&reply);
}

// VisitOpenAttrDirRequest
status_t
ClientConnection::VisitOpenAttrDirRequest(OpenAttrDirRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	VolumeManagerLocker managerLocker;

	// get the node
	Node* node = NULL;
	if (result == B_OK) {
		node = volume->GetNode(request->nodeID);
		if (!node)
			result = B_ENTRY_NOT_FOUND;
	}

	// check permission
	if (result == B_OK) {
		if (!volume->GetNodePermissions(node).ImpliesReadPermission())
			result = B_PERMISSION_DENIED;
	}

	// load/cache the attribute directory
	bool attrDirCached = (node->LoadAttrDir() == B_OK);

	// open the attribute directory, if caching it failed
	AttrDirIterator* handle = NULL;
	if (result == B_OK && !attrDirCached)
		result = volume->OpenAttrDir(node, &handle);
	NodeHandleUnlocker handleUnlocker(volume, handle);

	// prepare the reply
	OpenAttrDirReply reply;
	if (result == B_OK) {
		if (handle) {
			reply.cookie = handle->GetCookie();
		} else {
			// the attribute directory is cached
			reply.cookie = -1;
			result = _GetAttrDirInfo(request, node, &reply.attrDirInfo);
		}
	}

	managerLocker.Unlock();

	// send the reply
	reply.error = result;
	status_t error = GetChannel()->SendRequest(&reply);

	// close the handle, if a send error occurred
	if (error != B_OK && result == B_OK && handle)
		volume->Close(handle);

	return error;
}

// VisitReadAttrDirRequest
status_t
ClientConnection::VisitReadAttrDirRequest(ReadAttrDirRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	// get the node handle
	NodeHandle* handle = NULL;
	if (result == B_OK)
		result = volume->LockNodeHandle(request->cookie, &handle);
	NodeHandleUnlocker handleUnlocker(volume, handle);

	// check if it is a attribute directory iterator
	AttrDirIterator* iterator = NULL;
	if (result == B_OK) {
		iterator = dynamic_cast<AttrDirIterator*>(handle);
		if (!iterator)
			result = B_BAD_VALUE;
	}

	VolumeManagerLocker managerLocker;

	// get the node
	Node* node = NULL;
	if (result == B_OK) {
		node = volume->GetNode(iterator->GetNodeRef());
		if (!node)
			result = B_ENTRY_NOT_FOUND;
	}

	// check read permission (we already checked when opening, but anyway...)
	if (result == B_OK) {
		if (!volume->GetNodePermissions(node).ImpliesReadPermission())
			result = B_PERMISSION_DENIED;
	}

	managerLocker.Unlock();

	// read the attribute directory
	uint8 buffer[sizeof(struct dirent) + B_FILE_NAME_LENGTH];
	struct dirent* dirEntry = (struct dirent*)buffer;
	int32 countRead = 0;
	bool done = true;
	if (result == B_OK) {
		if (request->rewind)
			result = iterator->RewindDir();
		if (result == B_OK) {
			result = iterator->ReadDir(dirEntry, sizeof(buffer), 1,
				&countRead, &done);
		}
	}

	// prepare the reply
	ReadAttrDirReply reply;
	reply.name.SetTo(dirEntry->d_name);

	// send the reply
	reply.error = result;
	reply.count = countRead;
	return GetChannel()->SendRequest(&reply);
}

// VisitReadAttrRequest
status_t
ClientConnection::VisitReadAttrRequest(ReadAttrRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	VolumeManagerLocker managerLocker;

	// get the node
	Node* node = NULL;
	if (result == B_OK) {
		node = volume->GetNode(request->nodeID);
		if (!node)
			result = B_ENTRY_NOT_FOUND;
	}

	// check read permission
	if (result == B_OK) {
		if (!volume->GetNodePermissions(node).ImpliesReadPermission())
			result = B_PERMISSION_DENIED;
	}

	// open the node
	FileHandle* handle = NULL;
	if (result == B_OK)
		result = volume->Open(node, O_RDONLY, &handle);
	NodeHandleUnlocker handleUnlocker(volume, handle);

	managerLocker.Unlock();

	// read the attribute
	if (result == B_OK) {
		// Due to a bug in BFS the `pos' is ignored. This means that the loop
		// below won't work if the attribute is non-empty and the buffer is
		// larger than the attribute, because the we would again and again
		// read the beginning of the attribute until the buffer is full.
		// Hence we first get an attr_info and don't try to read more than
		// the size of the attribute.
		attr_info info;
		result = handle->StatAttr(request->name.GetString(), &info);
		off_t originalPos = max(request->pos, 0LL);
		int32 originalSize = max(request->size, 0L);
		off_t pos = originalPos;
		int32 size = originalSize;
		type_code type = B_SWAP_INT32(request->type);
		bool convert = false;

		if (result == B_OK) {
			originalSize = min((off_t)originalSize, max(0LL, info.size - pos));
			size = originalSize;

			// deal with inverse endianess clients
			if (fInverseClientEndianess) {
				convert = _KnownAttributeType(info.type);
				if (convert) {
					// read the whole attribute
					pos = 0;
					size = info.size;
				} else
					type = B_SWAP_INT32(request->type);
			}
		}
		int32 bufferSize = min(size, kMaxReadBufferSize);

		// allocate a buffer
		uint8* buffer = NULL;
		if (result == B_OK) {
			buffer = (uint8*)malloc(bufferSize);
			if (!buffer)
				result = B_NO_MEMORY;
		}
		MemoryDeleter bufferDeleter(buffer);

		if (convert) {
			// read the whole attribute and convert it
			if (result == B_OK) {
				// read
				size_t bytesRead = 0;
				result = handle->ReadAttr(request->name.GetString(),
					type, 0, buffer, size, &bytesRead);
				if (result == B_OK && (int32)bytesRead != size)
					result = B_ERROR;

				// convert
				if (result == B_OK)
					_ConvertAttribute(info, buffer);
			}

			// prepare the reply
			ReadAttrReply reply;
			if (result == B_OK) {
				reply.pos = originalPos;
				reply.data.SetTo(buffer + originalPos, originalSize);
				reply.moreToCome = false;
			}

			// send the reply
			reply.error = result;
			status_t error = GetChannel()->SendRequest(&reply);
			if (error != B_OK)
				return error;
		} else {
			// read as long as there are bytes left to read or an error occurs
			bool moreToRead = true;
			do {
				int32 bytesToRead = min(size, bufferSize);
				size_t bytesRead = 0;
				if (result == B_OK) {
					result = handle->ReadAttr(request->name.GetString(),
						request->type, pos, buffer, bytesToRead, &bytesRead);
				}
				moreToRead = (result == B_OK && bytesRead > 0
					&& (int32)bytesRead < size);

				// prepare the reply
				ReadAttrReply reply;
				if (result == B_OK) {
					reply.pos = pos;
					reply.data.SetTo(buffer, bytesRead);
					reply.moreToCome = moreToRead;
					pos += bytesRead;
					size -= bytesRead;
				}

				// send the reply
				reply.error = result;
				status_t error = GetChannel()->SendRequest(&reply);
				if (error != B_OK)
					return error;
			} while (moreToRead);
		}

		// close the handle
		volume->Close(handle);
	} else {
		// opening the node failed (or something even earlier): send an error
		// reply
		ReadAttrReply reply;
		reply.error = result;
		status_t error = GetChannel()->SendRequest(&reply);
		if (error != B_OK)
			return error;
	}

	return B_OK;
}

// VisitWriteAttrRequest
status_t
ClientConnection::VisitWriteAttrRequest(WriteAttrRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	VolumeManagerLocker managerLocker;

	// get the node
	Node* node = NULL;
	if (result == B_OK) {
		node = volume->GetNode(request->nodeID);
		if (!node)
			result = B_ENTRY_NOT_FOUND;
	}

	// check read permission
	if (result == B_OK) {
		if (!volume->GetNodePermissions(node).ImpliesWritePermission())
			result = B_PERMISSION_DENIED;
	}

	// open the node
	FileHandle* handle = NULL;
	if (result == B_OK)
		result = volume->Open(node, O_RDWR, &handle);
	NodeHandleUnlocker handleUnlocker(volume, handle);

	managerLocker.Unlock();

	if (result == B_OK) {
		off_t pos = max(request->pos, 0LL);
		int32 size = request->data.GetSize();
		type_code type = request->type;
		char* buffer = (char*)request->data.GetData();

		// convert the data, if necessary
		if (fInverseClientEndianess) {
			if (_KnownAttributeType(type)) {
				if (pos != 0) {
					WARN("WriteAttr(): WARNING: Need to convert attribute "
						"endianess, but position is not 0: attribute: %s, "
						"pos: %lld, size: %ld\n", request->name.GetString(),
						pos, size);
				}
				swap_data(type, buffer, size, B_SWAP_ALWAYS);
			} else
				type = B_SWAP_INT32(type);
		}

		// write the data
		while (result == B_OK && size > 0) {
			size_t bytesWritten;
			result = handle->WriteAttr(request->name.GetString(),
				type, pos, buffer, size, &bytesWritten);
			if (result == B_OK) {
				pos += bytesWritten;
				buffer += bytesWritten;
				size -= bytesWritten;
			}
		}

		// close the handle
		volume->Close(handle);
	}

	// prepare the reply
	WriteAttrReply reply;
	// send the reply
	reply.error = result;
	return GetChannel()->SendRequest(&reply);
}

// VisitRemoveAttrRequest
status_t
ClientConnection::VisitRemoveAttrRequest(RemoveAttrRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	VolumeManagerLocker managerLocker;

	// get the node
	Node* node = NULL;
	if (result == B_OK) {
		node = volume->GetNode(request->nodeID);
		if (!node)
			result = B_ENTRY_NOT_FOUND;
	}

	// check read permission
	if (result == B_OK) {
		if (!volume->GetNodePermissions(node).ImpliesWritePermission())
			result = B_PERMISSION_DENIED;
	}

	// open the node
	FileHandle* handle = NULL;
	if (result == B_OK)
		result = volume->Open(node, O_RDWR, &handle);
	NodeHandleUnlocker handleUnlocker(volume, handle);

	managerLocker.Unlock();

	// remove the attribute and close the node
	if (result == B_OK) {
		result = handle->RemoveAttr(request->name.GetString());
		volume->Close(handle);
	}

	// send the reply
	RemoveAttrReply reply;
	reply.error = result;
	return GetChannel()->SendRequest(&reply);
}

// VisitRenameAttrRequest
status_t
ClientConnection::VisitRenameAttrRequest(RenameAttrRequest* request)
{
	// Not supported, since there's no API function to rename an attribute.
	// send the reply
	RemoveAttrReply reply;
	reply.error = B_UNSUPPORTED;
	return GetChannel()->SendRequest(&reply);
}

// VisitStatAttrRequest
status_t
ClientConnection::VisitStatAttrRequest(StatAttrRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// get the volume
	status_t result = B_OK;
	ClientVolume* volume = _GetVolume(request->volumeID);
	if (!volume)
		result = B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	VolumeManagerLocker managerLocker;

	// get the node
	Node* node = NULL;
	if (result == B_OK) {
		node = volume->GetNode(request->nodeID);
		if (!node)
			result = B_ENTRY_NOT_FOUND;
	}

	// check read permission
	if (result == B_OK) {
		if (!volume->GetNodePermissions(node).ImpliesReadPermission())
			result = B_PERMISSION_DENIED;
	}

	// open the node
	FileHandle* handle = NULL;
	if (result == B_OK)
		result = volume->Open(node, O_RDONLY, &handle);
	NodeHandleUnlocker handleUnlocker(volume, handle);

	managerLocker.Unlock();

	// stat the attribute and close the node
	attr_info attrInfo;
	StatAttrReply reply;
	if (result == B_OK) {
		result = handle->StatAttr(request->name.GetString(), &attrInfo);
		volume->Close(handle);
	}

	// set the attribute info
	if (result == B_OK) {
		result = _GetAttrInfo(request, request->name.GetString(), attrInfo,
			NULL, &reply.attrInfo);
	}

	// send the reply
	reply.error = result;
	return GetChannel()->SendRequest(&reply);
}

// VisitOpenQueryRequest
status_t
ClientConnection::VisitOpenQueryRequest(OpenQueryRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	VolumeManagerLocker managerLocker;

	// open the query
	status_t result = B_OK;
	QueryHandle* handle = NULL;
	if (result == B_OK) {
		result = _OpenQuery(request->queryString.GetString(),
			request->flags, request->port, request->token, &handle);
	}
	QueryHandleUnlocker handleUnlocker(this, handle);

	// prepare the reply
	OpenQueryReply reply;
	if (result == B_OK)
		reply.cookie = handle->GetCookie();

	managerLocker.Unlock();

	// send the reply
	reply.error = result;
	status_t error = GetChannel()->SendRequest(&reply);

	// close the handle, if a send error occurred
	if (error != B_OK && result == B_OK)
		_CloseQuery(handle);

	return error;
}

// VisitReadQueryRequest
status_t
ClientConnection::VisitReadQueryRequest(ReadQueryRequest* request)
{
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return B_OK;

	// create an array for the IDs of the client volumes a found entry may
	// reside on
	status_t result = B_OK;
	int32 volumeCount = fVolumes->Size();
	int32* volumeIDs = new(std::nothrow) int32[volumeCount];
	if (!volumeIDs)
		result = B_NO_MEMORY;
	ArrayDeleter<int32> volumeIDsDeleter(volumeIDs);

	// get the query handle
	QueryHandle* handle = NULL;
	if (result == B_OK)
		result = _LockQueryHandle(request->cookie, &handle);
	QueryHandleUnlocker handleUnlocker(this, handle);

	// check if it is a query handle
	QueryHandle* queryHandle = NULL;
	if (result == B_OK) {
		queryHandle = dynamic_cast<QueryHandle*>(handle);
		if (!queryHandle)
			result = B_BAD_VALUE;
	}

	// read the query
	ReadQueryReply reply;
	int32 countRead = 0;
	while (result == B_OK) {
		uint8 buffer[sizeof(struct dirent) + B_FILE_NAME_LENGTH];
		struct dirent* dirEntry = (struct dirent*)buffer;

		result = queryHandle->ReadDir(dirEntry, 1, &countRead);
		if (result != B_OK)
			break;
		if (countRead == 0)
			break;
		PRINT("  query entry: %ld, %lld, \"%s\"\n",
			dirEntry->d_pdev, dirEntry->d_pino, dirEntry->d_name);

		VolumeManagerLocker managerLocker;
		VolumeManager* volumeManager = VolumeManager::GetDefault();

		// load the entry
		Entry* entry = NULL;
		result = volumeManager->LoadEntry(dirEntry->d_pdev,
			dirEntry->d_pino, dirEntry->d_name, true, &entry);

		// if at least one client volume contains the entry, get an entry info
		if (result == B_OK) {
			HasQueryPermissionClientVolumeFilter filter;
			int32 entryVolumeCount = _GetContainingClientVolumes(
				entry->GetDirectory(), volumeIDs, volumeCount, &filter);
			if (entryVolumeCount > 0) {
				// store all the client volume IDs in the reply
				for (int32 i = 0; i < entryVolumeCount; i++) {
					result = reply.clientVolumeIDs.Append(volumeIDs[i]);
					if (result != B_OK)
						break;
				}

				// get an entry info
				_GetNodeInfo(entry->GetDirectory(), &reply.dirInfo);
				_GetEntryInfo(entry, &reply.entryInfo);
				break;
			}
else
PRINT(("  -> no client volumes\n"));
		}

		// entry is not in the volume: next round...
		result = B_OK;
	}

	// send the reply
	reply.error = result;
	reply.count = countRead;
	PRINT("ReadQuery: (%lx, %ld, dir: (%ld, %lld), node: (%ld, %lld, `%s')"
		"\n", reply.error, reply.count,
		reply.entryInfo.directoryID.volumeID,
		reply.entryInfo.directoryID.nodeID,
		reply.entryInfo.nodeInfo.st.st_dev,
		reply.entryInfo.nodeInfo.st.st_ino,
		reply.entryInfo.name.GetString());
	return GetChannel()->SendRequest(&reply);
}


// #pragma mark -

// ProcessNodeMonitoringEvent
void
ClientConnection::ProcessNodeMonitoringEvent(int32 volumeID,
	NodeMonitoringEvent* event)
{
	// get a connection reference
	ConnectionReference connectionReference(this);
	if (!connectionReference.IsValid())
		return;

	_PushNodeMonitoringEvent(volumeID, event);
}

// CloseNodeMonitoringEventQueue
void
ClientConnection::CloseNodeMonitoringEventQueue()
{
	typedef Vector<NodeMonitoringRequest*> RequestVector;
	const RequestVector* requests;
	if (fNodeMonitoringEvents->Close(false, &requests) == B_OK) {
		for (RequestVector::ConstIterator it = requests->Begin();
			 it != requests->End();
			 it++) {
			delete *it;
		}
	}
}


// #pragma mark -

// QueryDomainIntersectsWith
bool
ClientConnection::QueryDomainIntersectsWith(Volume* volume)
{
	// Iterate through the the client volumes and check whether any one contains
	// the supplied volume or its root dir is on the volume. We don't check
	// directory inclusion for the latter, since we don't need to query the
	// volume, if the client volume is located on a volume mounted somewhere
	// under the supplied volume (e.g. the root FS contains everything, but does
	// seldomly need to be queried).
	VolumeManager* volumeManager = VolumeManager::GetDefault();
	AutoLocker<VolumeMap> volumesLocker(fVolumes);
	for (VolumeMap::Iterator it = fVolumes->GetIterator(); it.HasNext();) {
		ClientVolume* clientVolume = it.Next().value;
		Directory* volumeRoot = volume->GetRootDirectory();
		Directory* clientVolumeRoot = clientVolume->GetRootDirectory();
		if (volumeManager->DirectoryContains(clientVolumeRoot, volumeRoot, true)
			|| volumeRoot->GetVolumeID() == clientVolumeRoot->GetVolumeID()) {
			return true;
		}
	}

	return false;
}

// ProcessQueryEvent
void
ClientConnection::ProcessQueryEvent(NodeMonitoringEvent* event)
{
	dev_t volumeID;
	ino_t directoryID;
	if (event->opcode == B_ENTRY_CREATED) {
		// "entry created" event
		EntryCreatedEvent* createdEvent
			= dynamic_cast<EntryCreatedEvent*>(event);
		if (!createdEvent)
			return;
		volumeID = createdEvent->volumeID;
		directoryID = createdEvent->directoryID;

	} else if (event->opcode == B_ENTRY_REMOVED) {
		// "entry removed" event
		EntryRemovedEvent* removedEvent
			= dynamic_cast<EntryRemovedEvent*>(event);
		if (!removedEvent)
			return;
		volumeID = removedEvent->volumeID;
		directoryID = removedEvent->directoryID;

	} else {
		// We only support "entry created" and "entry removed" query events.
		// "entry moved" is split by the volume manager into those.
		ERROR("Ignoring unexpected query event: opcode: 0x%lx\n",
			event->opcode);
		return;
	}
	PRINT("ClientConnection::ProcessQueryEvent(): event: %p, type: %s:"
		" directory: (%ld, %lld)\n", event, typeid(event).name(),
		volumeID, directoryID);

	// create an array for the IDs of the client volumes a found entry may
	// reside on
	status_t result = B_OK;
	int32 volumeCount = fVolumes->Size();
	int32* volumeIDs = new(std::nothrow) int32[volumeCount];
	if (!volumeIDs)
		result = B_NO_MEMORY;
	ArrayDeleter<int32> volumeIDsDeleter(volumeIDs);

	HasQueryPermissionClientVolumeFilter filter;

	// load the directory the concerned entry belongs/belonged to
	Directory* directory;
	int32 concernedVolumes = 0;
	if (VolumeManager::GetDefault()->LoadDirectory(volumeID, directoryID,
			&directory) == B_OK) {
		// find out, which client volumes the directory is located in
		concernedVolumes = _GetContainingClientVolumes(directory, volumeIDs,
			volumeCount, &filter);
	} else {
		// Failed to load the directory, so maybe it has already been
		// deleted. For "entry removed" events, we consider all client
		// volumes to be notified -- those that don't know the entry will
		// ignore the event.
		if (event->opcode == B_ENTRY_REMOVED) {
			concernedVolumes = _GetAllClientVolumeIDs(volumeIDs, volumeCount,
				&filter);
		}
	}

	// now push the event for each concerned client volume
	for (int32 i = 0; i < concernedVolumes; i++)
		_PushNodeMonitoringEvent(volumeIDs[i], event);
	// TODO: More than one volume will usually only be concerned in case of
	// nested client volumes. We could optimize the case by having an array of
	// volume IDs in the respective requests sent over the net (just as in the
	// ReadQueryReply).
}


// #pragma mark -

// _Close
void
ClientConnection::_Close()
{
	// terminate node monitoring processor
	CloseNodeMonitoringEventQueue();
	if (fNodeMonitoringProcessor >= 0
		&& find_thread(NULL) != fNodeMonitoringProcessor) {
		int32 result;
		wait_for_thread(fNodeMonitoringProcessor, &result);
		// The variable is not unset, when this is the node monitoring
		// processor thread -- which is good, since the destructor will
		// wait for the thread in this case.
		fNodeMonitoringProcessor = -1;
	}
	if (fConnection)
		fConnection->Close();
	// notify the listener
	ClientConnectionListener* listener = fListener;
	fListener = NULL;
	if (listener)
		listener->ClientConnectionClosed(this, fError);
}

// _MarkClosed
void
ClientConnection::_MarkClosed(bool error)
{
	AutoLocker<Locker> _(fLock);
	if (!fClosed) {
		fClosed = true;
		fError = error;
	}
}

// _GetNodeInfo
void
ClientConnection::_GetNodeInfo(Node* node, NodeInfo* info)
{
	if (node && info) {
		info->st = node->GetStat();
		info->revision = VolumeManager::GetDefault()->GetRevision();
	}
}

// _GetEntryInfo
void
ClientConnection::_GetEntryInfo(Entry* entry, EntryInfo* info)
{
	if (entry && info) {
		info->directoryID.volumeID = entry->GetVolumeID();
		info->directoryID.nodeID = entry->GetDirectoryID();
		info->name.SetTo(entry->GetName());
		_GetNodeInfo(entry->GetNode(), &info->nodeInfo);
	}
}

// _GetAttrInfo
status_t
ClientConnection::_GetAttrInfo(Request* request, const char* name,
	const attr_info& attrInfo, const void* data, AttributeInfo* info)
{
	if (!request || !name || !info)
		return B_BAD_VALUE;

	info->name.SetTo(name);
	info->info = attrInfo;
	data = (attrInfo.size > 0 ? data : NULL);
	int32 dataSize = (data ? attrInfo.size : 0);
	info->data.SetTo(data, dataSize);

	// if the client has inverse endianess, swap the type, if we don't know it
	if (fInverseClientEndianess) {
		if (_KnownAttributeType(info->info.type)) {
			// we need to convert the data, if supplied
			if (data) {
				// allocate a buffer
				RequestBuffer* requestBuffer = RequestBuffer::Create(dataSize);
				if (!requestBuffer)
					return B_NO_MEMORY;

				// convert the data
				memcpy(requestBuffer->GetData(), data, dataSize);
				_ConvertAttribute(info->info, requestBuffer->GetData());
			}
		} else
			info->info.type = B_SWAP_INT32(info->info.type);
	}

	return B_OK;
}

// _GetAttrDirInfo
status_t
ClientConnection::_GetAttrDirInfo(Request* request, AttributeDirectory* attrDir,
	AttrDirInfo* info)
{
	if (!request || !attrDir || !info || !attrDir->IsAttrDirValid())
		return B_BAD_VALUE;

	// add the attribute infos
	for (Attribute* attribute = attrDir->GetFirstAttribute();
		 attribute;
		 attribute = attrDir->GetNextAttribute(attribute)) {
		// get the attribute info
		AttributeInfo attrInfo;
		attr_info bAttrInfo;
		attribute->GetInfo(&bAttrInfo);
		status_t error = _GetAttrInfo(request, attribute->GetName(), bAttrInfo,
			attribute->GetData(), &attrInfo);

		// append it
		if (error == B_OK)
			error = info->attributeInfos.Append(attrInfo);
		if (error != B_OK)
			return error;
	}

	info->revision = VolumeManager::GetDefault()->GetRevision();
	info->isValid = true;

	return B_OK;
}

// _CreateVolume
status_t
ClientConnection::_CreateVolume(ClientVolume** _volume)
{
	// create and init the volume
	ClientVolume* volume = new(std::nothrow) ClientVolume(fSecurityContextLock,
		this);
	if (!volume)
		return B_NO_MEMORY;
	status_t error = volume->Init();
	if (error != B_OK) {
		delete volume;
		return error;
	}

	// add it to the volume map
	AutoLocker<VolumeMap> locker(fVolumes);
	error = fVolumes->Put(volume->GetID(), volume);
	locker.Unlock();

	if (error == B_OK)
		*_volume = volume;
	else
		delete volume;

	return error;
}

// _GetVolume
ClientVolume*
ClientConnection::_GetVolume(int32 id)
{
	AutoLocker<VolumeMap> _(fVolumes);
	ClientVolume* volume = fVolumes->Get(id);
	if (!volume || volume->IsRemoved())
		return NULL;
	volume->AcquireReference();
	return volume;
}

// _PutVolume
//
// The VolumeManager may be locked, but no other lock must be held.
void
ClientConnection::_PutVolume(ClientVolume* volume)
{
	if (!volume)
		return;

	// decrement reference counter and remove the volume, if 0
	AutoLocker<VolumeMap> locker(fVolumes);
	bool removed = (volume->ReleaseReference() == 1 && volume->IsRemoved());
	if (removed)
		fVolumes->Remove(volume->GetID());
	locker.Unlock();

	if (removed) {
		VolumeManagerLocker managerLocker;
		delete volume;
	}
}

// _UnmountVolume
//
// The caller must have a reference to the volume.
void
ClientConnection::_UnmountVolume(ClientVolume* volume)
{
	if (!volume)
		return;
	AutoLocker<VolumeMap> locker(fVolumes);
	volume->MarkRemoved();
	locker.Unlock();

	// push a notification event
	if (VolumeUnmountedEvent* event = new(std::nothrow) VolumeUnmountedEvent) {
		VolumeManagerLocker managerLocker;

		event->opcode = B_DEVICE_UNMOUNTED;
		_PushNodeMonitoringEvent(volume->GetID(), event);
		event->ReleaseReference();
	}
}

// _UnmountAllVolumes
void
ClientConnection::_UnmountAllVolumes()
{
	while (true) {
		// To avoid heap allocation (which can fail) we unmount the volumes
		// chunkwise.
		// get the volumes
		const int32 volumeChunkSize = 32;
		ClientVolume* volumes[volumeChunkSize];
		int32 volumeCount = 0;
		AutoLocker<VolumeMap> volumesLocker(fVolumes);
		for (VolumeMap::Iterator it = fVolumes->GetIterator(); it.HasNext();) {
			if (ClientVolume* volume = _GetVolume(it.Next().value->GetID())) {
				volumes[volumeCount++] = volume;
			}
			if (volumeCount == volumeChunkSize)
				break;
		}
		volumesLocker.Unlock();

		// unmount and put the volumes
		for (int32 i = 0; i < volumeCount; i++) {
			ClientVolume* volume = volumes[i];
			_UnmountVolume(volume);
			_PutVolume(volume);
		}

		if (volumeCount < volumeChunkSize)
			break;
	}
}

// _NodeMonitoringProcessorEntry
int32
ClientConnection::_NodeMonitoringProcessorEntry(void* data)
{
	return ((ClientConnection*)data)->_NodeMonitoringProcessor();
}

// _NodeMonitoringProcessor
int32
ClientConnection::_NodeMonitoringProcessor()
{
	while (!fClosed) {
		// get the next request
		NodeMonitoringRequest* request;
		status_t error = fNodeMonitoringEvents->Pop(&request);

		// get a client connection reference
		ConnectionReference connectionReference(this);
		if (!connectionReference.IsValid())
			return B_OK;

		// No request? Next round...
		if (error != B_OK)
			continue;
		ObjectDeleter<NodeMonitoringRequest> requestDeleter(request);

		// send the request
		if (error == B_OK) {
			error = fConnection->SendRequest(request);
			if (error != B_OK) {
				ERROR(("ClientConnection::_NodeMonitoringProcessor(): "
					"Failed to send request.\n"));
			}
		}
	}
	return 0;
}

// _PushNodeMonitoringEvent
//
// The caller must have a connection reference. Moreover the VolumeManager
// must be locked.
status_t
ClientConnection::_PushNodeMonitoringEvent(int32 volumeID,
	NodeMonitoringEvent* event)
{
	if (!event)
		return B_BAD_VALUE;

	// get the volume
	ClientVolume* volume = _GetVolume(volumeID);
	if (!volume && event->opcode != B_DEVICE_UNMOUNTED)
		return B_BAD_VALUE;
	ClientVolumePutter volumePutter(this, volume);

	// create a node monitoring request
	NodeMonitoringRequest* request = NULL;
	status_t error = B_ERROR;
	switch (event->opcode) {
		case B_ENTRY_CREATED:
			error = _EntryCreated(volume,
				dynamic_cast<EntryCreatedEvent*>(event), request);
			break;
		case B_ENTRY_REMOVED:
			error = _EntryRemoved(volume,
				dynamic_cast<EntryRemovedEvent*>(event), request);
			break;
		case B_ENTRY_MOVED:
			error = _EntryMoved(volume,
				dynamic_cast<EntryMovedEvent*>(event), request);
			break;
		case B_STAT_CHANGED:
			error = _NodeStatChanged(volume,
				dynamic_cast<StatChangedEvent*>(event), request);
			break;
		case B_ATTR_CHANGED:
			error = _NodeAttributeChanged(volume,
				dynamic_cast<AttributeChangedEvent*>(event), request);
			break;
		case B_DEVICE_UNMOUNTED:
			error = B_OK;
			break;
	}

	// replace all data buffers -- when the request is actually sent, they
	// might no longer exist
	if (error == B_OK)
		error = RequestBufferReplacer().ReplaceBuffer(request);

	if (error == B_OK) {
		// common initialization
		request->volumeID = volumeID;
		request->opcode = event->opcode;
		request->revision = VolumeManager::GetDefault()->GetRevision();

		// push the request
		error = fNodeMonitoringEvents->Push(request);
		if (error != B_OK)
			delete request;
	}

	return error;
}

// _EntryCreated
status_t
ClientConnection::_EntryCreated(ClientVolume* volume, EntryCreatedEvent* event,
	NodeMonitoringRequest*& _request)
{
	// allocate the request
	EntryCreatedRequest* request = new(std::nothrow) EntryCreatedRequest;
	if (!request)
		return B_NO_MEMORY;
	ObjectDeleter<NodeMonitoringRequest> requestDeleter(request);

	// get the name
	const char* name = event->name.GetString();

	// set the request fields
	request->directoryID = NodeID(event->volumeID, event->directoryID);
	request->nodeID = NodeID(event->volumeID, event->nodeID);
	request->name.SetTo(name);
	if (event->queryHandler) {
		request->port = event->remotePort;
		request->token = event->remoteToken;
		request->queryUpdate = true;
	} else
		request->queryUpdate = false;

	// try to get an entry info
	Entry* entry;
	if (VolumeManager::GetDefault()->LoadEntry(event->volumeID,
			event->directoryID, name, true, &entry) == B_OK
		&& entry->GetNode()->GetVolumeID() == event->volumeID
		&& entry->GetNode()->GetID() == event->nodeID) {
		_GetEntryInfo(entry, &request->entryInfo);
		request->entryInfoValid = true;
	} else
		request->entryInfoValid = false;

	requestDeleter.Detach();
	_request = request;
	return B_OK;
}

// _EntryRemoved
status_t
ClientConnection::_EntryRemoved(ClientVolume* volume, EntryRemovedEvent* event,
	NodeMonitoringRequest*& _request)
{
	// special handling, if it is the root node of the client volume that has
	// been removed
	if (!event->queryHandler
		&& NodeRef(event->nodeVolumeID, event->nodeID)
			== volume->GetRootNodeRef()) {
		NoAllocEntryRef ref(event->nodeVolumeID, event->nodeID, ".");
		BEntry entry;
		if (FDManager::SetEntry(&entry, &ref) != B_OK || !entry.Exists())
			_UnmountVolume(volume);

		// don't send the "entry removed" event
		return B_ERROR;
	}

	// allocate the request
	EntryRemovedRequest* request = new(std::nothrow) EntryRemovedRequest;
	if (!request)
		return B_NO_MEMORY;
	ObjectDeleter<NodeMonitoringRequest> requestDeleter(request);

	// get the name
	const char* name = event->name.GetString();

	// set the request fields
	request->directoryID = NodeID(event->volumeID, event->directoryID);
	request->nodeID = NodeID(event->nodeVolumeID, event->nodeID);
	request->name.SetTo(name);
	if (event->queryHandler) {
		request->port = event->remotePort;
		request->token = event->remoteToken;
		request->queryUpdate = true;
	} else
		request->queryUpdate = false;

	requestDeleter.Detach();
	_request = request;
	return B_OK;
}

// _EntryMoved
status_t
ClientConnection::_EntryMoved(ClientVolume* volume, EntryMovedEvent* event,
	NodeMonitoringRequest*& _request)
{
	// allocate the request
	EntryMovedRequest* request = new(std::nothrow) EntryMovedRequest;
	if (!request)
		return B_NO_MEMORY;
	ObjectDeleter<NodeMonitoringRequest> requestDeleter(request);

	// allocate memory for the names
	int32 fromNameLen = event->fromName.GetLength();
	const char* fromName
		= (fromNameLen > 0 ? event->fromName.GetString() : NULL);
	const char* toName = event->toName.GetString();

	// set the request fields
	request->fromDirectoryID = NodeID(event->volumeID, event->fromDirectoryID);
	request->toDirectoryID = NodeID(event->volumeID, event->toDirectoryID);
	request->nodeID = NodeID(event->nodeVolumeID, event->nodeID);
	request->fromName.SetTo(fromName);
	request->toName.SetTo(toName);
	request->queryUpdate = false;

	// try to get an entry info
	Entry* entry;
	if (VolumeManager::GetDefault()->LoadEntry(event->volumeID,
			event->toDirectoryID, toName, true, &entry) == B_OK
		&& entry->GetNode()->GetVolumeID() == event->nodeVolumeID
		&& entry->GetNode()->GetID() == event->nodeID) {
		_GetEntryInfo(entry, &request->entryInfo);
		request->entryInfoValid = true;
	} else
		request->entryInfoValid = false;

	requestDeleter.Detach();
	_request = request;
	return B_OK;
}

// _NodeStatChanged
status_t
ClientConnection::_NodeStatChanged(ClientVolume* volume,
	StatChangedEvent* event, NodeMonitoringRequest*& _request)
{
	// get the node
	Node* node = volume->GetNode(event->volumeID, event->nodeID);
	if (!node)
		return B_ENTRY_NOT_FOUND;

	// allocate the request
	StatChangedRequest* request = new(std::nothrow) StatChangedRequest;
	if (!request)
		return B_NO_MEMORY;
	ObjectDeleter<NodeMonitoringRequest> requestDeleter(request);

	// set the request fields
	request->nodeID = NodeID(event->volumeID, event->nodeID);
	_GetNodeInfo(node, &request->nodeInfo);
	request->queryUpdate = false;

	requestDeleter.Detach();
	_request = request;
	return B_OK;
}

// _NodeAttributeChanged
status_t
ClientConnection::_NodeAttributeChanged(ClientVolume* volume,
	AttributeChangedEvent* event, NodeMonitoringRequest*& _request)
{
	// get the node
	Node* node = volume->GetNode(event->volumeID, event->nodeID);
	if (!node)
		return B_ENTRY_NOT_FOUND;

	// update the attribute directory
	bool removed = false;
	bool valid = false;
	attr_info info;
	const void* data = NULL;
	status_t error = node->UpdateAttribute(event->attribute.GetString(),
		&removed, &info, &data);
	valid = (error == B_OK);

	// allocate the request
	AttributeChangedRequest* request = new(std::nothrow) AttributeChangedRequest;
	if (!request)
		return B_NO_MEMORY;
	ObjectDeleter<NodeMonitoringRequest> requestDeleter(request);

	// get an attr dir info, if the directory is valid
	if (node->IsAttrDirValid()) {
		status_t error = _GetAttrDirInfo(request, node, &request->attrDirInfo);
		if (error != B_OK)
			return error;
	}

	// get name and the data size
	int32 dataSize = (data ? info.size : 0);
	const char* name = event->attribute.GetString();

	// set the request fields
	request->nodeID = NodeID(event->volumeID, event->nodeID);
	request->attrInfo.name.SetTo(name);
	request->valid = valid;
	request->removed = removed;
	if (!removed && valid) {
		request->attrInfo.info = info;
		request->attrInfo.data.SetTo(data, dataSize);
	}
	request->queryUpdate = false;

	requestDeleter.Detach();
	_request = request;
	return B_OK;
}

// _KnownAttributeType
bool
ClientConnection::_KnownAttributeType(type_code type)
{
	if (!fInverseClientEndianess)
		return false;

	switch (type) {
		case B_BOOL_TYPE:
		case B_CHAR_TYPE:
		case B_COLOR_8_BIT_TYPE:
		case B_DOUBLE_TYPE:
		case B_FLOAT_TYPE:
		case B_GRAYSCALE_8_BIT_TYPE:
		case B_INT64_TYPE:
		case B_INT32_TYPE:
		case B_INT16_TYPE:
		case B_INT8_TYPE:
		case B_MESSAGE_TYPE:
		case B_MESSENGER_TYPE:
		case B_MIME_TYPE:
		case B_MONOCHROME_1_BIT_TYPE:
		case B_OFF_T_TYPE:
		case B_POINTER_TYPE:
		case B_POINT_TYPE:
		case B_RECT_TYPE:
		case B_REF_TYPE:
		case B_RGB_COLOR_TYPE:
		case B_SIZE_T_TYPE:
		case B_SSIZE_T_TYPE:
		case B_STRING_TYPE:
		case B_TIME_TYPE:
		case B_UINT64_TYPE:
		case B_UINT32_TYPE:
		case B_UINT16_TYPE:
		case B_UINT8_TYPE:
		case B_ASCII_TYPE:
		case B_MIME_STRING_TYPE:
			return true;

		//B_RGB_32_BIT_TYPE: We could translate it, but it's heavy...
	}

	return false;
}

// _ConvertAttribute
void
ClientConnection::_ConvertAttribute(const attr_info& info, void* buffer)
{
	swap_data(info.type, buffer, info.size, B_SWAP_ALWAYS);
}


// #pragma mark -

// _OpenQuery
status_t
ClientConnection::_OpenQuery(const char* queryString, uint32 flags,
	port_id remotePort, int32 remoteToken, QueryHandle** _handle)
{
	if (!queryString || !_handle)
		return B_BAD_VALUE;

	// open query
	QueryHandle* queryHandle;
	status_t error = VolumeManager::GetDefault()->OpenQuery(this, queryString,
		flags, remotePort, remoteToken, &queryHandle);
	if (error != B_OK)
		return error;
	BReference<QueryHandle> handleReference(queryHandle, true);

	// lock the handle
	queryHandle->Lock();

	// add the handle
	error = fQueryHandles->AddNodeHandle(queryHandle);
	if (error != B_OK)
		return error;

	handleReference.Detach();
	*_handle = queryHandle;
	return B_OK;
}

// _CloseQuery
status_t
ClientConnection::_CloseQuery(QueryHandle* handle)
{
	if (!handle || !fQueryHandles->RemoveNodeHandle(handle))
		return B_BAD_VALUE;

	return B_OK;
}

// _LockQueryHandle
//
// VolumeManager must NOT be locked.
status_t
ClientConnection::_LockQueryHandle(int32 cookie, QueryHandle** _handle)
{
	NodeHandle* handle;
	status_t error = fQueryHandles->LockNodeHandle(cookie, &handle);
	if (error == B_OK)
		*_handle = static_cast<QueryHandle*>(handle);
	return error;
}

// _UnlockQueryHandle
//
// VolumeManager may or may not be locked.
void
ClientConnection::_UnlockQueryHandle(NodeHandle* nodeHandle)
{
	fQueryHandles->UnlockNodeHandle(nodeHandle);
}


// #pragma mark -

// _GetAllClientVolumeIDs
int32
ClientConnection::_GetAllClientVolumeIDs(int32* volumeIDs, int32 arraySize,
	ClientVolumeFilter* filter)
{
	int32 count = 0;
	AutoLocker<VolumeMap> volumesLocker(fVolumes);
	for (VolumeMap::Iterator it = fVolumes->GetIterator();
		 it.HasNext() && arraySize > count;) {
		ClientVolume* clientVolume = it.Next().value;
		if (!filter || filter->FilterVolume(this, clientVolume))
			volumeIDs[count++] = clientVolume->GetID();
	}

	return count;
}

// _GetContainingClientVolumes
int32
ClientConnection::_GetContainingClientVolumes(Directory* directory,
	int32* volumeIDs, int32 arraySize, ClientVolumeFilter* filter)
{
	int32 count = 0;
	VolumeManager* volumeManager = VolumeManager::GetDefault();
	AutoLocker<VolumeMap> volumesLocker(fVolumes);
	for (VolumeMap::Iterator it = fVolumes->GetIterator();
		 it.HasNext() && arraySize > count;) {
		ClientVolume* clientVolume = it.Next().value;
		Directory* clientVolumeRoot = clientVolume->GetRootDirectory();
		if (volumeManager->DirectoryContains(clientVolumeRoot, directory, true)
			&& (!filter || filter->FilterVolume(this, clientVolume))) {
			volumeIDs[count++] = clientVolume->GetID();
		}
	}

	return count;
}


// #pragma mark -
// #pragma mark ----- ClientConnectionListener -----

// constructor
ClientConnectionListener::ClientConnectionListener()
{
}

// destructor
ClientConnectionListener::~ClientConnectionListener()
{
}

