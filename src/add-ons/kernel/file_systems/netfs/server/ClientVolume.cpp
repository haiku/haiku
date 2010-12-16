// ClientVolume.cpp

#include <new>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <HashMap.h>
#include <Path.h>

#include "ClientVolume.h"
#include "DebugSupport.h"
#include "Directory.h"
#include "Entry.h"
#include "GlobalBlockerPool.h"
#include "NodeHandle.h"
#include "NodeHandleMap.h"
#include "NodeMonitoringEvent.h"
#include "SecurityContext.h"
#include "StatisticsManager.h"
#include "UserSecurityContext.h"
#include "Volume.h"
#include "VolumeManager.h"

// constructor
ClientVolume::ClientVolume(Locker& securityContextLocker,
	NodeMonitoringProcessor* nodeMonitoringProcessor)
	: FSObject(),
	  fID(_NextVolumeID()),
	  fSecurityContext(NULL),
	  fSecurityContextLock(securityContextLocker),
	  fNodeMonitoringProcessor(nodeMonitoringProcessor),
	  fNodeHandles(NULL),
	  fShare(NULL),
	  fRootNodeRef(),
	  fSharePermissions(),
	  fMounted(false)
{
}

// destructor
ClientVolume::~ClientVolume()
{
	Unmount();

	if (fShare)
		fShare->ReleaseReference();

	delete fNodeHandles;
	delete fSecurityContext;
}

// Init
status_t
ClientVolume::Init()
{
	// create the node handle map
	fNodeHandles = new(std::nothrow) NodeHandleMap("node handles");
	if (!fNodeHandles)
		return B_NO_MEMORY;
	status_t error = fNodeHandles->Init();
	if (error != B_OK)
		return error;

	return B_OK;
}

// GetID
int32
ClientVolume::GetID() const
{
	return fID;
}

// Mount
status_t
ClientVolume::Mount(UserSecurityContext* securityContext, Share* share)
{
	if (!securityContext || !share)
		return B_BAD_VALUE;
	ObjectDeleter<UserSecurityContext> securityContextDeleter(securityContext);
	if (IsMounted())
		return B_BAD_VALUE;
	fSecurityContext = securityContext;
	securityContextDeleter.Detach();

	fShare = share;
	fShare->AcquireReference();
	dev_t volumeID = share->GetVolumeID();
	ino_t nodeID = share->GetNodeID();

	// into root node ref
	fRootNodeRef.device = volumeID;
	fRootNodeRef.node = nodeID;

	// get the share permissions
	fSharePermissions = securityContext->GetNodePermissions(volumeID, nodeID);

	// get the root directory
	VolumeManager* volumeManager = VolumeManager::GetDefault();
	Directory* rootDir;
	status_t error = volumeManager->LoadDirectory(volumeID, nodeID, &rootDir);
	if (error != B_OK)
		return error;

	// register with the volume manager
	error = volumeManager->AddClientVolume(this);
	if (error != B_OK) {
		Unmount();
		return error;
	}
	fMounted = true;

	// notify the statistics manager
	StatisticsManager::GetDefault()->ShareMounted(fShare,
		fSecurityContext->GetUser());

	return B_OK;
}

// Unmount
void
ClientVolume::Unmount()
{
	PRINT(("ClientVolume::Unmount()\n"));

	if (fMounted) {
		fMounted = false;

		// notify the statistics manager
		StatisticsManager::GetDefault()->ShareUnmounted(fShare,
			fSecurityContext->GetUser());
	}

	// remove ourselves from the volume manager
	VolumeManager::GetDefault()->RemoveClientVolume(this);

	// close all node handles
//	while (true) {
//		// get a cookie
//		int32 cookie;
//		{
//			NodeHandleMap::Iterator it = fNodeHandles->GetIterator();
//			if (!it.HasNext())
//				break;
//			cookie = it.Next().key.value;
//		}
//
//		// get the handle
//		NodeHandle* handle;
//		status_t error = LockNodeHandle(cookie, &handle);
//		if (error == B_OK) {
//			// close the node handle
//			ClientNodeUnlocker _(handle->GetClientNode());
//			Close(handle);
//		} else {
//			ClientVolumeLocker _(this);
//			if (fNodeHandles->ContainsKey(cookie)) {
//				// something went seriously wrong
//				ERROR(("ClientVolume::Unmount(): ERROR: Failed to lock "
//					"existing node handle! Can't continue Unmount().\n"));
//				return;
//			}
//		}
//	}
}


// IsMounted
bool
ClientVolume::IsMounted() const
{
	return fMounted;
}

