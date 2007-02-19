// FileSystem.cpp

#include "AutoLocker.h"
#include "Compatibility.h"
#include "Debug.h"
#include "FileSystem.h"
#include "HashMap.h"
#include "KernelRequestHandler.h"
#include "PortReleaser.h"
#include "RequestAllocator.h"
#include "RequestPort.h"
#include "Requests.h"
#include "Settings.h"
#include "SingleReplyRequestHandler.h"
#include "Volume.h"

// The time after which the notification thread times out at the port and
// restarts the loop. Of interest only when the FS is deleted. It is the
// maximal time the destructor has to wait for the thread.
static const bigtime_t kNotificationRequestTimeout = 50000;	// 50 ms

// SelectSyncMap
struct FileSystem::SelectSyncMap
	: public SynchronizedHashMap<HashKey32<selectsync*>, int32*> {
};

// constructor
FileSystem::FileSystem(const char* name, RequestPort* initPort, status_t* error)
	: LazyInitializable(),
	  Referencable(),
	  fVolumes(),
	  fVolumeLock(),
	  fName(name),
	  fInitPort(initPort),
	  fNotificationPort(NULL),
	  fNotificationThread(-1),
	  fPortPool(),
	  fSelectSyncs(NULL),
	  fSettings(NULL),
	  fUserlandServerTeam(-1),
	  fTerminating(false)
{
	if (error)
		*error = (fName.GetLength() == 0 ? B_NO_MEMORY : B_OK);
}

// destructor
FileSystem::~FileSystem()
{
	fTerminating = true;
	// wait for the notification thread to terminate
	if (fNotificationThread >= 0) {
		int32 result;
		wait_for_thread(fNotificationThread, &result);
	}
	// delete our data structures
	if (fSelectSyncs) {
		for (SelectSyncMap::Iterator it = fSelectSyncs->GetIterator();
			 it.HasNext();) {
			SelectSyncMap::Entry entry = it.Next();
			delete entry.value;
		}
		delete fSelectSyncs;
	}
	delete fSettings;
}

// GetName
const char*
FileSystem::GetName() const
{
	return fName.GetString();
}

// GetPortPool
RequestPortPool*
FileSystem::GetPortPool()
{
	return &fPortPool;
}

// Mount
status_t
FileSystem::Mount(nspace_id id, const char* device, ulong flags,
	const char* parameters, int32 len, Volume** _volume)
{
	// check initialization and parameters
	if (InitCheck() != B_OK)
		return InitCheck();
	if (!_volume)
		return B_BAD_VALUE;
	// create volume
	Volume* volume = new(nothrow) Volume(this, id);
	if (!volume)
		return B_NO_MEMORY;
	// add volume to the volume list
	fVolumeLock.Lock();
	status_t error = fVolumes.PushBack(volume);
	fVolumeLock.Unlock();
	if (error != B_OK)
		return error;
	// mount volume
	error = volume->Mount(device, flags, parameters, len);
	if (error != B_OK) {
		fVolumeLock.Lock();
		fVolumes.Remove(volume);
		fVolumeLock.Unlock();
		volume->RemoveReference();
		return error;
	}
	*_volume = volume;
	return error;
}

