// ShareVolume.cpp

#include "ShareVolume.h"

#include <new>
#include <unistd.h>

#include <AppDefs.h>	// for B_QUERY_UPDATE
#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <HashMap.h>

#include "AuthenticationServer.h"
#include "Compatibility.h"
#include "Connection.h"
#include "ConnectionFactory.h"
#include "DebugSupport.h"
#include "ExtendedServerInfo.h"
#include "Permissions.h"
#include "Node.h"
#include "QueryManager.h"
#include "RequestChannel.h"
#include "RequestConnection.h"
#include "Requests.h"
#include "RootVolume.h"
#include "SendReceiveRequest.h"
#include "ServerConnection.h"
#include "ServerConnectionProvider.h"
#include "ShareAttrDir.h"
#include "ShareAttrDirIterator.h"
#include "ShareNode.h"
#include "Utils.h"
#include "VolumeManager.h"
#include "VolumeSupport.h"

// TODO: Request timeouts!

static const int32 kMaxWriteBufferSize	= 64 * 1024;	// 64 KB
static const int32 kUserBufferSize = 256;
static const int32 kPasswordBufferSize = 256;

// connection states
enum {
	CONNECTION_NOT_INITIALIZED,
	CONNECTION_READY,
	CONNECTION_CLOSED,
};

// NodeMap
struct ShareVolume::NodeMap : HashMap<HashKey64<ino_t>, ShareNode*> {
};

// EntryKey
//
// NOTE: This class doesn't make a copy of the name string it is constructed
// with. So, when entering the key in a map, one must make sure, that the
// string stays valid as long as the entry is in the map.
struct ShareVolume::EntryKey {
	EntryKey() {}

	EntryKey(ino_t directoryID, const char* name)
		: directoryID(directoryID),
		  name(name)
	{
	}

	EntryKey(const EntryKey& other)
		: directoryID(other.directoryID),
		  name(other.name)
	{
	}

	uint32 GetHashCode() const
	{
		uint32 hash = (uint32)directoryID;
		hash = 31 * hash + (uint32)(directoryID >> 32);
		hash = 31 * hash + string_hash(name);
		return hash;
	}

	EntryKey& operator=(const EntryKey& other)
	{
		directoryID = other.directoryID;
		name = other.name;
		return *this;
	}

	bool operator==(const EntryKey& other) const
	{
		if (directoryID != other.directoryID)
			return false;

		if (name)
			return (other.name && strcmp(name, other.name) == 0);

		return !other.name;
	}

	bool operator!=(const EntryKey& other) const
	{
		return !(*this == other);
	}

	ino_t		directoryID;
	const char*	name;
};

// EntryMap
struct ShareVolume::EntryMap : HashMap<EntryKey, ShareDirEntry*> {
};

// LocalNodeIDMap
struct ShareVolume::LocalNodeIDMap : HashMap<NodeID, ino_t> {
};

// RemoteNodeIDMap
struct ShareVolume::RemoteNodeIDMap : HashMap<HashKey64<ino_t>, NodeID> {
};

// DirCookie
struct ShareVolume::DirCookie {
	ShareDirIterator*	iterator;
};

// AttrDirCookie
struct ShareVolume::AttrDirCookie {
	AttrDirCookie()
		: iterator(NULL),
		  cookie(-1),
		  rewind(false)
	{
	}

	ShareAttrDirIterator*	iterator;
	intptr_t				cookie;
	bool					rewind;
};

// AttrDirIteratorMap
struct ShareVolume::AttrDirIteratorMap
	: HashMap<HashKey64<ino_t>, DoublyLinkedList<ShareAttrDirIterator>*> {
};


// #pragma mark -

// constructor
ShareVolume::ShareVolume(VolumeManager* volumeManager,
	ServerConnectionProvider* connectionProvider,
	ExtendedServerInfo* serverInfo, ExtendedShareInfo* shareInfo)
	: Volume(volumeManager),
	  fID(-1),
	  fFlags(0),
	  fMountLock("share mount lock"),
	  fNodes(NULL),
	  fEntries(NULL),
	  fAttrDirIterators(NULL),
	  fLocalNodeIDs(NULL),
	  fRemoteNodeIDs(NULL),
	  fServerConnectionProvider(connectionProvider),
	  fServerInfo(serverInfo),
	  fShareInfo(shareInfo),
	  fServerConnection(NULL),
	  fConnection(NULL),
	  fSharePermissions(0),
	  fConnectionState(CONNECTION_NOT_INITIALIZED)
{
	fFlags = fVolumeManager->GetMountFlags();
	if (fServerConnectionProvider)
		fServerConnectionProvider->AcquireReference();
	if (fServerInfo)
		fServerInfo->AcquireReference();
	if (fShareInfo)
		fShareInfo->AcquireReference();
}

// destructor
ShareVolume::~ShareVolume()
{
PRINT(("ShareVolume::~ShareVolume()\n"));
	// delete the root node
	if (fRootNode) {
		if (fNodes)
			fNodes->Remove(fRootNode->GetID());
		delete fRootNode;
		fRootNode = NULL;
	}

	// delete the nodes
	if (fNodes) {
		// there shouldn't be any more nodes
		if (fNodes->Size() > 0) {
			WARN("ShareVolume::~ShareVolume(): WARNING: There are still "
				"%" B_PRId32 " nodes\n", fNodes->Size());
		}
		for (NodeMap::Iterator it = fNodes->GetIterator(); it.HasNext();)
			delete it.Next().value;
		delete fNodes;
	}

	// delete the entries
	if (fEntries) {
		// there shouldn't be any more entries
		if (fEntries->Size() > 0) {
			WARN("ShareVolume::~ShareVolume(): WARNING: There are still "
				"%" B_PRId32 " entries\n", fEntries->Size());
		}
		for (EntryMap::Iterator it = fEntries->GetIterator(); it.HasNext();)
			delete it.Next().value;
		delete fEntries;
	}

	delete fLocalNodeIDs;
	delete fRemoteNodeIDs;

	if (fShareInfo)
		fShareInfo->ReleaseReference();
	if (fServerInfo)
		fServerInfo->ReleaseReference();
	if (fServerConnection)
		fServerConnection->ReleaseReference();
	if (fServerConnectionProvider)
		fServerConnectionProvider->ReleaseReference();
}

// GetID
nspace_id
ShareVolume::GetID() const
{
	return fID;
}

// IsReadOnly
bool
ShareVolume::IsReadOnly() const
{
	return (fFlags & B_MOUNT_READ_ONLY);
}

// SupportsQueries
bool
ShareVolume::SupportsQueries() const
{
	return (fSharePermissions & QUERY_SHARE_PERMISSION);
}

// Init
status_t
ShareVolume::Init(const char* name)
{
	status_t error = Volume::Init(name);
	if (error != B_OK)
		return error;

	// create node map
	fNodes = new(std::nothrow) NodeMap;
	if (!fNodes)
		RETURN_ERROR(B_NO_MEMORY);
	error = fNodes->InitCheck();
	if (error != B_OK)
		return error;

	// create entry map
	fEntries = new(std::nothrow) EntryMap;
	if (!fEntries)
		RETURN_ERROR(B_NO_MEMORY);
	error = fEntries->InitCheck();
	if (error != B_OK)
		return error;

	// create attribute iterator map
	fAttrDirIterators = new(std::nothrow) AttrDirIteratorMap;
	if (!fAttrDirIterators)
		RETURN_ERROR(B_NO_MEMORY);
	error = fAttrDirIterators->InitCheck();
	if (error != B_OK)
		return error;

	// create local node ID map
	fLocalNodeIDs = new(std::nothrow) LocalNodeIDMap;
	if (!fLocalNodeIDs)
		RETURN_ERROR(B_NO_MEMORY);
	error = fLocalNodeIDs->InitCheck();
	if (error != B_OK)
		return error;

	// create remote node ID map
	fRemoteNodeIDs = new(std::nothrow) RemoteNodeIDMap;
	if (!fRemoteNodeIDs)
		RETURN_ERROR(B_NO_MEMORY);
	error = fRemoteNodeIDs->InitCheck();
	if (error != B_OK)
		return error;

	// get a local node ID for our root node
	vnode_id localID = fVolumeManager->NewNodeID(this);
	if (localID < 0)
		return localID;

	// create the root node
	fRootNode = new(std::nothrow) ShareDir(this, localID, NULL);
	if (!fRootNode) {
		fVolumeManager->RemoveNodeID(localID);
		return B_NO_MEMORY;
	}

	// add the root node to the node map
	error = fNodes->Put(localID, fRootNode);
	if (error != B_OK)
		return error;

	return B_OK;
}

// Uninit
void
ShareVolume::Uninit()
{
	Volume::Uninit();
}

// GetRootNode
Node*
ShareVolume::GetRootNode() const
{
	return fRootNode;
}

// PrepareToUnmount
void
ShareVolume::PrepareToUnmount()
{
PRINT(("ShareVolume::PrepareToUnmount()\n"));
	Volume::PrepareToUnmount();

	ConnectionClosed();

	AutoLocker<Locker> locker(fLock);

	// remove all entries and send respective "entry removed" events
	for (EntryMap::Iterator it = fEntries->GetIterator(); it.HasNext();) {
		ShareDirEntry* entry = it.Next().value;

		NotifyListener(B_ENTRY_REMOVED, fVolumeManager->GetID(),
			entry->GetDirectory()->GetID(), 0, entry->GetNode()->GetID(),
			entry->GetName());

		_RemoveEntry(entry);
	}
	fEntries->Clear();

	// get all IDs
	int32 count = fNodes->Size();
	if (count == 0)
		return;
	PRINT("  %" B_PRId32 " nodes to remove\n", count);
	vnode_id* ids = new(std::nothrow) vnode_id[count];
	if (!ids) {
		ERROR("ShareVolume::PrepareToUnmount(): ERROR: Insufficient memory to "
			"allocate the node ID array!\n");
		return;
	}
	ArrayDeleter<vnode_id> _(ids);
	count = 0;
	for (NodeMap::Iterator it = fNodes->GetIterator(); it.HasNext();) {
		ShareNode* node = it.Next().value;
		ids[count++] = node->GetID();
	}

	// Remove all nodes that are not known to the VFS right away.
	// If the netfs is already in the process of being unmounted, GetVNode()
	// will fail and the GetVNode(), RemoveVNode(), PutVNode() method won't
	// work for removing them.
	int32 remainingCount = 0;
	for (int32 i = 0; i < count; i++) {
		if (Node* node = fNodes->Get(ids[i])) {
			if (node->IsKnownToVFS()) {
				// node is known to VFS; we need to use the GetVNode(),
				// RemoveVNode(), PutVNode() method
				ids[remainingCount++] = ids[i];
			} else {
				// node is not known to VFS; just remove and delete it
				fNodes->Remove(node->GetID());
				_RemoveLocalNodeID(node->GetID());
				if (node != fRootNode)
					delete node;
			}
		}
	}
	count = remainingCount;

	locker.Unlock();

	// remove the nodes
	for (int32 i = 0; i < count; i++) {
		Node* node;
		if (GetVNode(ids[i], &node) == B_OK) {
			PRINT("  removing node %" B_PRIdINO "\n", ids[i]);
			Volume::RemoveVNode(ids[i]);
			PutVNode(ids[i]);
		}
	}

	// remove ourselves for the server connection
	if (fServerConnection)
		fServerConnection->RemoveVolume(this);

	// delete all entries

PRINT(("ShareVolume::PrepareToUnmount() done\n"));
}

