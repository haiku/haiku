// RootVolume.cpp

#include "RootVolume.h"

#include <new>

#include <AutoLocker.h>

#include "Compatibility.h"
#include "DebugSupport.h"
#include "ExtendedServerInfo.h"
#include "NetAddress.h"
#include "netfs_ioctl.h"
#include "ServerVolume.h"
#include "TaskManager.h"
#include "VolumeManager.h"
#include "VolumeSupport.h"

static const int32 kOptimalIOSize = 64 * 1024;
static const char* kFSName = "netfs";

// constructor
RootVolume::RootVolume(VolumeManager* volumeManager)
	: VirtualVolume(volumeManager)
{
}

// destructor
RootVolume::~RootVolume()
{
}

// Init
status_t
RootVolume::Init()
{
	status_t error = VirtualVolume::Init("Network");
	if (error != B_OK)
		return error;

	// create and init the server manager
	fServerManager = new(std::nothrow) ServerManager(this);
	if (!fServerManager)
		RETURN_ERROR(B_NO_MEMORY);
	error = fServerManager->Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	return B_OK;
}

// Uninit
void
RootVolume::Uninit()
{
	// delete the server manager
	delete fServerManager;
	fServerManager = NULL;

	VirtualVolume::Uninit();
}

// PrepareToUnmount
void
RootVolume::PrepareToUnmount()
{
	VirtualVolume::PrepareToUnmount();
}


// #pragma mark -
// #pragma mark ----- FS -----

// Mount
status_t
RootVolume::Mount(const char* device, uint32 flags, const char* parameters,
	int32 len)
{
	status_t error = NewVNode(fRootNode->GetID(), fRootNode);
	if (error != B_OK)
		RETURN_ERROR(error);

	// start the server manager
	fServerManager->Run();

	return B_OK;
}

// Unmount
status_t
RootVolume::Unmount()
{
	Uninit();
	return B_OK;
}

// Sync
status_t
RootVolume::Sync()
{
	return B_BAD_VALUE;
}

// ReadFSStat
status_t
RootVolume::ReadFSStat(fs_info* info)
{
	info->flags = B_FS_IS_PERSISTENT | B_FS_HAS_MIME | B_FS_HAS_ATTR
		| B_FS_IS_SHARED | B_FS_HAS_QUERY;
	if (fVolumeManager->GetMountFlags() & B_MOUNT_READ_ONLY)
		info->flags |= B_FS_IS_READONLY;
	info->block_size = 1024;
	info->io_size = kOptimalIOSize;
	info->total_blocks = 0;	// TODO: We could at least fill this in.
	info->free_blocks = LONGLONG_MAX / info->block_size;
		// keep the Tracker happy
	strcpy(info->device_name, "");
	strcpy(info->volume_name, GetName());
	strcpy(info->fsh_name, kFSName);
	return B_OK;
}

// WriteFSStat
status_t
RootVolume::WriteFSStat(struct fs_info* info, int32 mask)
{
	// TODO: Allow editing the volume name.
	return B_BAD_VALUE;
}

// #pragma mark -
// #pragma mark ----- files -----

// IOCtl
status_t
RootVolume::IOCtl(Node* node, void* cookie, int cmd, void* buffer,
	size_t bufferSize)
{
	if (node != fRootNode)
		return B_BAD_VALUE;

	switch (cmd) {
		case NET_FS_IOCTL_ADD_SERVER:
		{
			// check the parameters
			if (!buffer)
				return B_BAD_VALUE;
			netfs_ioctl_add_server* params = (netfs_ioctl_add_server*)buffer;
			int32 serverNameLen = strnlen(params->serverName,
				sizeof(params->serverName));
			if (serverNameLen == 0
				|| serverNameLen == sizeof(params->serverName)) {
				return B_BAD_VALUE;
			}
			PRINT("RootVolume::IOCtl(): NET_FS_IOCTL_ADD_SERVER: "
				"`%s'\n", params->serverName);

			// get the server address
			NetAddress netAddress;
			NetAddressResolver resolver;
			status_t error = resolver.GetHostAddress(params->serverName, &netAddress);
			if (error != B_OK)
				return error;

			// ask the server manager to add the server
			return fServerManager->AddServer(netAddress);
		}
		case NET_FS_IOCTL_REMOVE_SERVER:
		{
			// check the parameters
			if (!buffer)
				return B_BAD_VALUE;
			netfs_ioctl_remove_server* params
				= (netfs_ioctl_remove_server*)buffer;
			int32 serverNameLen = strnlen(params->serverName,
				sizeof(params->serverName));
			if (serverNameLen == 0
				|| serverNameLen == sizeof(params->serverName)) {
				return B_BAD_VALUE;
			}
			PRINT("RootVolume::IOCtl(): NET_FS_IOCTL_REMOVE_SERVER:"
				" `%s'\n", params->serverName);

			// get the server volume
			ServerVolume* serverVolume = _GetServerVolume(params->serverName);
			if (!serverVolume)
				return B_ENTRY_NOT_FOUND;
			VolumePutter volumePutter(serverVolume);

			// ask the server manager to remove the server
			fServerManager->RemoveServer(serverVolume->GetServerAddress());

			return B_OK;
		}
		default:
			PRINT("RootVolume::IOCtl(): unknown ioctl: %d\n", cmd);
			return B_BAD_VALUE;
			break;
	}
	return B_BAD_VALUE;
}


