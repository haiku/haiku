/*
 * Copyright 2001-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Volume.h"

#include <util/AutoLock.h>

#include <fs/node_monitor.h>
#include <Notifications.h>

#include "AutoLocker.h"
#include "Compatibility.h"
#include "Debug.h"
#include "FileSystem.h"
#include "HashMap.h"
#include "kernel_interface.h"
#include "KernelRequestHandler.h"
#include "PortReleaser.h"
#include "RequestAllocator.h"
#include "Requests.h"
#include "Settings.h"
#include "SingleReplyRequestHandler.h"


// The time after which the notification thread times out at the port and
// restarts the loop. Of interest only when the FS is deleted. It is the
// maximal time the destructor has to wait for the thread.
static const bigtime_t kNotificationRequestTimeout = 50000;	// 50 ms


// #pragma mark - SelectSyncMap


struct FileSystem::SelectSyncMap
	: public SynchronizedHashMap<HashKeyPointer<selectsync*>, int32*, Locker> {
};


// #pragma mark - NodeListenerKey


struct FileSystem::NodeListenerKey {
	NodeListenerKey(void* clientListener, dev_t device, ino_t node)
		:
		fClientListener(clientListener),
		fDevice(device),
		fNode(node)
	{
	}

	void* ClientListener() const
	{
		return fClientListener;
	}

	dev_t Device() const
	{
		return fDevice;
	}

	ino_t Node() const
	{
		return fNode;
	}

	uint32 HashValue() const
	{
		return (uint32)(addr_t)fClientListener ^ (uint32)fDevice
			^ (uint32)fNode ^ (uint32)(fNode >> 32);
	}

	bool operator==(const NodeListenerKey& other) const
	{
		return fClientListener == other.fClientListener
			&& fDevice == other.fDevice && fNode == other.fNode;
	}

protected:
	void*	fClientListener;
	dev_t	fDevice;
	ino_t	fNode;
};


// #pragma mark - NodeListenerProxy


struct FileSystem::NodeListenerProxy : NodeListenerKey, NotificationListener {
	NodeListenerProxy(FileSystem* fileSystem, void* clientListener,
		dev_t device, ino_t node)
		:
		NodeListenerKey(clientListener, device, node),
		fFileSystem(fileSystem)
	{
	}

	virtual void EventOccurred(NotificationService& service,
		const KMessage* event)
	{
		fFileSystem->_NodeListenerEventOccurred(this, event);
	}

	NodeListenerProxy*& HashTableLink()
	{
		return fHashTableLink;
	}

	status_t StartListening(uint32 flags)
	{
		return add_node_listener(fDevice, fNode, flags, *this);
	}

	status_t StopListening()
	{
		return remove_node_listener(fDevice, fNode, *this);
	}

private:
	FileSystem*			fFileSystem;
	NodeListenerProxy*	fHashTableLink;
};


// #pragma mark - NodeListenerHashDefinition


struct FileSystem::NodeListenerHashDefinition {
	typedef NodeListenerKey		KeyType;
	typedef	NodeListenerProxy	ValueType;

	size_t HashKey(const NodeListenerKey& key) const
	{
		return key.HashValue();
	}

	size_t Hash(const NodeListenerProxy* value) const
	{
		return value->HashValue();
	}

	bool Compare(const NodeListenerKey& key,
		const NodeListenerProxy* value) const
	{
		return key == *value;
	}

	NodeListenerProxy*& GetLink(NodeListenerProxy* value) const
	{
		return value->HashTableLink();
	}
};


// #pragma mark - FileSystem


// constructor
FileSystem::FileSystem()
	:
	fVolumes(),
	fName(),
	fTeam(-1),
	fNotificationPort(NULL),
	fNotificationThread(-1),
	fPortPool(),
	fSelectSyncs(NULL),
	fSettings(NULL),
	fUserlandServerTeam(-1),
	fInitialized(false),
	fTerminating(false)
{
	mutex_init(&fVolumeLock, "userlandfs volumes");
	mutex_init(&fVNodeOpsLock, "userlandfs vnode ops");
	mutex_init(&fNodeListenersLock, "userlandfs node listeners");
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
	if (fNodeListeners != NULL) {
		MutexLocker nodeListenersLocker(fNodeListenersLock);
		NodeListenerProxy* proxy = fNodeListeners->Clear(true);
		while (proxy != NULL) {
			NodeListenerProxy* next = proxy->HashTableLink();
			proxy->StopListening();
			delete proxy;
			proxy = next;
		}
	}

	if (fSelectSyncs) {
		for (SelectSyncMap::Iterator it = fSelectSyncs->GetIterator();
				it.HasNext();) {
			SelectSyncMap::Entry entry = it.Next();
			delete entry.value;
		}
		delete fSelectSyncs;
	}

	delete fSettings;

	// delete vnode ops vectors -- there shouldn't be any left, though
	VNodeOps* ops = fVNodeOps.Clear();
	int32 count = 0;
	while (ops != NULL) {
		count++;
		VNodeOps* next = ops->hash_link;
		free(ops);
		ops = next;
	}
	if (count > 0)
		WARN(("Deleted %" B_PRId32 " vnode ops vectors!\n", count));


	mutex_destroy(&fVolumeLock);
	mutex_destroy(&fVNodeOpsLock);
	mutex_destroy(&fNodeListenersLock);
}

// Init
status_t
FileSystem::Init(const char* name, team_id team, Port::Info* infos, int32 count,
	const FSCapabilities& capabilities)
{
	PRINT(("FileSystem::Init(\"%s\", %p, %" B_PRId32 ")\n", name, infos,
		count));
	capabilities.Dump();

	// check parameters
	if (!name || !infos || count < 2)
		RETURN_ERROR(B_BAD_VALUE);

	// set the name
	if (!fName.SetTo(name))
		return B_NO_MEMORY;

	// init VNodeOps map
	status_t error = fVNodeOps.Init();
	if (error != B_OK)
		return error;

	fTeam = team;
	fCapabilities = capabilities;

	// create the select sync entry map
	fSelectSyncs = new(nothrow) SelectSyncMap;
	if (!fSelectSyncs)
		return B_NO_MEMORY;

	// create the node listener proxy map
	fNodeListeners = new(std::nothrow) NodeListenerMap;
	if (fNodeListeners == NULL || fNodeListeners->Init() != B_OK)
		return B_NO_MEMORY;

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
		PRINT(("  userland team is: %" B_PRId32 "\n", fUserlandServerTeam));
		int32 cookie = 0;
		thread_info threadInfo;
		while (get_next_thread_info(fUserlandServerTeam, &cookie, &threadInfo)
			   == B_OK) {
			PRINT(("    userland thread: %" B_PRId32 ": `%s'\n",
				threadInfo.thread, threadInfo.name));
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

	fInitialized = (error == B_OK);
	RETURN_ERROR(error);
}

// GetName
const char*
FileSystem::GetName() const
{
	return fName.GetString();
}

// GetCapabilities
const FSCapabilities&
FileSystem::GetCapabilities() const
{
	return fCapabilities;
}

// GetPortPool
RequestPortPool*
FileSystem::GetPortPool()
{
	return &fPortPool;
}

// Mount
status_t
FileSystem::Mount(fs_volume* fsVolume, const char* device, uint32 flags,
	const char* parameters, Volume** _volume)
{
	// check initialization and parameters
	if (!fInitialized || !_volume)
		return B_BAD_VALUE;

	// create volume
	Volume* volume = new(nothrow) Volume(this, fsVolume);
	if (!volume)
		return B_NO_MEMORY;

	// add volume to the volume list
	MutexLocker locker(fVolumeLock);
	status_t error = fVolumes.PushBack(volume);
	locker.Unlock();
	if (error != B_OK)
		return error;

	// mount volume
	error = volume->Mount(device, flags, parameters);
	if (error != B_OK) {
		MutexLocker locker(fVolumeLock);
		fVolumes.Remove(volume);
		locker.Unlock();
		volume->ReleaseReference();
		return error;
	}

	*_volume = volume;
	return error;
}

// Initialize
/*status_t
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
}*/