// RemoveChildVolume
void
ShareVolume::RemoveChildVolume(Volume* volume)
{
	// should never be called
	WARN("WARNING: ShareVolume::RemoveChildVolume(%p) invoked!\n", volume);
}


// #pragma mark -
// #pragma mark ----- FS -----

// Unmount
status_t
ShareVolume::Unmount()
{
	if (_IsConnected()) {
		// send the request
		UnmountRequest request;
		request.volumeID = fID;
		fConnection->SendRequest(&request);
	}
	return B_OK;
}

// Sync
status_t
ShareVolume::Sync()
{
	// TODO: Implement?
	// We can't implement this without risking an endless recursion. The server
	// could only invoke sync(), which would sync all FSs, including a NetFS
	// which might be connected with a server running alongside this client.
	// We could introduce a sync flag to break the recursion. This might be
	// risky though.
	return B_OK;
}


// #pragma mark -
// #pragma mark ----- vnodes -----

// ReadVNode
status_t
ShareVolume::ReadVNode(vnode_id vnid, char reenter, Node** _node)
{
	// check the map, maybe it's already loaded
	ShareNode* node = NULL;
	{
		AutoLocker<Locker> _(fLock);
		node = _GetNodeByLocalID(vnid);
		if (node) {
			node->SetKnownToVFS(true);
			*_node = node;

			// add a volume reference for the node
			AcquireReference();

			return B_OK;
		}
	}

	// not yet loaded: send a request to the server
	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	// get the remote ID
	NodeID remoteID;
	status_t error = _GetRemoteNodeID(vnid, &remoteID);
	if (error != B_OK)
		return error;

	// prepare the request
	ReadVNodeRequest request;
	request.volumeID = fID;
	request.nodeID = remoteID;

	// send the request
	ReadVNodeReply* reply;
	error = SendRequest(fConnection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);
	if (reply->error != B_OK)
		RETURN_ERROR(reply->error);

	// add the node
	AutoLocker<Locker> _(fLock);
	error = _LoadNode(reply->nodeInfo, &node);
	if (error != B_OK)
		RETURN_ERROR(error);
	node->SetKnownToVFS(true);
	*_node = node;

	// add a volume reference for the node
	AcquireReference();

	return B_OK;
}

// WriteVNode
status_t
ShareVolume::WriteVNode(Node* node, char reenter)
{
	AutoLocker<Locker> locker(fLock);
	node->SetKnownToVFS(false);

	// surrender the node's volume reference
	locker.Unlock();
	PutVolume();

	return B_OK;
}

// RemoveVNode
status_t
ShareVolume::RemoveVNode(Node* node, char reenter)
{
	AutoLocker<Locker> locker(fLock);
	node->SetKnownToVFS(false);
	fNodes->Remove(node->GetID());
	_RemoveLocalNodeID(node->GetID());
	if (node != fRootNode)
		delete node;

	// surrender the node's volume reference
	locker.Unlock();
	PutVolume();

	return B_OK;
}


// #pragma mark -
// #pragma mark ----- nodes -----

// FSync
status_t
ShareVolume::FSync(Node* _node)
{
	// TODO: Implement!
	return B_BAD_VALUE;
}

// ReadStat
status_t
ShareVolume::ReadStat(Node* _node, struct stat* st)
{
	ShareNode* node = dynamic_cast<ShareNode*>(_node);

	AutoLocker<Locker> _(fLock);
	*st = node->GetNodeInfo().st;
	st->st_dev = fVolumeManager->GetID();
	st->st_ino = node->GetID();
	// we set the UID/GID fields to the one who mounted the FS
	st->st_uid = fVolumeManager->GetMountUID();
	st->st_gid = fVolumeManager->GetMountGID();
	return B_OK;
}

// WriteStat
status_t
ShareVolume::WriteStat(Node* _node, struct stat *st, uint32 mask)
{
	ShareNode* node = dynamic_cast<ShareNode*>(_node);

	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	// check read-only
	if (IsReadOnly())
		return B_PERMISSION_DENIED;
	// prepare the request
	WriteStatRequest request;
	request.volumeID = fID;
	request.nodeID = node->GetRemoteID();
	request.nodeInfo.st = *st;
	request.mask = mask;
	// send the request
	WriteStatReply* reply;
	status_t error = SendRequest(fConnection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);
	// update the node
	if (reply->nodeInfoValid)
		_UpdateNode(reply->nodeInfo);
	if (reply->error != B_OK)
		RETURN_ERROR(reply->error);
	return B_OK;
}

// Access
status_t
ShareVolume::Access(Node* _node, int mode)
{
	// TODO: Implement.
	return B_OK;
}


// #pragma mark -
// #pragma mark ----- files -----

// Create
status_t
ShareVolume::Create(Node* _dir, const char* name, int openMode, int mode,
	vnode_id* vnid, void** cookie)
{
	ShareDir* dir = dynamic_cast<ShareDir*>(_dir);
	if (!dir)
		return B_BAD_VALUE;

	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	// check permissions
	if (IsReadOnly())
		return B_PERMISSION_DENIED;
	if (IsVNodeRemoved(dir->GetID()) > 0)
		RETURN_ERROR(B_NOT_ALLOWED);

	// prepare the request
	CreateFileRequest request;
	request.volumeID = fID;
	request.directoryID = dir->GetRemoteID();
	request.name.SetTo(name);
	request.openMode = openMode;
	request.mode = mode;

	// send the request
	CreateFileReply* reply;
	status_t error = SendRequest(fConnection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);
	if (reply->error != B_OK)
		RETURN_ERROR(reply->error);

	// add/update the entry
	vnode_id localID = -1;
	EntryInfo* entryInfo = &reply->entryInfo;
	while (true) {
		// get an up to date entry info
		WalkReply* walkReply = NULL;
		if (!entryInfo) {
			error = _Walk(reply->entryInfo.directoryID,
				reply->entryInfo.name.GetString(), false, &walkReply);
			if (error != B_OK)
				break;

			entryInfo = &walkReply->entryInfo;
		}
		ObjectDeleter<Request> walkReplyDeleter(walkReply);

		AutoLocker<Locker> locker(fLock);

		// check, if the info is obsolete
		if (_IsObsoleteEntryInfo(*entryInfo)) {
			entryInfo = NULL;
			continue;
		}

		// load the entry
		ShareDirEntry* entry;
		error = _LoadEntry(dir, *entryInfo, &entry);
		if (error == B_OK)
			localID = entry->GetNode()->GetID();

		break;
	}

	if (error == B_OK) {
		Node* _node;
		error = GetVNode(localID, &_node);
	}

	// set the results / close the handle on error
	if (error == B_OK) {
		*vnid = localID;
		*cookie = (void*)reply->cookie;
	} else
		_Close(reply->cookie);

	RETURN_ERROR(error);
}

// Open
status_t
ShareVolume::Open(Node* _node, int openMode, void** cookie)
{
	ShareNode* node = dynamic_cast<ShareNode*>(_node);

// TODO: Allow opening the root node?
	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	// check the open mode
	if (IsReadOnly()) {
		if ((openMode & O_RWMASK) == O_WRONLY)
			return B_PERMISSION_DENIED;
		if (openMode & O_TRUNC)
			return B_PERMISSION_DENIED;
		if ((openMode & O_RWMASK) == O_RDWR)
			openMode = (openMode & ~O_RWMASK) | O_RDONLY;
	}
	// prepare the request
	OpenRequest request;
	request.volumeID = fID;
	request.nodeID = node->GetRemoteID();
	request.openMode = openMode;
	// send the request
	OpenReply* reply;
	status_t error = SendRequest(fConnection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);
	if (reply->error != B_OK)
		RETURN_ERROR(reply->error);
	// update the node
	_UpdateNode(reply->nodeInfo);
	*cookie = (void*)reply->cookie;
	return B_OK;
}

// Close
status_t
ShareVolume::Close(Node* _node, void* cookie)
{
	// no-op: FreeCookie does the job
	return B_OK;
}

// FreeCookie
status_t
ShareVolume::FreeCookie(Node* _node, void* cookie)
{
	return _Close((intptr_t)cookie);
}

// Read
status_t
ShareVolume::Read(Node* _node, void* cookie, off_t pos, void* _buffer,
	size_t bufferSize, size_t* _bytesRead)
{
	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	*_bytesRead = 0;
	if (bufferSize == 0)
		return B_OK;
	uint8* buffer = (uint8*)_buffer;
	// prepare the request
	ReadRequest request;
	request.volumeID = fID;
	request.cookie = (intptr_t)cookie;
	request.pos = pos;
	request.size = bufferSize;

	struct ReadRequestHandler : public RequestHandler {
		uint8*	buffer;
		off_t	pos;
		int32	bufferSize;
		int32	bytesRead;

		ReadRequestHandler(uint8* buffer, off_t pos, int32 bufferSize)
			: buffer(buffer),
			  pos(pos),
			  bufferSize(bufferSize),
			  bytesRead(0)
		{
		}

		virtual status_t HandleRequest(Request* _reply, RequestChannel* channel)
		{
			// the passed request is deleted by the request port
			ReadReply* reply = dynamic_cast<ReadReply*>(_reply);
			if (!reply)
				RETURN_ERROR(B_BAD_DATA);
			// process the reply
			status_t error = ProcessReply(reply);
			if (error != B_OK)
				return error;
			bool moreToCome = reply->moreToCome;
			while (moreToCome) {
				// receive a reply
				error = ReceiveRequest(channel, &reply);
				if (error != B_OK)
					RETURN_ERROR(error);
				moreToCome = reply->moreToCome;
				ObjectDeleter<Request> replyDeleter(reply);
				// process the reply
				error = ProcessReply(reply);
				if (error != B_OK)
					return error;
			}
			return B_OK;
		}

		status_t ProcessReply(ReadReply* reply)
		{
			if (reply->error != B_OK)
				RETURN_ERROR(reply->error);
			// check the fields
			if (reply->pos != pos)
				RETURN_ERROR(B_BAD_DATA);
			int32 bytesRead = reply->data.GetSize();
			if (bytesRead > (int32)bufferSize)
				RETURN_ERROR(B_BAD_DATA);
			// copy the data into the buffer
			if (bytesRead > 0)
				memcpy(buffer, reply->data.GetData(), bytesRead);
			pos += bytesRead;
			buffer += bytesRead;
			bufferSize -= bytesRead;
			this->bytesRead += bytesRead;
			return B_OK;
		}
	} requestHandler(buffer, pos, bufferSize);

	// send the request
	status_t error = fConnection->SendRequest(&request, &requestHandler);
	if (error != B_OK)
		RETURN_ERROR(error);
	*_bytesRead = requestHandler.bytesRead;
	return B_OK;
}