// GetSecurityContext
//
// Caller must hold fSecurityContextLock. Only the ClientConnection should
// do this.
UserSecurityContext*
ClientVolume::GetSecurityContext() const
{
	return fSecurityContext;
}

// SetSecurityContext
void
ClientVolume::SetSecurityContext(UserSecurityContext* securityContext)
{
	AutoLocker<Locker> locker(fSecurityContextLock);

	// unset old
	delete fSecurityContext;

	// set new
	fSecurityContext = securityContext;
	fSharePermissions = fSecurityContext->GetNodePermissions(fRootNodeRef);
}

// GetShare
Share*
ClientVolume::GetShare() const
{
	return fShare;
}

// GetRootDirectory
Directory*
ClientVolume::GetRootDirectory() const
{
	return VolumeManager::GetDefault()->GetDirectory(
		fRootNodeRef.device, fRootNodeRef.node);
}

// GetRootNodeRef
const NodeRef&
ClientVolume::GetRootNodeRef() const
{
	return fRootNodeRef;
}

// GetSharePermissions
Permissions
ClientVolume::GetSharePermissions() const
{
	return fSharePermissions;
}

// GetNodePermissions
Permissions
ClientVolume::GetNodePermissions(dev_t volumeID, ino_t nodeID)
{
	return fSharePermissions;
}

// GetNodePermissions
Permissions
ClientVolume::GetNodePermissions(Node* node)
{
// TODO: We should also check whether the node is located on the client volume
// in the first place. Otherwise someone with access to a low-security share
// could get access to arbitrary nodes on the server.
	return fSharePermissions;
}

// GetNode
Node*
ClientVolume::GetNode(dev_t volumeID, ino_t nodeID)
{
	VolumeManager* volumeManager = VolumeManager::GetDefault();

	// get the node
	Node* node = volumeManager->GetNode(volumeID, nodeID);
	if (!node)
		return NULL;

	// check, if the node is contained by the root dir of the client volume
	if (volumeManager->DirectoryContains(GetRootDirectory(), node, true))
		return node;

	return NULL;
}

// GetNode
Node*
ClientVolume::GetNode(NodeID nodeID)
{
	return GetNode(nodeID.volumeID, nodeID.nodeID);
}

// GetNode
Node*
ClientVolume::GetNode(const node_ref& nodeRef)
{
	return GetNode(nodeRef.device, nodeRef.node);
}

// GetDirectory
Directory*
ClientVolume::GetDirectory(dev_t volumeID, ino_t nodeID)
{
	VolumeManager* volumeManager = VolumeManager::GetDefault();

	// get the directory
	Directory* dir = GetDirectory(volumeID, nodeID);
	if (!dir)
		return NULL;

	// check, if the dir is contained by the root dir of the client volume
	if (volumeManager->DirectoryContains(GetRootDirectory(), dir, true))
		return dir;

	return NULL;
}

// GetDirectory
Directory*
ClientVolume::GetDirectory(NodeID nodeID)
{
	return GetDirectory(nodeID.volumeID, nodeID.nodeID);
}

// LoadDirectory
status_t
ClientVolume::LoadDirectory(dev_t volumeID, ino_t nodeID,
	Directory** _directory)
{
	if (!_directory)
		return B_BAD_VALUE;

	VolumeManager* volumeManager = VolumeManager::GetDefault();

	// load the directory
	Directory* dir;
	status_t error = volumeManager->LoadDirectory(volumeID, nodeID, &dir);
	if (error != B_OK)
		return error;

	// check, if the dir is contained by the root dir of the client volume
	if (!volumeManager->DirectoryContains(GetRootDirectory(), dir, true))
		return B_ENTRY_NOT_FOUND;

	*_directory = dir;
	return B_OK;
}

// GetEntry
Entry*
ClientVolume::GetEntry(dev_t volumeID, ino_t dirID, const char* name)
{
	VolumeManager* volumeManager = VolumeManager::GetDefault();

	// get the entry
	Entry* entry = volumeManager->GetEntry(volumeID, dirID, name);
	if (!entry)
		return NULL;

	// check, if the entry is contained by the root dir of the client volume
	if (volumeManager->DirectoryContains(GetRootDirectory(), entry))
		return entry;

	return NULL;
}

// GetEntry
Entry*
ClientVolume::GetEntry(Directory* directory, const char* name)
{
	if (!directory)
		return NULL;

	return GetEntry(directory->GetVolumeID(), directory->GetID(), name);
}