// VolumeUnmounted
void
FileSystem::VolumeUnmounted(Volume* volume)
{
	MutexLocker locker(fVolumeLock);
	fVolumes.Remove(volume);
}

// GetVolume
Volume*
FileSystem::GetVolume(dev_t id)
{
	MutexLocker _(fVolumeLock);
	for (Vector<Volume*>::Iterator it = fVolumes.Begin();
		 it != fVolumes.End();
		 it++) {
		 Volume* volume = *it;
		 if (volume->GetID() == id) {
		 	volume->AcquireReference();
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


// AddNodeListener
status_t
FileSystem::AddNodeListener(dev_t device, ino_t node, uint32 flags,
	void* listener)
{
	MutexLocker nodeListenersLocker(fNodeListenersLock);

	// lookup the proxy
	NodeListenerProxy* proxy = fNodeListeners->Lookup(
		NodeListenerKey(listener, device, node));
	if (proxy != NULL)
		return proxy->StartListening(flags);

	// it doesn't exist yet -- create it
	proxy = new(std::nothrow) NodeListenerProxy(this, listener, device, node);
	if (proxy == NULL)
		return B_NO_MEMORY;

	// start listening
	status_t error = proxy->StartListening(flags);
	if (error != B_OK) {
		delete proxy;
		return error;
	}

	fNodeListeners->Insert(proxy);
	return B_OK;
}


// RemoveNodeListener
status_t
FileSystem::RemoveNodeListener(dev_t device, ino_t node, void* listener)
{
	MutexLocker nodeListenersLocker(fNodeListenersLock);

	// lookup the proxy
	NodeListenerProxy* proxy = fNodeListeners->Lookup(
		NodeListenerKey(listener, device, node));
	if (proxy == NULL)
		return B_BAD_VALUE;

	status_t error = proxy->StopListening();

	fNodeListeners->Remove(proxy);
	delete proxy;

	return error;
}


// GetVNodeOps
VNodeOps*
FileSystem::GetVNodeOps(const FSVNodeCapabilities& capabilities)
{
	MutexLocker locker(fVNodeOpsLock);

	// do we already have ops for those capabilities
	VNodeOps* ops = fVNodeOps.Lookup(capabilities);
	if (ops != NULL) {
		ops->refCount++;
		return ops;
	}

	// no, create a new object
	fs_vnode_ops* opsVector = new(std::nothrow) fs_vnode_ops;
	if (opsVector == NULL)
		return NULL;

	// set the operations
	_InitVNodeOpsVector(opsVector, capabilities);

	// create the VNodeOps object
	ops = new(std::nothrow) VNodeOps(capabilities, opsVector);
	if (ops == NULL) {
		delete opsVector;
		return NULL;
	}

	fVNodeOps.Insert(ops);

	return ops;
}


// PutVNodeOps
void
FileSystem::PutVNodeOps(VNodeOps* ops)
{
	MutexLocker locker(fVNodeOpsLock);

	if (--ops->refCount == 0) {
		fVNodeOps.Remove(ops);
		delete ops;
	}
}


// IsUserlandServerThread
bool
FileSystem::IsUserlandServerThread() const
{
	thread_info info;
	get_thread_info(find_thread(NULL), &info);
	return (info.team == fUserlandServerTeam);
}


// _InitVNodeOpsVector
void
FileSystem::_InitVNodeOpsVector(fs_vnode_ops* ops,
	const FSVNodeCapabilities& capabilities)
{
	memcpy(ops, &gUserlandFSVnodeOps, sizeof(fs_vnode_ops));

	#undef CLEAR_UNSUPPORTED
	#define CLEAR_UNSUPPORTED(capability, op) 	\
		if (!capabilities.Get(capability))				\
			ops->op = NULL

	// vnode operations
	// FS_VNODE_CAPABILITY_LOOKUP: lookup
	// FS_VNODE_CAPABILITY_GET_VNODE_NAME: get_vnode_name
		// emulated in userland
	// FS_VNODE_CAPABILITY_PUT_VNODE: put_vnode
	// FS_VNODE_CAPABILITY_REMOVE_VNODE: remove_vnode
		// needed by Volume to clean up

	// asynchronous I/O
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_IO, io);
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_CANCEL_IO, cancel_io);

	// cache file access
	ops->get_file_map = NULL;	// never used

	// common operations
	// FS_VNODE_CAPABILITY_IOCTL: ioctl
		// needed by Volume
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_SET_FLAGS, set_flags);
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_SELECT, select);
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_DESELECT, deselect);
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_FSYNC, fsync);

	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_READ_SYMLINK, read_symlink);
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_CREATE_SYMLINK, create_symlink);

	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_LINK, link);
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_UNLINK, unlink);
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_RENAME, rename);

	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_ACCESS, access);
	// FS_VNODE_CAPABILITY_READ_STAT: read_stat
		// needed by Volume
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_WRITE_STAT, write_stat);

	// file operations
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_CREATE, create);
	// FS_VNODE_CAPABILITY_OPEN: open
		// mandatory
	// FS_VNODE_CAPABILITY_CLOSE: close
		// needed by Volume
	// FS_VNODE_CAPABILITY_FREE_COOKIE: free_cookie
		// needed by Volume
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_READ, read);
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_WRITE, write);

	// directory operations
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_CREATE_DIR, create_dir);
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_REMOVE_DIR, remove_dir);
	// FS_VNODE_CAPABILITY_OPEN_DIR: open_dir
		// mandatory
	// FS_VNODE_CAPABILITY_CLOSE_DIR: close_dir
		// needed by Volume
	// FS_VNODE_CAPABILITY_FREE_DIR_COOKIE: free_dir_cookie
		// needed by Volume
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_READ_DIR, read_dir);
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_REWIND_DIR, rewind_dir);

	// attribute directory operations
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_OPEN_ATTR_DIR, open_attr_dir);
	// FS_VNODE_CAPABILITY_CLOSE_ATTR_DIR: close_attr_dir
		// needed by Volume
	// FS_VNODE_CAPABILITY_FREE_ATTR_DIR_COOKIE: free_attr_dir_cookie
		// needed by Volume
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_READ_ATTR_DIR, read_attr_dir);
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_REWIND_ATTR_DIR, rewind_attr_dir);

	// attribute operations
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_CREATE_ATTR, create_attr);
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_OPEN_ATTR, open_attr);
	// FS_VNODE_CAPABILITY_CLOSE_ATTR: close_attr
		// needed by Volume
	// FS_VNODE_CAPABILITY_FREE_ATTR_COOKIE: free_attr_cookie
		// needed by Volume
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_READ_ATTR, read_attr);
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_WRITE_ATTR, write_attr);

	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_READ_ATTR_STAT, read_attr_stat);
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_WRITE_ATTR_STAT, write_attr_stat);
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_RENAME_ATTR, rename_attr);
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_REMOVE_ATTR, remove_attr);

	// support for node and FS layers
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_CREATE_SPECIAL_NODE,
		create_special_node);
	CLEAR_UNSUPPORTED(FS_VNODE_CAPABILITY_GET_SUPER_VNODE, get_super_vnode);

	#undef CLEAR_UNSUPPORTED
}


// _NodeListenerEventOccurred
void
FileSystem::_NodeListenerEventOccurred(NodeListenerProxy* proxy,
	const KMessage* event)
{
	// get a free port
	RequestPort* port = fPortPool.AcquirePort();
	if (port == NULL)
		return;
	PortReleaser _(&fPortPool, port);

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	NodeMonitoringEventRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return;

	error = allocator.AllocateData(request->event, event->Buffer(),
		event->ContentSize(), 1);
	if (error != B_OK)
		return;

	Thread* thread = thread_get_current_thread();
	request->team = thread->team->id;
	request->thread = thread->id;
	request->user = geteuid();
	request->group = getegid();
	request->listener = proxy->ClientListener();

	// send the request
	KernelRequestHandler handler(this, NODE_MONITORING_EVENT_REPLY);
	port->SendRequest(&allocator, &handler);
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