// Write
status_t
ShareVolume::Write(Node* _node, void* cookie, off_t pos, const void* _buffer,
	size_t bufferSize, size_t* bytesWritten)
{
	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	// check permissions
	if (IsReadOnly())
		return B_PERMISSION_DENIED;

	*bytesWritten = 0;
	off_t bytesLeft = bufferSize;
	const char* buffer = (const char*)_buffer;
	while (bytesLeft > 0) {
		off_t toWrite = bytesLeft;
		if (toWrite > kMaxWriteBufferSize)
			toWrite = kMaxWriteBufferSize;

		// prepare the request
		WriteRequest request;
		request.volumeID = fID;
		request.cookie = (intptr_t)cookie;
		request.pos = pos;
		request.data.SetTo(buffer, toWrite);

		// send the request
		WriteReply* reply;
		status_t error = SendRequest(fConnection, &request, &reply);
		if (error != B_OK)
			RETURN_ERROR(error);
		ObjectDeleter<Request> replyDeleter(reply);
		if (reply->error != B_OK)
			RETURN_ERROR(reply->error);

		bytesLeft -= toWrite;
		pos += toWrite;
		buffer += toWrite;

// TODO: We should probably add an "up to date" flag for ShareNode (just as
// done for ShareAttrDir) and clear it at this point. Currently continuity
// inconsistencies could occur (e.g. a stat() after a write() returns
// obsolete data), depending on when the node monitoring update arrives.
	}

	*bytesWritten = bufferSize;
	return B_OK;
}


// #pragma mark -
// #pragma mark ----- hard links / symlinks -----

// Link
status_t
ShareVolume::Link(Node* _dir, const char* name, Node* _node)
{
	ShareNode* dir = dynamic_cast<ShareNode*>(_dir);
	ShareNode* node = dynamic_cast<ShareNode*>(_node);

	if (!node || node->GetVolume() != this)
		return B_NOT_ALLOWED;

	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	// check permissions
	if (IsReadOnly())
		return B_PERMISSION_DENIED;
	if (IsVNodeRemoved(dir->GetID()) > 0)
		RETURN_ERROR(B_NOT_ALLOWED);
	if (IsVNodeRemoved(node->GetID()) > 0)
		RETURN_ERROR(B_NOT_ALLOWED);
	// prepare the request
	CreateLinkRequest request;
	request.volumeID = fID;
	request.directoryID = dir->GetRemoteID();
	request.name.SetTo(name);
	request.nodeID = node->GetRemoteID();
	// send the request
	CreateLinkReply* reply;
	status_t error = SendRequest(fConnection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);
	RETURN_ERROR(reply->error);
}

// Unlink
status_t
ShareVolume::Unlink(Node* _dir, const char* name)
{
	ShareNode* dir = dynamic_cast<ShareNode*>(_dir);

	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	// check permissions
	if (IsReadOnly())
		return B_PERMISSION_DENIED;
	// prepare the request
	UnlinkRequest request;
	request.volumeID = fID;
	request.directoryID = dir->GetRemoteID();
	request.name.SetTo(name);
	// send the request
	UnlinkReply* reply;
	status_t error = SendRequest(fConnection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);
	RETURN_ERROR(reply->error);
}

// Symlink
status_t
ShareVolume::Symlink(Node* _dir, const char* name, const char* target)
{
	ShareNode* dir = dynamic_cast<ShareNode*>(_dir);

	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	// check permissions
	if (IsReadOnly())
		return B_PERMISSION_DENIED;
	if (IsVNodeRemoved(dir->GetID()) > 0)
		RETURN_ERROR(B_NOT_ALLOWED);
	// prepare the request
	CreateSymlinkRequest request;
	request.volumeID = fID;
	request.directoryID = dir->GetRemoteID();
	request.name.SetTo(name);
	request.target.SetTo(target);
	// send the request
	CreateSymlinkReply* reply;
	status_t error = SendRequest(fConnection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);
	RETURN_ERROR(reply->error);
}

// ReadLink
status_t
ShareVolume::ReadLink(Node* _node, char* buffer, size_t bufferSize,
	size_t* bytesRead)
{
	ShareNode* node = dynamic_cast<ShareNode*>(_node);

	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	*bytesRead = 0;
	// prepare the request
	ReadLinkRequest request;
	request.volumeID = fID;
	request.nodeID = node->GetRemoteID();
	request.maxSize = bufferSize;
	// send the request
	ReadLinkReply* reply;
	status_t error = SendRequest(fConnection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);
	if (reply->error != B_OK)
		RETURN_ERROR(reply->error);
	if (reply->data.GetSize() > (int32)bufferSize)
		RETURN_ERROR(B_BAD_DATA);
	*bytesRead = reply->data.GetSize();
	if (*bytesRead > 0)
		memcpy(buffer, reply->data.GetData(), *bytesRead);
	_UpdateNode(reply->nodeInfo);
	return B_OK;
}

// Rename
status_t
ShareVolume::Rename(Node* _oldDir, const char* oldName, Node* _newDir,
	const char* newName)
{
	ShareNode* oldDir = dynamic_cast<ShareNode*>(_oldDir);
	ShareNode* newDir = dynamic_cast<ShareNode*>(_newDir);

	if (!newDir || newDir->GetVolume() != this)
		return B_NOT_ALLOWED;

	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	// check permissions
	if (IsReadOnly())
		return B_PERMISSION_DENIED;
	if (IsVNodeRemoved(newDir->GetID()) > 0)
		RETURN_ERROR(B_NOT_ALLOWED);
	// prepare the request
	RenameRequest request;
	request.volumeID = fID;
	request.oldDirectoryID = oldDir->GetRemoteID();
	request.oldName.SetTo(oldName);
	request.newDirectoryID = newDir->GetRemoteID();
	request.newName.SetTo(newName);
	// send the request
	RenameReply* reply;
	status_t error = SendRequest(fConnection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);
	RETURN_ERROR(reply->error);
}


// #pragma mark -
// #pragma mark ----- directories -----

// MkDir
status_t
ShareVolume::MkDir(Node* _dir, const char* name, int mode)
{
	ShareNode* dir = dynamic_cast<ShareNode*>(_dir);

	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	// check permissions
	if (IsReadOnly())
		return B_PERMISSION_DENIED;
	if (IsVNodeRemoved(dir->GetID()) > 0)
		RETURN_ERROR(B_NOT_ALLOWED);
	// prepare the request
	MakeDirRequest request;
	request.volumeID = fID;
	request.directoryID = dir->GetRemoteID();
	request.name.SetTo(name);
	request.mode = mode;
	// send the request
	MakeDirReply* reply;
	status_t error = SendRequest(fConnection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);
	RETURN_ERROR(reply->error);
}

// RmDir
status_t
ShareVolume::RmDir(Node* _dir, const char* name)
{
	ShareNode* dir = dynamic_cast<ShareNode*>(_dir);

	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	// check permissions
	if (IsReadOnly())
		return B_PERMISSION_DENIED;
	// prepare the request
	RemoveDirRequest request;
	request.volumeID = fID;
	request.directoryID = dir->GetRemoteID();
	request.name.SetTo(name);
	// send the request
	RemoveDirReply* reply;
	status_t error = SendRequest(fConnection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);
	RETURN_ERROR(reply->error);
}

// OpenDir
status_t
ShareVolume::OpenDir(Node* _node, void** _cookie)
{
	// we opendir() only directories
	ShareDir* node = dynamic_cast<ShareDir*>(_node);
	if (!node)
		return B_NOT_ALLOWED;

// TODO: Allow opening the root node?
	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	// allocate a dir cookie
	DirCookie* cookie = new(std::nothrow) DirCookie;
	if (!cookie)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<DirCookie> cookieDeleter(cookie);

	// if the directory is fully cached, we allocate a local iterator
	{
		AutoLocker<Locker> locker(fLock);
		if (node->IsComplete()) {
			// create a local dir iterator
			LocalShareDirIterator* iterator
				= new(std::nothrow) LocalShareDirIterator();
			if (!iterator)
				RETURN_ERROR(B_NO_MEMORY);
			iterator->SetDirectory(node);

			// init the cookie
			cookie->iterator = iterator;
			*_cookie = cookie;
			cookieDeleter.Detach();
			return B_OK;
		}
	}

	// allocate a remote dir iterator
	RemoteShareDirIterator* iterator = new(std::nothrow) RemoteShareDirIterator;
	if (!iterator)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<RemoteShareDirIterator> iteratorDeleter(iterator);

	// prepare the request
	OpenDirRequest request;
	request.volumeID = fID;
	request.nodeID = node->GetRemoteID();

	// send the request
	OpenDirReply* reply;
	status_t error = SendRequest(fConnection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);
	if (reply->error != B_OK)
{
		PRINT("OpenDir() failed: node: %" B_PRIdINO ", remote: (%" B_PRIdDEV
			", %" B_PRIdINO ")\n", node->GetID(), node->GetRemoteID().volumeID,
			node->GetRemoteID().nodeID);
		RETURN_ERROR(reply->error);
}

	// update the node
	_UpdateNode(reply->nodeInfo);

	// init the cookie
	iterator->SetCookie(reply->cookie);
	cookie->iterator = iterator;

	*_cookie = cookie;
	cookieDeleter.Detach();
	iteratorDeleter.Detach();
	return B_OK;
}

// CloseDir
status_t
ShareVolume::CloseDir(Node* _node, void* cookie)
{
	// no-op: FreeDirCookie does the job
	return B_OK;
}

// FreeDirCookie
status_t
ShareVolume::FreeDirCookie(Node* _node, void* _cookie)
{
	DirCookie* cookie = (DirCookie*)_cookie;
	ObjectDeleter<DirCookie> _(cookie);
	ShareDirIterator* iterator = cookie->iterator;

	status_t error = B_OK;
	if (RemoteShareDirIterator* remoteIterator
		= dynamic_cast<RemoteShareDirIterator*>(iterator)) {
		// prepare the request
		CloseRequest request;
		request.volumeID = fID;
		request.cookie = remoteIterator->GetCookie();

		// send the request
		if (!_EnsureShareMounted())
			error = ERROR_NOT_CONNECTED;

		if (error == B_OK) {
			CloseReply* reply;
			error = SendRequest(fConnection, &request, &reply);
			if (error == B_OK) {
				error = reply->error;
				delete reply;
			}
		}
	}

	// delete the iterator
	AutoLocker<Locker> locker(fLock);
	delete iterator;

	return error;
}