// Initialize
status_t
FileSystem::Initialize(const char* deviceName, const char* parameters,
	size_t len)
{
	// get a free port
	RequestPort* port = fPortPool.AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(&fPortPool, port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	MountVolumeRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	error = allocator.AllocateString(request->device, deviceName);
	if (error == B_OK)
		error = allocator.AllocateData(request->parameters, parameters, len, 1);
	if (error != B_OK)
		return error;
	// send the request
	SingleReplyRequestHandler handler(MOUNT_VOLUME_REPLY);
	InitializeVolumeReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// VolumeUnmounted
void
FileSystem::VolumeUnmounted(Volume* volume)
{
	fVolumeLock.Lock();
	fVolumes.Remove(volume);
	fVolumeLock.Unlock();
}

// GetVolume
Volume*
FileSystem::GetVolume(nspace_id id)
{
	AutoLocker<Locker> _(fVolumeLock);
	for (Vector<Volume*>::Iterator it = fVolumes.Begin();
		 it != fVolumes.End();
		 it++) {
		 Volume* volume = *it;
		 if (volume->GetID() == id) {
		 	volume->AddReference();
		 	return volume;
		 }
	}
	return NULL;
}

// GetIOCtlInfo
const IOCtlInfo*
FileSystem::GetIOCtlInfo(int command) const
{
	return (fSettings ? fSettings->GetIOCtlInfo(command) : NULL);
}

// AddSelectSyncEntry
status_t
FileSystem::AddSelectSyncEntry(selectsync* sync)
{
	AutoLocker<SelectSyncMap> _(fSelectSyncs);
	int32* count = fSelectSyncs->Get(sync);
	if (!count) {
		count = new(nothrow) int32(0);
		if (!count)
			return B_NO_MEMORY;
		status_t error = fSelectSyncs->Put(sync, count);
		if (error != B_OK) {
			delete count;
			return error;
		}
	}
	(*count)++;
	return B_OK;
}

// RemoveSelectSyncEntry
void
FileSystem::RemoveSelectSyncEntry(selectsync* sync)
{
	AutoLocker<SelectSyncMap> _(fSelectSyncs);
	if (int32* count = fSelectSyncs->Get(sync)) {
		if (--(*count) <= 0) {
			fSelectSyncs->Remove(sync);
			delete count;
		}
	}
}

// KnowsSelectSyncEntry
bool
FileSystem::KnowsSelectSyncEntry(selectsync* sync)
{
	return fSelectSyncs->ContainsKey(sync);
}

// IsUserlandServerThread
bool
FileSystem::IsUserlandServerThread() const
{
	thread_info info;
	get_thread_info(find_thread(NULL), &info);
	return (info.team == fUserlandServerTeam);
}

// FirstTimeInit
status_t
FileSystem::FirstTimeInit()
{
	if (fName.GetLength() == 0)
		RETURN_ERROR(B_NO_MEMORY);
	PRINT(("FileSystem::FirstTimeInit(): %s\n", fName.GetString()));
	// create the select sync entry map
	fSelectSyncs = new(nothrow) SelectSyncMap;
	if (!fSelectSyncs)
		return B_NO_MEMORY;
	// prepare the request
	RequestAllocator allocator(fInitPort->GetPort());
	FSConnectRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		RETURN_ERROR(error);
	error = allocator.AllocateString(request->fsName, fName.GetString());
	if (error != B_OK)
		RETURN_ERROR(error);
	// send the request
	SingleReplyRequestHandler handler(FS_CONNECT_REPLY);
	FSConnectReply* reply;
	error = fInitPort->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	RequestReleaser requestReleaser(fInitPort, reply);
	// process the reply
	if (reply->error != B_OK)
		RETURN_ERROR(reply->error);
	// get the port infos
	int32 count = reply->portInfoCount;
	if (count < 2)
		RETURN_ERROR(B_BAD_DATA);
	if (reply->portInfos.GetSize() != count * (int32)sizeof(Port::Info))
		RETURN_ERROR(B_BAD_DATA);
	Port::Info* infos = (Port::Info*)reply->portInfos.GetData();
	// create the request ports
	// the notification port
	fNotificationPort = new(nothrow) RequestPort(infos);
	if (!fNotificationPort)
		RETURN_ERROR(B_NO_MEMORY);
	error = fNotificationPort->InitCheck();
	if (error != B_OK)
		return error;
	// the other request ports
	for (int32 i = 1; i < count; i++) {
		RequestPort* port = new(nothrow) RequestPort(infos + i);
		if (!port)
			RETURN_ERROR(B_NO_MEMORY);
		error = port->InitCheck();
		if (error == B_OK)
			error = fPortPool.AddPort(port);
		if (error != B_OK) {
			delete port;
			RETURN_ERROR(error);
		}
	}
	// get the userland team
	port_info portInfo;
	error = get_port_info(infos[0].owner_port, &portInfo);
	if (error != B_OK)
		RETURN_ERROR(error);
	fUserlandServerTeam = portInfo.team;
	// print some info about the userland team
	D(
		PRINT(("  userland team is: %ld\n", fUserlandServerTeam));
		int32 cookie = 0;
		thread_info threadInfo;
		while (get_next_thread_info(fUserlandServerTeam, &cookie, &threadInfo)
			   == B_OK) {
			PRINT(("    userland thread: %ld: `%s'\n", threadInfo.thread,
				threadInfo.name));
		}
	);
	// load the settings
	fSettings = new(nothrow) Settings;
	if (fSettings) {
		status_t settingsError = fSettings->SetTo(fName.GetString());
		if (settingsError != B_OK) {
			PRINT(("Failed to load settings: %s\n", strerror(settingsError)));
			delete fSettings;
			fSettings = NULL;
		} else
			fSettings->Dump();
	} else
		ERROR(("Failed to allocate settings.\n"));
	// spawn the notification thread
	#if USER
		fNotificationThread = spawn_thread(_NotificationThreadEntry,
			"UFS notification thread", B_NORMAL_PRIORITY, this);
	#else
		fNotificationThread = spawn_kernel_thread(_NotificationThreadEntry,
			"UFS notification thread", B_NORMAL_PRIORITY, this);
	#endif
	if (fNotificationThread < 0)
		RETURN_ERROR(fNotificationThread);
	resume_thread(fNotificationThread);
	RETURN_ERROR(error);
}

// _NotificationThreadEntry
int32
FileSystem::_NotificationThreadEntry(void* data)
{
	return ((FileSystem*)data)->_NotificationThread();
}

// _NotificationThread
int32
FileSystem::_NotificationThread()
{
	// process the notification requests until the FS is deleted
	while (!fTerminating) {
		if (fNotificationPort->InitCheck() != B_OK)
			return fNotificationPort->InitCheck();
		KernelRequestHandler handler(this, NO_REQUEST);
		fNotificationPort->HandleRequests(&handler, NULL,
			kNotificationRequestTimeout);
	}
	// We eat all remaining notification requests, so that they aren't
	// presented to the file system, when it is mounted next time.
	// TODO: We should probably use a special handler that sends an ack reply,
	// but ignores the requests otherwise.
	KernelRequestHandler handler(this, NO_REQUEST);
	fNotificationPort->HandleRequests(&handler, NULL, 0);
	return 0;
}