// LoadEntry
status_t
ClientVolume::LoadEntry(dev_t volumeID, ino_t dirID, const char* name,
	Entry** _entry)
{
	if (!name || !_entry)
		return B_BAD_VALUE;

	VolumeManager* volumeManager = VolumeManager::GetDefault();

	// get the entry
	Entry* entry;
	status_t error = VolumeManager::GetDefault()->LoadEntry(volumeID, dirID,
		name, true, &entry);
	if (error != B_OK)
		return error;

	// check, if the entry is contained by the root dir of the client volume
	if (!volumeManager->DirectoryContains(GetRootDirectory(), entry))
		return B_ENTRY_NOT_FOUND;

	*_entry = entry;
	return B_OK;
}

// LoadEntry
status_t
ClientVolume::LoadEntry(Directory* directory, const char* name, Entry** entry)
{
	if (!directory)
		return B_BAD_VALUE;

	return LoadEntry(directory->GetVolumeID(), directory->GetID(), name, entry);
}

// Open
//
// The caller gets a lock to the returned node handle.
status_t
ClientVolume::Open(Node* node, int openMode, FileHandle** _handle)
{
	if (!node || !_handle)
		return B_BAD_VALUE;

	// open the node
	FileHandle* handle = NULL;
	status_t error = node->Open(openMode, &handle);
	if (error != B_OK)
		return error;
	BReference<NodeHandle> handleReference(handle, true);

	// lock the handle
	handle->Lock();

	// add the handle
	error = fNodeHandles->AddNodeHandle(handle);
	if (error != B_OK)
		return error;

	handleReference.Detach();
	*_handle = handle;
	return B_OK;
}

// OpenDir
//
// The caller gets a lock to the returned node handle.
status_t
ClientVolume::OpenDir(Directory* directory, DirIterator** _iterator)
{
	if (!directory || !_iterator)
		return B_BAD_VALUE;

	// open the directory
	DirIterator* iterator = NULL;
	status_t error = directory->OpenDir(&iterator);
	if (error != B_OK)
		return error;
	BReference<NodeHandle> handleReference(iterator, true);

	// lock the handle
	iterator->Lock();

	// add the handle
	error = fNodeHandles->AddNodeHandle(iterator);
	if (error != B_OK)
		return error;

	handleReference.Detach();
	*_iterator = iterator;
	return B_OK;
}

// OpenAttrDir
//
// The caller gets a lock to the returned node handle.
status_t
ClientVolume::OpenAttrDir(Node* node, AttrDirIterator** _iterator)
{
	if (!node || !_iterator)
		return B_BAD_VALUE;

	// open the attribut directory
	AttrDirIterator* iterator = NULL;
	status_t error = node->OpenAttrDir(&iterator);
	if (error != B_OK)
		return error;
	BReference<NodeHandle> handleReference(iterator, true);

	// lock the handle
	iterator->Lock();

	// add the handle
	error = fNodeHandles->AddNodeHandle(iterator);
	if (error != B_OK)
		return error;

	handleReference.Detach();
	*_iterator = iterator;
	return B_OK;
}

// Close
//
// VolumeManager MUST be locked. After closing the handle must still be
// unlocked. When the last reference is surrendered it will finally be deleted.
status_t
ClientVolume::Close(NodeHandle* handle)
{
	if (!handle || !fNodeHandles->RemoveNodeHandle(handle))
		return B_BAD_VALUE;

	return B_OK;
}

// LockNodeHandle
//
// VolumeManager must NOT be locked.
status_t
ClientVolume::LockNodeHandle(int32 cookie, NodeHandle** _handle)
{
	return fNodeHandles->LockNodeHandle(cookie, _handle);
}

// UnlockNodeHandle
//
// VolumeManager may or may not be locked.
void
ClientVolume::UnlockNodeHandle(NodeHandle* nodeHandle)
{
	fNodeHandles->UnlockNodeHandle(nodeHandle);
}

// ProcessNodeMonitoringEvent
void
ClientVolume::ProcessNodeMonitoringEvent(NodeMonitoringEvent* event)
{
	if (fNodeMonitoringProcessor)
		fNodeMonitoringProcessor->ProcessNodeMonitoringEvent(fID, event);
}

// _NextVolumeID
int32
ClientVolume::_NextVolumeID()
{
	return atomic_add(&sNextVolumeID, 1);
}

// sNextVolumeID
vint32 ClientVolume::sNextVolumeID = 0;


// #pragma -

// destructor
ClientVolume::NodeMonitoringProcessor::~NodeMonitoringProcessor()
{
}