// ReadDir
status_t
ShareVolume::ReadDir(Node* _dir, void* _cookie, struct dirent* buffer,
	size_t bufferSize, int32 count, int32* countRead)
{
	ShareDir* directory = dynamic_cast<ShareDir*>(_dir);

	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	*countRead = 0;
	if (count <= 0)
		return B_OK;

	DirCookie* cookie = (DirCookie*)_cookie;
	ShareDirIterator* iterator = cookie->iterator;

	while (true) {
		status_t error;
		AutoLocker<Locker> locker(fLock);
		while (ShareDirEntry* entry = iterator->GetCurrentEntry()) {
			// re-get the entry -- it might already have been removed
			const char* name = entry->GetName();
			entry = _GetEntryByLocalID(directory->GetID(), name);
			if (entry) {
				// set the name: this also checks the size of the buffer
				error = set_dirent_name(buffer, bufferSize, name,
					strlen(name));
				if (error != B_OK) {
					// if something has been read at all, we're content
					if (*countRead > 0)
						return B_OK;
					RETURN_ERROR(error);
				}

				// fill in the other fields
				buffer->d_pdev = fVolumeManager->GetID();
				buffer->d_pino = directory->GetID();
				buffer->d_dev = fVolumeManager->GetID();
				buffer->d_ino = entry->GetNode()->GetID();

				// if the entry is the parent of the share root, we need to
				// fix the node ID
				if (directory == fRootNode && strcmp(name, "..") == 0) {
					if (Volume* parentVolume = GetParentVolume())
						buffer->d_ino = parentVolume->GetRootID();
				}

				iterator->NextEntry();
				(*countRead)++;
				if (*countRead >= count || !next_dirent(buffer, bufferSize))
					return B_OK;
			} else
				iterator->NextEntry();
		}

		// no more entries: check, if we're completely through
		if (iterator->IsDone())
			return B_OK;

		// we need to actually get entries from the server
		locker.Unlock();
		if (RemoteShareDirIterator* remoteIterator
				= dynamic_cast<RemoteShareDirIterator*>(iterator)) {
			error = _ReadRemoteDir(directory, remoteIterator);
			if (error != B_OK)
				return error;
		}
	}
}

// RewindDir
status_t
ShareVolume::RewindDir(Node* _node, void* _cookie)
{
	DirCookie* cookie = (DirCookie*)_cookie;
	ShareDirIterator* iterator = cookie->iterator;
	AutoLocker<Locker> _(fLock);
	iterator->Rewind();
	return B_OK;
}

// Walk
status_t
ShareVolume::Walk(Node* _dir, const char* entryName, char** resolvedPath,
	vnode_id* vnid)
{
	ShareDir* dir = dynamic_cast<ShareDir*>(_dir);
	if (!dir)
		return B_BAD_VALUE;

	// we always resolve "." and ".." of the root node
	if (dir == fRootNode) {
		if (strcmp(entryName, ".") == 0 || strcmp(entryName, "..") == 0) {
			AutoLocker<Locker> _(fLock);
			if (strcmp(entryName, ".") == 0) {
				*vnid = fRootNode->GetID();
			} else if (Volume* parentVolume = GetParentVolume()) {
				*vnid = parentVolume->GetRootID();
			} else
				*vnid = fRootNode->GetID();
			Node* node;
			return GetVNode(*vnid, &node);
		}
	}

	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	// check, if the entry is already known
	{
		AutoLocker<Locker> _(fLock);
		ShareDirEntry* entry = _GetEntryByLocalID(dir->GetID(), entryName);
		if (entry) {
			*vnid = entry->GetNode()->GetID();
			Node* node;
			return GetVNode(*vnid, &node);
		} else if (dir->IsComplete())
			return B_ENTRY_NOT_FOUND;
	}

	WalkReply* reply;
	while (true) {
		// send the request
		status_t error = _Walk(dir->GetRemoteID(), entryName, resolvedPath,
			&reply);
		if (error != B_OK)
			RETURN_ERROR(error);
		ObjectDeleter<Request> replyDeleter(reply);

		AutoLocker<Locker> locker(fLock);

		// check, if the returned info is obsolete
		if (_IsObsoleteEntryInfo(reply->entryInfo))
			continue;

		// load the entry
		ShareDirEntry* entry;
		error = _LoadEntry(dir, reply->entryInfo, &entry);
		if (error != B_OK)
			RETURN_ERROR(error);
		*vnid = entry->GetNode()->GetID();

		// deal with symlinks
		if (reply->linkPath.GetString() && resolvedPath) {
			*resolvedPath = strdup(reply->linkPath.GetString());
			if (!*resolvedPath)
				RETURN_ERROR(B_NO_MEMORY);
			return B_OK;
		}

		break;
	}

	// no symlink or we shall not resolve it: get the node
	Node* _node;
	RETURN_ERROR(GetVNode(*vnid, &_node));
}


// #pragma mark -
// #pragma mark ----- attributes -----

// OpenAttrDir
status_t
ShareVolume::OpenAttrDir(Node* _node, void** _cookie)
{
	ShareNode* node = dynamic_cast<ShareNode*>(_node);

// TODO: Allow opening the root node?
	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	// allocate a dir cookie
	AttrDirCookie* cookie = new(std::nothrow) AttrDirCookie;
	if (!cookie)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<AttrDirCookie> cookieDeleter(cookie);

	AutoLocker<Locker> locker(fLock);

	if (!node->GetAttrDir() || !node->GetAttrDir()->IsUpToDate()) {
		// prepare the request
		OpenAttrDirRequest request;
		request.volumeID = fID;
		request.nodeID = node->GetRemoteID();

		locker.Unlock();

		// send the request
		OpenAttrDirReply* reply;
		status_t error = SendRequest(fConnection, &request, &reply);
		if (error != B_OK)
			RETURN_ERROR(error);
		ObjectDeleter<Request> replyDeleter(reply);
		if (reply->error != B_OK)
			RETURN_ERROR(reply->error);

		// If no AttrDirInfo was supplied, we just save the cookie and be done.
		// This usually happens when the attr dir is too big to be cached.
		if (!reply->attrDirInfo.isValid) {
			cookie->cookie = reply->cookie;
			cookie->rewind = false;
			*_cookie = cookie;
			cookieDeleter.Detach();
			return B_OK;
		}

		locker.SetTo(fLock, false);

		// a AttrDirInfo has been supplied: load the attr dir
		error = _LoadAttrDir(node, reply->attrDirInfo);
		if (error != B_OK)
			return error;
	}

	// we have a valid attr dir: create an attr dir iterator
	ShareAttrDirIterator* iterator = new(std::nothrow) ShareAttrDirIterator;
	if (!iterator)
		return B_NO_MEMORY;
	iterator->SetAttrDir(node->GetAttrDir());

	// add the iterator
	status_t error = _AddAttrDirIterator(node, iterator);
	if (error != B_OK) {
		delete iterator;
		return error;
	}

	cookie->iterator = iterator;
	*_cookie = cookie;
	cookieDeleter.Detach();
	return B_OK;
}

// CloseAttrDir
status_t
ShareVolume::CloseAttrDir(Node* _node, void* cookie)
{
	// no-op: FreeAttrDirCookie does the job
	return B_OK;
}

// FreeAttrDirCookie
status_t
ShareVolume::FreeAttrDirCookie(Node* _node, void* _cookie)
{
	ShareNode* node = dynamic_cast<ShareNode*>(_node);
	AttrDirCookie* cookie = (AttrDirCookie*)_cookie;
	ObjectDeleter<AttrDirCookie> _(cookie);

	// if this is a local iterator, we just delete it and be done
	if (cookie->iterator) {
		AutoLocker<Locker> locker(fLock);

		// remove and delete the iterator
		_RemoveAttrDirIterator(node, cookie->iterator);
		delete cookie->iterator;

		return B_OK;
	}

	// prepare the request
	CloseRequest request;
	request.volumeID = fID;
	request.cookie = cookie->cookie;

	// send the request
	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;
	CloseReply* reply;
	status_t error = SendRequest(fConnection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);
	if (reply->error != B_OK)
		RETURN_ERROR(reply->error);

	return B_OK;
}

// ReadAttrDir
status_t
ShareVolume::ReadAttrDir(Node* _node, void* _cookie, struct dirent* buffer,
	size_t bufferSize, int32 count, int32* countRead)
{
	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	*countRead = 0;
	AttrDirCookie* cookie = (AttrDirCookie*)_cookie;

	// if we have a local iterator, things are easy
	if (ShareAttrDirIterator* iterator = cookie->iterator) {
		AutoLocker<Locker> locker(fLock);

		// get the current attribute
		Attribute* attribute = iterator->GetCurrentAttribute();
		if (!attribute)
			return B_OK;

		// set the name: this also checks the size of the buffer
		const char* name = attribute->GetName();
		status_t error = set_dirent_name(buffer, bufferSize, name,
			strlen(name));
		if (error != B_OK)
			RETURN_ERROR(error);

		// fill in the other fields
		buffer->d_dev = fVolumeManager->GetID();
		buffer->d_ino = -1;
		*countRead = 1;

		iterator->NextAttribute();
		return B_OK;
	}

	// prepare the request
	ReadAttrDirRequest request;
	request.volumeID = fID;
	request.cookie = cookie->cookie;
	request.count = 1;
	request.rewind = cookie->rewind;

	// send the request
	ReadAttrDirReply* reply;
	status_t error = SendRequest(fConnection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);
	if (reply->error != B_OK)
		RETURN_ERROR(reply->error);
	cookie->rewind = false;

	// check, if anything has been read at all
	if (reply->count == 0) {
		*countRead = 0;
		return B_OK;
	}
	const char* name = reply->name.GetString();
	int32 nameLen = reply->name.GetSize();
	if (!name || nameLen < 2)
		return B_BAD_DATA;

	// set the name: this also checks the size of the buffer
	error = set_dirent_name(buffer, bufferSize, name, nameLen - 1);
	if (error != B_OK)
		RETURN_ERROR(error);

	// fill in the other fields
	buffer->d_dev = fVolumeManager->GetID();
	buffer->d_ino = -1;
	*countRead = 1;
	return B_OK;
}

// RewindAttrDir
status_t
ShareVolume::RewindAttrDir(Node* _node, void* _cookie)
{
	AttrDirCookie* cookie = (AttrDirCookie*)_cookie;

	// if we have a local iterator, rewind it
	if (ShareAttrDirIterator* iterator = cookie->iterator) {
		AutoLocker<Locker> locker(fLock);

		iterator->Rewind();
	} else
		cookie->rewind = true;

	return B_OK;
}