// #pragma mark -

// ServerAdded
void
RootVolume::ServerAdded(ExtendedServerInfo* serverInfo)
{
	PRINT("RootVolume::ServerAdded(%s)\n", serverInfo->GetServerName());
	// check, if the server does already exist
	ServerVolume* serverVolume = _GetServerVolume(serverInfo->GetAddress());
	if (serverVolume) {
		WARN("RootVolume::ServerAdded(): WARNING: ServerVolume does "
			"already exist.\n");
		serverVolume->PutVolume();
		return;
	}

	AutoLocker<Locker> locker(fLock);

	// get a unique name for the server
	char serverName[B_FILE_NAME_LENGTH];
	status_t error = GetUniqueEntryName(serverInfo->GetServerName(),
		serverName);
	if (error != B_OK)
		return;

	// create a server volume
	serverVolume = new(std::nothrow) ServerVolume(fVolumeManager, serverInfo);
	if (!serverVolume)
		return;
	error = serverVolume->Init(serverName);
	if (error != B_OK) {
		delete serverVolume;
		return;
	}

	// add the volume to the volume manager
	error = fVolumeManager->AddVolume(serverVolume);
	if (error != B_OK) {
		delete serverVolume;
		return;
	}
	VolumePutter volumePutter(serverVolume);

	// add the volume to us
	locker.Unlock();
	error = AddChildVolume(serverVolume);
	if (error != B_OK) {
		serverVolume->SetUnmounting(true);
		return;
	}
}

// ServerUpdated
void
RootVolume::ServerUpdated(ExtendedServerInfo* oldInfo,
	ExtendedServerInfo* newInfo)
{
	PRINT("RootVolume::ServerUpdated(%s)\n", newInfo->GetServerName());
	// get the volume
	ServerVolume* serverVolume = _GetServerVolume(newInfo->GetAddress());
	if (!serverVolume)
		return;

	// set the new server info
	VolumePutter _(serverVolume);
	serverVolume->SetServerInfo(newInfo);
}

// ServerRemoved
void
RootVolume::ServerRemoved(ExtendedServerInfo* serverInfo)
{
	PRINT("RootVolume::ServerRemoved(%s)\n", serverInfo->GetServerName());
	// get the volume
	ServerVolume* serverVolume = _GetServerVolume(serverInfo->GetAddress());
	if (!serverVolume)
		return;

	// set it to unmounting
	VolumePutter _(serverVolume);
	serverVolume->SetUnmounting(true);
}


// #pragma mark -

// _GetServerVolume
ServerVolume*
RootVolume::_GetServerVolume(const char* name)
{
	Volume* volume = GetChildVolume(name);
	if (!volume)
		return NULL;
	if (ServerVolume* serverVolume = dynamic_cast<ServerVolume*>(volume))
		return serverVolume;
	fVolumeManager->PutVolume(volume);
	return NULL;
}

// _GetServerVolume
ServerVolume*
RootVolume::_GetServerVolume(const NetAddress& address)
{
	AutoLocker<Locker> locker(fLock);

	// init a directory iterator
	VirtualDirIterator iterator;
	iterator.SetDirectory(fRootNode, true);

	// iterate through the directory
	const char* name;
	Node* node;
	while (iterator.GetCurrentEntry(&name, &node)) {
		iterator.NextEntry();
		ServerVolume* volume = dynamic_cast<ServerVolume*>(node->GetVolume());
		if (volume && volume->GetServerAddress().GetIP() == address.GetIP()) {
			return dynamic_cast<ServerVolume*>(
				fVolumeManager->GetVolume(node->GetID()));
		}
	}

	return NULL;
}