// ReadAttr
status_t
ShareVolume::ReadAttr(Node* _node, const char* name, int type, off_t pos,
	void* _buffer, size_t bufferSize, size_t* _bytesRead)
{
	ShareNode* node = dynamic_cast<ShareNode*>(_node);

	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	*_bytesRead = 0;
	if (bufferSize == 0)
		return B_OK;
	uint8* buffer = (uint8*)_buffer;

	// if we have the attribute directory cached, we can first check, if the
	// attribute exists at all -- maybe its data are cached, too
	{
		AutoLocker<Locker> locker(fLock);

		ShareAttrDir* attrDir = node->GetAttrDir();
		if (attrDir && attrDir->IsUpToDate()) {
			// get the attribute
			Attribute* attribute = attrDir->GetAttribute(name);
			if (!attribute)
				return B_ENTRY_NOT_FOUND;

			// get the data
			if (const void* data = attribute->GetData()) {
				// first check, if we can read anything at all
				if (pos < 0)
					pos = 0;
				int32 size = attribute->GetSize();
				if (pos >= size)
					return B_OK;

				// get the data
				bufferSize = min(bufferSize, size_t(size - pos));
				memcpy(buffer, data, bufferSize);
				*_bytesRead = bufferSize;
				return B_OK;
			}
		}
	}

	// prepare the request
	ReadAttrRequest request;
	request.volumeID = fID;
	request.nodeID = node->GetRemoteID();
	request.name.SetTo(name);
	request.type = type;
	request.pos = pos;
	request.size = bufferSize;

	struct ReadRequestHandler : public RequestHandler {
		uint8*	buffer;
		off_t	pos;
		int32	bufferSize;
		int32	bytesRead;

		ReadRequestHandler(uint8* buffer, off_t pos, int32 bufferSize)
			: buffer(buffer),
			  pos(pos),
			  bufferSize(bufferSize),
			  bytesRead(0)
		{
		}

		virtual status_t HandleRequest(Request* _reply, RequestChannel* channel)
		{
			// the passed request is deleted by the request port
			ReadAttrReply* reply = dynamic_cast<ReadAttrReply*>(_reply);
			if (!reply)
				RETURN_ERROR(B_BAD_DATA);
			// process the reply
			status_t error = ProcessReply(reply);
			if (error != B_OK)
				return error;
			bool moreToCome = reply->moreToCome;
			while (moreToCome) {
				// receive a reply
				error = ReceiveRequest(channel, &reply);
				if (error != B_OK)
					RETURN_ERROR(error);
				moreToCome = reply->moreToCome;
				ObjectDeleter<Request> replyDeleter(reply);
				// process the reply
				error = ProcessReply(reply);
				if (error != B_OK)
					return error;
			}
			return B_OK;
		}

		status_t ProcessReply(ReadAttrReply* reply)
		{
			if (reply->error != B_OK)
				RETURN_ERROR(reply->error);
			// check the fields
			if (reply->pos != pos)
				RETURN_ERROR(B_BAD_DATA);
			int32 bytesRead = reply->data.GetSize();
			if (bytesRead > (int32)bufferSize)
				RETURN_ERROR(B_BAD_DATA);
			// copy the data into the buffer
			if (bytesRead > 0)
				memcpy(buffer, reply->data.GetData(), bytesRead);
			pos += bytesRead;
			buffer += bytesRead;
			bufferSize -= bytesRead;
			this->bytesRead += bytesRead;
			return B_OK;
		}
	} requestHandler(buffer, pos, bufferSize);

	// send the request
	status_t error = fConnection->SendRequest(&request, &requestHandler);
	if (error != B_OK)
		RETURN_ERROR(error);
	*_bytesRead = requestHandler.bytesRead;
	return B_OK;
}

// WriteAttr
status_t
ShareVolume::WriteAttr(Node* _node, const char* name, int type, off_t pos,
	const void* _buffer, size_t bufferSize, size_t* bytesWritten)
{
	ShareNode* node = dynamic_cast<ShareNode*>(_node);

	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	// check permissions
	if (IsReadOnly())
		return B_PERMISSION_DENIED;

	*bytesWritten = 0;
	off_t bytesLeft = bufferSize;
	const char* buffer = (const char*)_buffer;
	while (bytesLeft > 0) {
		// store the current attibute dir revision for reference below
		int64 attrDirRevision = -1;
		{
			AutoLocker<Locker> _(fLock);
			if (ShareAttrDir* attrDir = node->GetAttrDir()) {
				if (attrDir->IsUpToDate())
					attrDirRevision = attrDir->GetRevision();
			}
		}

		off_t toWrite = bytesLeft;
		if (toWrite > kMaxWriteBufferSize)
			toWrite = kMaxWriteBufferSize;

		// prepare the request
		WriteAttrRequest request;
		request.volumeID = fID;
		request.nodeID = node->GetRemoteID();
		request.name.SetTo(name);
		request.type = type;
		request.pos = pos;
		request.data.SetTo(buffer, toWrite);

		// send the request
		WriteAttrReply* reply;
		status_t error = SendRequest(fConnection, &request, &reply);
		if (error != B_OK)
			RETURN_ERROR(error);

		ObjectDeleter<Request> replyDeleter(reply);
		if (reply->error != B_OK)
			RETURN_ERROR(reply->error);

		bytesLeft -= toWrite;
		pos += toWrite;
		buffer += toWrite;

		// If the request was successful, we consider the cached attr dir
		// no longer up to date.
		if (attrDirRevision >= 0) {
			AutoLocker<Locker> _(fLock);
			ShareAttrDir* attrDir = node->GetAttrDir();
			if (attrDir && attrDir->GetRevision() == attrDirRevision)
				attrDir->SetUpToDate(false);
		}
	}

	*bytesWritten = bufferSize;
	return B_OK;
}

// RemoveAttr
status_t
ShareVolume::RemoveAttr(Node* _node, const char* name)
{
	ShareNode* node = dynamic_cast<ShareNode*>(_node);

	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	// check permissions
	if (IsReadOnly())
		return B_PERMISSION_DENIED;

	// store the current attibute dir revision for reference below
	int64 attrDirRevision = -1;
	{
		AutoLocker<Locker> _(fLock);
		if (ShareAttrDir* attrDir = node->GetAttrDir()) {
			if (attrDir->IsUpToDate())
				attrDirRevision = attrDir->GetRevision();
		}
	}

	// prepare the request
	RemoveAttrRequest request;
	request.volumeID = fID;
	request.nodeID = node->GetRemoteID();
	request.name.SetTo(name);

	// send the request
	RemoveAttrReply* reply;
	status_t error = SendRequest(fConnection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);

	// If the request was successful, we consider the cached attr dir
	// no longer up to date.
	if (reply->error == B_OK && attrDirRevision >= 0) {
		AutoLocker<Locker> _(fLock);
		ShareAttrDir* attrDir = node->GetAttrDir();
		if (attrDir && attrDir->GetRevision() == attrDirRevision)
			attrDir->SetUpToDate(false);
	}

	RETURN_ERROR(reply->error);
}

// RenameAttr
status_t
ShareVolume::RenameAttr(Node* _node, const char* oldName, const char* newName)
{
	ShareNode* node = dynamic_cast<ShareNode*>(_node);

	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	// check permissions
	if (IsReadOnly())
		return B_PERMISSION_DENIED;

	// store the current attibute dir revision for reference below
	int64 attrDirRevision = -1;
	{
		AutoLocker<Locker> _(fLock);
		if (ShareAttrDir* attrDir = node->GetAttrDir()) {
			if (attrDir->IsUpToDate())
				attrDirRevision = attrDir->GetRevision();
		}
	}

	// prepare the request
	RenameAttrRequest request;
	request.volumeID = fID;
	request.nodeID = node->GetRemoteID();
	request.oldName.SetTo(oldName);
	request.newName.SetTo(newName);

	// send the request
	RenameAttrReply* reply;
	status_t error = SendRequest(fConnection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);

	// If the request was successful, we consider the cached attr dir
	// no longer up to date.
	if (reply->error == B_OK && attrDirRevision >= 0) {
		AutoLocker<Locker> _(fLock);
		ShareAttrDir* attrDir = node->GetAttrDir();
		if (attrDir && attrDir->GetRevision() == attrDirRevision)
			attrDir->SetUpToDate(false);
	}

	RETURN_ERROR(reply->error);
}

// StatAttr
status_t
ShareVolume::StatAttr(Node* _node, const char* name, struct attr_info* attrInfo)
{
	ShareNode* node = dynamic_cast<ShareNode*>(_node);

	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	// if we have the attribute directory cached, get the info from there
	{
		AutoLocker<Locker> locker(fLock);

		ShareAttrDir* attrDir = node->GetAttrDir();
		if (attrDir && attrDir->IsUpToDate()) {
			// get the attribute
			Attribute* attribute = attrDir->GetAttribute(name);
			if (!attribute)
				return B_ENTRY_NOT_FOUND;

			attribute->GetInfo(attrInfo);
			return B_OK;
		}
	}

	// prepare the request
	StatAttrRequest request;
	request.volumeID = fID;
	request.nodeID = node->GetRemoteID();
	request.name.SetTo(name);

	// send the request
	StatAttrReply* reply;
	status_t error = SendRequest(fConnection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);
	if (reply->error != B_OK)
		RETURN_ERROR(reply->error);

	// set the result
	*attrInfo = reply->attrInfo.info;
	return B_OK;
}


// #pragma mark -
// #pragma mark ----- queries -----

// GetQueryEntry
status_t
ShareVolume::GetQueryEntry(const EntryInfo& entryInfo,
	const NodeInfo& dirInfo, struct dirent* buffer, size_t bufferSize,
	int32* countRead)
{
	status_t error = B_OK;

	const char* name = entryInfo.name.GetString();
	int32 nameLen = entryInfo.name.GetSize();
	if (!name || nameLen < 2)
		RETURN_ERROR(B_BAD_DATA);

	// load the directory
	vnode_id localDirID = -1;
	{
		AutoLocker<Locker> _(fLock);
		ShareNode* node;
		error = _LoadNode(dirInfo, &node);
		if (error != B_OK)
			RETURN_ERROR(error);
		ShareDir* directory = dynamic_cast<ShareDir*>(node);
		if (!directory)
			RETURN_ERROR(B_ENTRY_NOT_FOUND);
		localDirID = directory->GetID();
	}

	// add/update the entry
	vnode_id localNodeID = -1;
	const EntryInfo* resolvedEntryInfo = &entryInfo;
	while (error == B_OK) {
		// get an up to date entry info
		WalkReply* walkReply = NULL;
		if (!resolvedEntryInfo) {
			error = _Walk(entryInfo.directoryID, name, false,
				&walkReply);
			if (error != B_OK)
				RETURN_ERROR(error);

			resolvedEntryInfo = &walkReply->entryInfo;
		}
		ObjectDeleter<Request> walkReplyDeleter(walkReply);

		AutoLocker<Locker> locker(fLock);

		// check, if the info is obsolete
		if (_IsObsoleteEntryInfo(*resolvedEntryInfo)) {
			resolvedEntryInfo = NULL;
			continue;
		}

		// get the directory
		ShareDir* dir = dynamic_cast<ShareDir*>(_GetNodeByLocalID(localDirID));
		if (!dir)
			RETURN_ERROR(B_ERROR);

		// load the entry
		ShareDirEntry* entry;
		error = _LoadEntry(dir, *resolvedEntryInfo, &entry);
		if (error == B_OK)
			localNodeID = entry->GetNode()->GetID();

		break;
	}

	// set the name: this also checks the size of the buffer
	error = set_dirent_name(buffer, bufferSize, name, nameLen - 1);
	if (error != B_OK)
		RETURN_ERROR(error);

	// fill in the other fields
	buffer->d_pdev = fVolumeManager->GetID();
	buffer->d_pino = localDirID;
	buffer->d_dev = fVolumeManager->GetID();
	buffer->d_ino = localNodeID;

	*countRead = 1;
	PRINT("  entry: d_dev: %" B_PRIdDEV ", d_pdev: %" B_PRIdDEV ", d_ino: %"
		B_PRIdINO ", d_pino: %" B_PRIdINO ", d_reclen: %hu, d_name: `%s'\n",
		buffer->d_dev, buffer->d_pdev, buffer->d_ino, buffer->d_pino,
		buffer->d_reclen, buffer->d_name);
	return B_OK;
}


// #pragma mark -

// ProcessNodeMonitoringRequest
void
ShareVolume::ProcessNodeMonitoringRequest(NodeMonitoringRequest* request)
{
	switch (request->opcode) {
		case B_ENTRY_CREATED:
			_HandleEntryCreatedRequest(
				dynamic_cast<EntryCreatedRequest*>(request));
			break;
		case B_ENTRY_REMOVED:
			_HandleEntryRemovedRequest(
				dynamic_cast<EntryRemovedRequest*>(request));
			break;
		case B_ENTRY_MOVED:
			_HandleEntryMovedRequest(
				dynamic_cast<EntryMovedRequest*>(request));
			break;
		case B_STAT_CHANGED:
			_HandleStatChangedRequest(
				dynamic_cast<StatChangedRequest*>(request));
			break;
		case B_ATTR_CHANGED:
			_HandleAttributeChangedRequest(
				dynamic_cast<AttributeChangedRequest*>(request));
			break;
	}
}

// ConnectionClosed
void
ShareVolume::ConnectionClosed()
{
	AutoLocker<Locker> _(fMountLock);
	fConnectionState = CONNECTION_CLOSED;
}


// #pragma mark -

// _ReadRemoteDir
status_t
ShareVolume::_ReadRemoteDir(ShareDir* directory,
	RemoteShareDirIterator* iterator)
{
	// prepare the request
	fLock.Lock();
	ReadDirRequest request;
	request.volumeID = fID;
	request.cookie = iterator->GetCookie();
	request.count = iterator->GetCapacity();
	request.rewind = iterator->GetRewind();
	fLock.Unlock();

	// send the request
	ReadDirReply* reply;
	status_t error = SendRequest(fConnection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);
	if (reply->error != B_OK)
		RETURN_ERROR(reply->error);

	RequestMemberArray<EntryInfo>* entryInfos = &reply->entryInfos;
	while (true) {
		// get up to date entry infos
		MultiWalkReply* walkReply = NULL;
		if (!entryInfos) {
			error = _MultiWalk(reply->entryInfos, &walkReply);
			if (error != B_OK)
				RETURN_ERROR(error);

			entryInfos = &walkReply->entryInfos;
		}
		ObjectDeleter<Request> walkReplyDeleter(walkReply);

		AutoLocker<Locker> locker(fLock);

		// check, if any info is obsolete
		int32 count = entryInfos->CountElements();
		for (int32 i = 0; i < count; i++) {
			const EntryInfo& entryInfo = entryInfos->GetElements()[i];
			if (_IsObsoleteEntryInfo(entryInfo)) {
				entryInfos = NULL;
				continue;
			}
		}

		// init the iterator's revision, if it's new or has been rewinded
		if (request.rewind || iterator->GetRevision() < 0)
			iterator->SetRevision(reply->revision);

		iterator->Clear();
		iterator->SetDone(reply->done);

		// iterate through the entries
		for (int32 i = 0; i < count; i++) {
			const EntryInfo& entryInfo = entryInfos->GetElements()[i];
			const char* name = entryInfo.name.GetString();
			int32 nameLen = entryInfo.name.GetSize();
			if (!name || nameLen < 2)
				return B_BAD_DATA;

			// load the node/entry
			ShareDirEntry* entry;
			error = _LoadEntry(directory, entryInfo, &entry);
			if (error != B_OK)
				RETURN_ERROR(error);

			// add the entry
			if (!iterator->AddEntry(entry)) {
				ERROR("ShareVolume::_ReadRemoteDir(): ERROR: Failed to add "
					"entry to remote iterator!\n");
			}
		}

		// directory now complete?
		if (reply->done && directory->GetEntryRemovedEventRevision()
				< iterator->GetRevision()) {
			directory->SetComplete(true);
		}

		break;
	}

	return B_OK;
}

// _HandleEntryCreatedRequest
void
ShareVolume::_HandleEntryCreatedRequest(EntryCreatedRequest* request)
{
	if (!request)
		return;

	nspace_id nsid = fVolumeManager->GetID();
	const char* name = request->name.GetString();

	// translate the node IDs
	vnode_id vnida = 0;
	vnode_id vnidb = 0;
	vnode_id vnidc = 0;
	status_t error = _GetLocalNodeID(request->directoryID, &vnida, true);
	if (error == B_OK)
		error = _GetLocalNodeID(request->nodeID, &vnidc, true);
		PRINT("ShareVolume::_HandleEntryCreatedRequest(): error: 0x%" B_PRIx32
			", name: \"%s\", dir: %" B_PRIdINO " (remote: (%" B_PRIdDEV ", %"
			B_PRIdINO ")), node: %" B_PRIdINO " (remote: (%" B_PRIdDEV ", %"
			B_PRIdINO "))\n", error, name, vnida,
			request->directoryID.volumeID, request->directoryID.nodeID, vnidc,
			request->nodeID.volumeID, request->nodeID.nodeID);

	// send notifications / do additional processing
	if (request->queryUpdate) {
		if (error == B_OK) {
			SendNotification(request->port, request->token,
				B_QUERY_UPDATE, request->opcode, nsid, 0, vnida, vnidb,
				vnidc, name);
		}
	} else {
		_EntryCreated(request->directoryID, name,
			(request->entryInfoValid ? &request->entryInfo : NULL),
			request->revision);
		if (error == B_OK)
			NotifyListener(request->opcode, nsid, vnida, vnidb, vnidc, name);
	}
}

// _HandleEntryRemovedRequest
void
ShareVolume::_HandleEntryRemovedRequest(EntryRemovedRequest* request)
{
	if (!request)
		return;

	nspace_id nsid = fVolumeManager->GetID();
	const char* name = request->name.GetString();

	// translate the node IDs
	vnode_id vnida = 0;
	vnode_id vnidb = 0;
	vnode_id vnidc = 0;
	status_t error = _GetLocalNodeID(request->directoryID, &vnida, true);
	if (error == B_OK)
		error = _GetLocalNodeID(request->nodeID, &vnidc, false);
			// TODO: We don't enter a node ID mapping here, which might cause
			// undesired behavior in some cases. Queries should usually be
			// fine, since, if the entry was in the query set, then the
			// respective node ID should definately be known at this point.
			// If an entry is removed and the node monitoring event comes
			// before a live query event for the same entry, the former one
			// will cause the ID to be removed, so that it is already gone
			// when the latter one arrives. I haven't observed this yet,
			// though. The query events always seem to be sent before the
			// node monitoring events (and the server processes them in a
			// single-threaded way). I guess, this is FS implementation
			// dependent, though.
			// A node monitoring event that will always get lost, is when the
			// FS user watches a directory that hasn't been read before. Its
			// entries will not be known yet and thus "entry removed" events
			// will be dropped here. I guess, it's arguable whether this is
			// a practical problem (why should the watcher care that an entry
			// has been removed, when they didn't know what entries were in
			// the directory in the first place?).
			// A possible solution would be to never remove node ID mappings.
			// We would only need to take care that the cached node info is
			// correctly flushed, so that e.g. a subsequent ReadVNode() has to
			// ask the server and doesn't work with obsolete data. We would
			// also enter a yet unknown ID into the node ID map here. The only
			// problem is that the ID maps will keep growing, especially when
			// there's a lot of FS activity on the server.

	// send notifications / do additional processing
	if (request->queryUpdate) {
		if (error == B_OK) {
			SendNotification(request->port, request->token,
				B_QUERY_UPDATE, request->opcode, nsid, 0, vnida, vnidb,
				vnidc, name);
		}
	} else {
		_EntryRemoved(request->directoryID, name, request->revision);
		_NodeRemoved(request->nodeID);
		if (error == B_OK)
			NotifyListener(request->opcode, nsid, vnida, vnidb, vnidc, name);
	}
}

// _HandleEntryMovedRequest
void
ShareVolume::_HandleEntryMovedRequest(EntryMovedRequest* request)
{
	if (!request)
		return;

	nspace_id nsid = fVolumeManager->GetID();
	const char* oldName = request->fromName.GetString();
	const char* name = request->toName.GetString();

	// translate the node IDs
	vnode_id vnida = 0;
	vnode_id vnidb = 0;
	vnode_id vnidc = 0;
	status_t error = _GetLocalNodeID(request->fromDirectoryID, &vnida, true);
	if (error == B_OK)
		error = _GetLocalNodeID(request->toDirectoryID, &vnidb, true);
	if (error == B_OK)
		error = _GetLocalNodeID(request->nodeID, &vnidc, true);

	// send notifications / do additional processing
	if (!request->queryUpdate) {
		_EntryMoved(request->fromDirectoryID, oldName, request->toDirectoryID,
			name, (request->entryInfoValid ? &request->entryInfo : NULL),
			request->revision);
		if (error == B_OK)
			NotifyListener(request->opcode, nsid, vnida, vnidb, vnidc, name);
	}
}

// _HandleStatChangedRequest
void
ShareVolume::_HandleStatChangedRequest(StatChangedRequest* request)
{
	if (!request)
		return;

	nspace_id nsid = fVolumeManager->GetID();

	// translate the node IDs
	vnode_id vnida = 0;
	vnode_id vnidb = 0;
	vnode_id vnidc = 0;
	status_t error = _GetLocalNodeID(request->nodeID, &vnidc, true);

	// send notifications / do additional processing
	if (!request->queryUpdate) {
		_UpdateNode(request->nodeInfo);
		if (error == B_OK)
			NotifyListener(request->opcode, nsid, vnida, vnidb, vnidc, NULL);
	}
}

// _HandleAttributeChangedRequest
void
ShareVolume::_HandleAttributeChangedRequest(AttributeChangedRequest* request)
{
	if (!request)
		return;

	nspace_id nsid = fVolumeManager->GetID();
	const char* name = request->attrInfo.name.GetString();

	// translate the node IDs
	vnode_id vnida = 0;
	vnode_id vnidb = 0;
	vnode_id vnidc = 0;
	status_t error = _GetLocalNodeID(request->nodeID, &vnidc, true);

	// send notifications / do additional processing
	if (!request->queryUpdate) {
		_UpdateAttrDir(request->nodeID, request->attrDirInfo);
		if (error == B_OK)
			NotifyListener(request->opcode, nsid, vnida, vnidb, vnidc, name);
	}
}

// _GetLocalNodeID
status_t
ShareVolume::_GetLocalNodeID(NodeID remoteID, ino_t* _localID, bool enter)
{
	AutoLocker<Locker> _(fLock);
	// if the ID is already know, just return it
	if (fLocalNodeIDs->ContainsKey(remoteID)) {
		*_localID = fLocalNodeIDs->Get(remoteID);
		return B_OK;
	}

	// ID not yet known
	// enter it? Do this only, if requested and we're not already unmounting.
	if (!enter)
		return B_ENTRY_NOT_FOUND;
	if (fUnmounting)
		return ERROR_NOT_CONNECTED;

	// get a fresh ID from the volume manager
	vnode_id localID = fVolumeManager->NewNodeID(this);
	if (localID < 0)
		return localID;

	// put the IDs into local map
	status_t error = fLocalNodeIDs->Put(remoteID, localID);
	if (error != B_OK) {
		fVolumeManager->RemoveNodeID(localID);
		return error;
	}

	// put the IDs into remote map
	error = fRemoteNodeIDs->Put(localID, remoteID);
	if (error != B_OK) {
		fLocalNodeIDs->Remove(remoteID);
		fVolumeManager->RemoveNodeID(localID);
		return error;
	}
	PRINT("ShareVolume(%" B_PRId32 "): added node ID mapping: local: %"
		B_PRIdINO " -> remote: (%" B_PRIdDEV ", %" B_PRIdINO ")\n", fID,
		localID, remoteID.volumeID, remoteID.nodeID);

	*_localID = localID;
	return B_OK;
}

// _GetRemoteNodeID
status_t
ShareVolume::_GetRemoteNodeID(ino_t localID, NodeID* remoteID)
{
	AutoLocker<Locker> _(fLock);

	// check, if the ID is known
	if (!fRemoteNodeIDs->ContainsKey(localID))
		return B_ENTRY_NOT_FOUND;

	*remoteID = fRemoteNodeIDs->Get(localID);
	return B_OK;
}

// _RemoveLocalNodeID
void
ShareVolume::_RemoveLocalNodeID(ino_t localID)
{
	AutoLocker<Locker> _(fLock);

	// check, if the ID is known
	if (!fRemoteNodeIDs->ContainsKey(localID))
		return;

	// remove from ID maps
	NodeID remoteID = fRemoteNodeIDs->Get(localID);
	PRINT("ShareVolume::_RemoveLocalNodeID(%" B_PRIdINO "): remote: (%"
		B_PRIdDEV ", %" B_PRIdINO ")\n", localID, remoteID.volumeID,
		remoteID.nodeID);
	fRemoteNodeIDs->Remove(localID);
	fLocalNodeIDs->Remove(remoteID);

	// remove from volume manager
	fVolumeManager->RemoveNodeID(localID);
}

// _GetNodeByLocalID
ShareNode*
ShareVolume::_GetNodeByLocalID(ino_t localID)
{
	AutoLocker<Locker> _(fLock);
	return fNodes->Get(localID);
}

// _GetNodeByRemoteID
ShareNode*
ShareVolume::_GetNodeByRemoteID(NodeID remoteID)
{
	AutoLocker<Locker> _(fLock);

	ino_t localID;
	if (_GetLocalNodeID(remoteID, &localID, false) == B_OK)
		return fNodes->Get(localID);

	return NULL;
}

// _LoadNode
status_t
ShareVolume::_LoadNode(const NodeInfo& nodeInfo, ShareNode** _node)
{
	AutoLocker<Locker> _(fLock);

	// check, if the node is already known
	ShareNode* node = _GetNodeByRemoteID(nodeInfo.GetID());
	if (node) {
		node->Update(nodeInfo);
	} else {
		// don't load the node when already unmounting
		if (fUnmounting)
			return B_ERROR;

		// get a local node ID
		vnode_id localID;
		status_t error = _GetLocalNodeID(nodeInfo.GetID(), &localID, true);
		if (error != B_OK)
			return error;

		// create a new node
		if (S_ISDIR(nodeInfo.st.st_mode))
			node = new(std::nothrow) ShareDir(this, localID, &nodeInfo);
		else
			node = new(std::nothrow) ShareNode(this, localID, &nodeInfo);
		if (!node) {
			_RemoveLocalNodeID(localID);
			return B_NO_MEMORY;
		}

		// add it
		error = fNodes->Put(node->GetID(), node);
		if (error != B_OK) {
			_RemoveLocalNodeID(localID);
			delete node;
			return error;
		}
		PRINT("ShareVolume: added node: %" B_PRIdINO ": remote: (%" B_PRIdDEV
			", %" B_PRIdINO "), localID: %" B_PRIdINO "\n", node->GetID(),
			node->GetRemoteID().volumeID,
			node->GetRemoteID().nodeID, localID);
	}

	if (_node)
		*_node = node;
	return B_OK;
}

// _UpdateNode
status_t
ShareVolume::_UpdateNode(const NodeInfo& nodeInfo)
{
	AutoLocker<Locker> _(fLock);

	if (fUnmounting)
		return ERROR_NOT_CONNECTED;

	ShareNode* node = _GetNodeByRemoteID(nodeInfo.GetID());
	if (node) {
		node->Update(nodeInfo);
		return B_OK;
	}
	return B_ENTRY_NOT_FOUND;
}

// _GetEntryByLocalID
ShareDirEntry*
ShareVolume::_GetEntryByLocalID(ino_t localDirID, const char* name)
{
	if (!name)
		return NULL;

	AutoLocker<Locker> _(fLock);
	return fEntries->Get(EntryKey(localDirID, name));
}

// _GetEntryByRemoteID
ShareDirEntry*
ShareVolume::_GetEntryByRemoteID(NodeID remoteDirID, const char* name)
{
	if (!name)
		return NULL;

	AutoLocker<Locker> _(fLock);

	ino_t localDirID;
	if (_GetLocalNodeID(remoteDirID, &localDirID, false) == B_OK)
		return fEntries->Get(EntryKey(localDirID, name));

	return NULL;
}

// _LoadEntry
//
// If _entry is supplied, fLock should be held, otherwise the returned entry
// might as well be deleted.
status_t
ShareVolume::_LoadEntry(ShareDir* directory, const EntryInfo& entryInfo,
	ShareDirEntry** _entry)
{
	const char* name = entryInfo.name.GetString();
	if (!directory || !name)
		return B_BAD_VALUE;

	AutoLocker<Locker> _(fLock);

	ShareDirEntry* entry = _GetEntryByLocalID(directory->GetID(), name);
	if (entry) {
		if (entryInfo.nodeInfo.revision > entry->GetRevision()) {
			if (entryInfo.nodeInfo.GetID() != entry->GetNode()->GetRemoteID()) {
				// The node the existing entry refers to is not the node it
				// should refer to. Remove the old entry and create a new one.
				_EntryRemoved(directory->GetRemoteID(), name,
					entryInfo.nodeInfo.revision);
				_EntryCreated(directory->GetRemoteID(), name, &entryInfo,
					entryInfo.nodeInfo.revision);

				// re-get the entry and check, if everything is fine
				entry = _GetEntryByLocalID(directory->GetID(), name);
				if (!entry)
					return B_ERROR;
				if (entryInfo.nodeInfo.GetID()
					!= entry->GetNode()->GetRemoteID()) {
					return B_ERROR;
				}
			} else {
				entry->SetRevision(entryInfo.nodeInfo.revision);
				_UpdateNode(entryInfo.nodeInfo);
			}
		}
	} else {
		// entry not known yet: create it

		// don't load the entry when already unmounting
		if (fUnmounting)
			return B_ERROR;

		// load the node
		ShareNode* node;
		status_t error = _LoadNode(entryInfo.nodeInfo, &node);
		if (error != B_OK)
			return error;

		// if the directory or the node are marked remove, we don't create the
		// entry
		if (IsVNodeRemoved(directory->GetID()) > 0
			|| IsVNodeRemoved(node->GetID()) > 0) {
			return B_NOT_ALLOWED;
		}

		// create the entry
		entry = new(std::nothrow) ShareDirEntry(directory, name, node);
		if (!entry)
			return B_NO_MEMORY;
		ObjectDeleter<ShareDirEntry> entryDeleter(entry);
		error = entry->InitCheck();
		if (error != B_OK)
			return error;

		// add the entry
		error = fEntries->Put(EntryKey(directory->GetID(), entry->GetName()),
			entry);
		if (error != B_OK)
			return error;

		// set the entry revision
		entry->SetRevision(entryInfo.nodeInfo.revision);

		// add the entry to the directory and the node
		directory->AddEntry(entry);
		entry->GetNode()->AddReferringEntry(entry);

		// everything went fine
		entryDeleter.Detach();
	}

	if (_entry)
		*_entry = entry;
	return B_OK;
}

// _RemoveEntry
//
// fLock must be held.
void
ShareVolume::_RemoveEntry(ShareDirEntry* entry)
{
	fEntries->Remove(EntryKey(entry->GetDirectory()->GetID(),
		entry->GetName()));
	entry->GetDirectory()->RemoveEntry(entry);
	entry->GetNode()->RemoveReferringEntry(entry);
	entry->ReleaseReference();
}

// _IsObsoleteEntryInfo
//
// fLock must be held.
bool
ShareVolume::_IsObsoleteEntryInfo(const EntryInfo& entryInfo)
{
	// get the directory
	ShareDir* dir
		= dynamic_cast<ShareDir*>(_GetNodeByRemoteID(entryInfo.directoryID));
	if (!dir)
		return false;

	return (entryInfo.nodeInfo.revision <= dir->GetEntryRemovedEventRevision());
}

// _LoadAttrDir
status_t
ShareVolume::_LoadAttrDir(ShareNode* node, const AttrDirInfo& attrDirInfo)
{
	if (!node || !attrDirInfo.isValid)
		return B_BAD_VALUE;

	AutoLocker<Locker> _(fLock);

	if (fUnmounting)
		return ERROR_NOT_CONNECTED;

	ShareAttrDir* attrDir = node->GetAttrDir();
	if (attrDir) {
		if (attrDir->GetRevision() > attrDirInfo.revision)
			return B_OK;

		// update the attr dir
		return attrDir->Update(attrDirInfo,
			fAttrDirIterators->Get(node->GetID()));
	} else {
		// no attribute directory yet: create one
		attrDir = new(std::nothrow) ShareAttrDir;
		if (!attrDir)
			return B_NO_MEMORY;
		ObjectDeleter<ShareAttrDir> attrDirDeleter(attrDir);

		// initialize it
		status_t error = attrDir->Init(attrDirInfo);
		if (error != B_OK)
			return error;

		// set it
		node->SetAttrDir(attrDir);
		attrDirDeleter.Detach();
		return B_OK;
	}
}

// _UpdateAttrDir
status_t
ShareVolume::_UpdateAttrDir(NodeID remoteID, const AttrDirInfo& attrDirInfo)
{
	AutoLocker<Locker> _(fLock);

	// get the node
	ShareNode* node = _GetNodeByRemoteID(remoteID);
	if (!node)
		return B_ENTRY_NOT_FOUND;

	if (!attrDirInfo.isValid || _LoadAttrDir(node, attrDirInfo) != B_OK) {
		// updating/creating the attr dir failed; if existing, we mark it
		// obsolete
		if (ShareAttrDir* attrDir = node->GetAttrDir())
			attrDir->SetUpToDate(false);
	}

	return B_OK;
}

// _AddAttrDirIterator
status_t
ShareVolume::_AddAttrDirIterator(ShareNode* node,
	ShareAttrDirIterator* iterator)
{
	if (!node || !iterator)
		return B_BAD_VALUE;

	AutoLocker<Locker> locker(fLock);

	// get the iterator list
	DoublyLinkedList<ShareAttrDirIterator>* iteratorList
		= fAttrDirIterators->Get(node->GetID());
	if (!iteratorList) {
		// no list for the node yet: create one
		iteratorList = new(std::nothrow) DoublyLinkedList<ShareAttrDirIterator>;
		if (!iteratorList)
			return B_NO_MEMORY;

		// add it
		status_t error = fAttrDirIterators->Put(node->GetID(), iteratorList);
		if (error != B_OK) {
			delete iteratorList;
			return error;
		}
	}

	// add the iterator
	iteratorList->Insert(iterator);

	return B_OK;
}

// _RemoveAttrDirIterator
void
ShareVolume::_RemoveAttrDirIterator(ShareNode* node,
	ShareAttrDirIterator* iterator)
{
	if (!node || !iterator)
		return;

	AutoLocker<Locker> locker(fLock);

	// get the iterator list
	DoublyLinkedList<ShareAttrDirIterator>* iteratorList
		= fAttrDirIterators->Get(node->GetID());
	if (!iteratorList) {
		WARN("ShareVolume::_RemoveAttrDirIterator(): Iterator list not "
			"found: node: %" B_PRIdINO "\n", node->GetID());
		return;
	}

	// remove the iterator
	iteratorList->Remove(iterator);

	// if the list is empty now, discard it
	if (!iteratorList->First()) {
		fAttrDirIterators->Remove(node->GetID());
		delete iteratorList;
	}
}

// _NodeRemoved
void
ShareVolume::_NodeRemoved(NodeID remoteID)
{
	AutoLocker<Locker> locker(fLock);

	ShareNode* node = _GetNodeByRemoteID(remoteID);
	if (!node)
		return;

	// if the node still has referring entries, we do nothing
	if (node->GetActualReferringEntry())
		return;

	// if the node is a directory, we remove its entries first
	if (ShareDir* dir = dynamic_cast<ShareDir*>(node)) {
		while (ShareDirEntry* entry = dir->GetFirstEntry())
			_RemoveEntry(entry);
	}

	// remove all entries still referring to the node
	while (ShareDirEntry* entry = node->GetFirstReferringEntry())
		_RemoveEntry(entry);

	ino_t localID = node->GetID();

	// Remove the node ID in all cases -- even, if the node is still
	// known to the VFS. Otherwise there could be a race condition, that the
	// server re-uses the remote ID and we have it still associated with an
	// obsolete local ID.
	_RemoveLocalNodeID(localID);

	if (node->IsKnownToVFS()) {
		Node* _node;
		if (GetVNode(localID, &_node) != B_OK)
			return;
		Volume::RemoveVNode(localID);
		locker.Unlock();
		PutVNode(localID);

	} else {
		fNodes->Remove(localID);
		delete node;
	}
}

// _EntryCreated
void
ShareVolume::_EntryCreated(NodeID remoteDirID, const char* name,
	const EntryInfo* entryInfo, int64 revision)
{
	if (!name)
		return;

	AutoLocker<Locker> locker(fLock);

	// get the directory
	ShareDir* dir = dynamic_cast<ShareDir*>(_GetNodeByRemoteID(remoteDirID));
	if (!dir)
		return;

	// load the entry, if possible
	ShareDirEntry* entry;
	bool entryLoaded = false;
	if (entryInfo && _LoadEntry(dir, *entryInfo, &entry) == B_OK)
		entryLoaded = true;

	// if the entry could not be loaded, we have to mark the dir incomplete
	// and update its revision counter
	if (!entryLoaded) {
		dir->UpdateEntryCreatedEventRevision(revision);
		dir->SetComplete(false);
	}
}

// _EntryRemoved
void
ShareVolume::_EntryRemoved(NodeID remoteDirID, const char* name, int64 revision)
{
	if (!name)
		return;

	AutoLocker<Locker> locker(fLock);

	// get the directory
	ShareDir* dir = dynamic_cast<ShareDir*>(_GetNodeByRemoteID(remoteDirID));
	if (!dir)
		return;

	// update the directory's "entry removed" event revision
	dir->UpdateEntryRemovedEventRevision(revision);

	// get the entry
	ShareDirEntry* entry = _GetEntryByRemoteID(remoteDirID, name);
	if (!entry)
		return;

	// check the entry revision
	if (entry->GetRevision() > revision)
		return;

	// remove the entry
	_RemoveEntry(entry);
}

// _EntryMoved
void
ShareVolume::_EntryMoved(NodeID remoteOldDirID, const char* oldName,
	NodeID remoteNewDirID, const char* name, const EntryInfo* entryInfo,
	int64 revision)
{
	AutoLocker<Locker> locker(fLock);

	_EntryRemoved(remoteOldDirID, oldName, revision);
	_EntryCreated(remoteNewDirID, name, entryInfo, revision);
}

// _Walk
status_t
ShareVolume::_Walk(NodeID remoteDirID, const char* entryName, bool resolveLink,
	WalkReply** _reply)
{
	// prepare the request
	WalkRequest request;
	request.volumeID = fID;
	request.nodeID = remoteDirID;
	request.name.SetTo(entryName);
	request.resolveLink = resolveLink;

	// send the request
	WalkReply* reply;
	status_t error = SendRequest(fConnection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);
	if (reply->error != B_OK)
		RETURN_ERROR(reply->error);

	replyDeleter.Detach();
	*_reply = reply;
	return B_OK;
}

// _MultiWalk
status_t
ShareVolume::_MultiWalk(RequestMemberArray<EntryInfo>& _entryInfos,
	MultiWalkReply** _reply)
{
	int32 count = _entryInfos.CountElements();
	if (!_reply || count == 0)
		return B_BAD_VALUE;

	EntryInfo* entryInfos = _entryInfos.GetElements();

	// prepare the request
	MultiWalkRequest request;
	request.volumeID = fID;
	request.nodeID = entryInfos[0].directoryID;

	// add the names
	for (int32 i = 0; i < count; i++) {
		StringData name;
		name.SetTo(entryInfos[i].name.GetString());

		status_t error = request.names.Append(name);
		if (error != B_OK)
			return error;
	}

	// send the request
	MultiWalkReply* reply;
	status_t error = SendRequest(fConnection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);
	if (reply->error != B_OK)
		RETURN_ERROR(reply->error);

	replyDeleter.Detach();
	*_reply = reply;
	return B_OK;
}

// _Close
status_t
ShareVolume::_Close(intptr_t cookie)
{
	if (!_EnsureShareMounted())
		return ERROR_NOT_CONNECTED;

	// prepare the request
	CloseRequest request;
	request.volumeID = fID;
	request.cookie = cookie;
	// send the request
	CloseReply* reply;
	status_t error = SendRequest(fConnection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);
	if (reply->error != B_OK)
		RETURN_ERROR(reply->error);
	return B_OK;
}

// _GetConnectionState
//
// Must not be called with fLock being held!
uint32
ShareVolume::_GetConnectionState()
{
	AutoLocker<Locker> _(fMountLock);
	return fConnectionState;
}

// _IsConnected
//
// Must not be called with fLock being held!
bool
ShareVolume::_IsConnected()
{
	return (_GetConnectionState() == CONNECTION_READY);
}

// _EnsureShareMounted
//
// Must not be called with fLock being held!
bool
ShareVolume::_EnsureShareMounted()
{
	AutoLocker<Locker> _(fMountLock);
	if (fConnectionState == CONNECTION_NOT_INITIALIZED)
		_MountShare();

	return (fConnectionState == CONNECTION_READY);
}

// _MountShare
//
// fMountLock must be held.
status_t
ShareVolume::_MountShare()
{
	// get references to the server and share info
	AutoLocker<Locker> locker(fLock);
	BReference<ExtendedServerInfo> serverInfoReference(fServerInfo);
	BReference<ExtendedShareInfo> shareInfoReference(fShareInfo);
	ExtendedServerInfo* serverInfo = fServerInfo;
	ExtendedShareInfo* shareInfo = fShareInfo;
	locker.Unlock();

	// get server address as string
	HashString serverAddressString;
	status_t error = serverInfo->GetAddress().GetString(&serverAddressString,
		false);
	if (error != B_OK)
		return error;
	const char* server = serverAddressString.GetString();

	// get the server name
	const char* serverName = serverInfo->GetServerName();
	if (serverName && strlen(serverName) == 0)
		serverName = NULL;

	// get the share name
	const char* share = shareInfo->GetShareName();

	PRINT("ShareVolume::_MountShare(%s, %s)\n", server, share);
	// init a connection to the authentication server
	AuthenticationServer authenticationServer;
	error = authenticationServer.InitCheck();
	if (error != B_OK)
		RETURN_ERROR(error);

	// get the server connection
	fConnectionState = CONNECTION_CLOSED;
	if (!fServerConnection) {
		status_t error = fServerConnectionProvider->GetServerConnection(
			&fServerConnection);
		if (error != B_OK)
			return error;
		fConnection = fServerConnection->GetRequestConnection();
	}

	// the mount loop
	bool badPassword = false;
	MountReply* reply = NULL;
	do {
		// get the user and password from the authentication server
		char user[kUserBufferSize];
		char password[kPasswordBufferSize];
		bool cancelled;
		error = authenticationServer.GetAuthentication("netfs",
			(serverName ? serverName : server), share,
			fVolumeManager->GetMountUID(), badPassword,
			&cancelled, user, sizeof(user), password, sizeof(password));
		if (cancelled || error != B_OK)
			RETURN_ERROR(error);

		// prepare the request
		MountRequest request;
		request.share.SetTo(share);
		request.user.SetTo(user);
		request.password.SetTo(password);

		// send the request
		error = SendRequest(fConnection, &request, &reply);
		if (error != B_OK)
			RETURN_ERROR(error);
		ObjectDeleter<Request> replyDeleter(reply);

		// if no permission, try again
		badPassword = reply->noPermission;
		if (!badPassword) {
			if (reply->error != B_OK)
				RETURN_ERROR(reply->error);
			fSharePermissions = reply->sharePermissions;
		}
	} while (badPassword);

	AutoLocker<Locker> _(fLock);

	fID = reply->volumeID;

	// update the root node and enter its ID
	fRootNode->Update(reply->nodeInfo);

	// put the IDs into local map
	error = fLocalNodeIDs->Put(fRootNode->GetRemoteID(), fRootNode->GetID());
	if (error != B_OK)
		RETURN_ERROR(error);

	// put the IDs into remote map
	error = fRemoteNodeIDs->Put(fRootNode->GetID(), fRootNode->GetRemoteID());
	if (error != B_OK) {
		fLocalNodeIDs->Remove(fRootNode->GetRemoteID());
		RETURN_ERROR(error);
	}
	PRINT("ShareVolume::_MountShare(): root node: local: %" B_PRIdINO
		", remote: (%" B_PRIdDEV ", %" B_PRIdINO ")\n", fRootNode->GetID(),
		fRootNode->GetRemoteID().volumeID,
		fRootNode->GetRemoteID().nodeID);

	// Add ourselves to the server connection, so that we can receive
	// node monitoring events. There a race condition: We might already
	// have missed events for the root node.
	error = fServerConnection->AddVolume(this);
	if (error != B_OK) {
		_RemoveLocalNodeID(fRootNode->GetID());
		return error;
	}

	fConnectionState = CONNECTION_READY;
	return B_OK;
}

