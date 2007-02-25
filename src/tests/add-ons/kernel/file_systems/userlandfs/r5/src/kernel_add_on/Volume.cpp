// Volume.cpp

#include <errno.h>
#include <unistd.h>

#include "AutoLocker.h"
#include "Compatibility.h"
#include "Debug.h"
#include "FileSystem.h"
#include "HashMap.h"
#include "IOCtlInfo.h"
#include "KernelRequestHandler.h"
#include "PortReleaser.h"
#include "RequestAllocator.h"
#include "RequestPort.h"
#include "Requests.h"
#include "userlandfs_ioctl.h"
#include "Volume.h"

// If a thread of the userland server enters userland FS kernel code and
// is sending a request, this is the time after which it shall time out
// waiting for a reply.
static const bigtime_t kUserlandServerlandPortTimeout = 10000000;	// 10s

// MountVNodeMap
struct Volume::MountVNodeMap : public HashMap<HashKey64<vnode_id>, void*> {
};

// VNodeCountMap
struct Volume::VNodeCountMap
	: public SynchronizedHashMap<HashKey64<vnode_id>, int32*> {
};

// AutoIncrementer
class Volume::AutoIncrementer {
public:
	AutoIncrementer(vint32* variable)
		: fVariable(variable)
	{
		if (fVariable)
			atomic_add(fVariable, 1);
	}

	~AutoIncrementer()
	{
		if (fVariable)
			atomic_add(fVariable, -1);
	}

	void Keep()
	{
		fVariable = NULL;
	}

private:
	vint32*	fVariable;
};

// constructor
Volume::Volume(FileSystem* fileSystem, nspace_id id)
	: Referencable(true),
	  fFileSystem(fileSystem),
	  fID(id),
	  fUserlandVolume(NULL),
	  fRootID(0),
	  fRootNode(NULL),
	  fMountVNodes(NULL),
	  fOpenFiles(0),
	  fOpenDirectories(0),
	  fOpenAttributeDirectories(0),
	  fOpenIndexDirectories(0),
	  fOpenQueries(0),
	  fVNodeCountMap(NULL),
	  fVNodeCountingEnabled(false)
{
}

// destructor
Volume::~Volume()
{
}

// GetFileSystem
FileSystem*
Volume::GetFileSystem() const
{
	return fFileSystem;
}

// GetID
nspace_id
Volume::GetID() const
{
	return fID;
}

// GetUserlandVolume
void*
Volume::GetUserlandVolume() const
{
	return fUserlandVolume;
}

// GetRootID
vnode_id
Volume::GetRootID() const
{
	return fRootID;
}

// IsMounting
bool
Volume::IsMounting() const
{
	return fMountVNodes;
}


// #pragma mark -
// #pragma mark ----- client methods -----

// GetVNode
status_t
Volume::GetVNode(vnode_id vnid, void** node)
{
PRINT(("get_vnode(%ld, %Ld)\n", fID, vnid));
	if (IsMounting() && !fMountVNodes->ContainsKey(vnid)) {
		ERROR(("Volume::GetVNode(): get_vnode() invoked for unknown vnode "
			"while mounting!\n"));
	}
	status_t error = get_vnode(fID, vnid, node);
	if (error == B_OK)
		_IncrementVNodeCount(vnid);
	return error;
}

// PutVNode
status_t
Volume::PutVNode(vnode_id vnid)
{
PRINT(("put_vnode(%ld, %Ld)\n", fID, vnid));
	status_t error = put_vnode(fID, vnid);
	if (error == B_OK)
		_DecrementVNodeCount(vnid);
	return error;
}

// NewVNode
status_t
Volume::NewVNode(vnode_id vnid, void* node)
{
PRINT(("new_vnode(%ld, %Ld)\n", fID, vnid));
	status_t error = new_vnode(fID, vnid, node);
	if (error == B_OK) {
		if (IsMounting()) {
			error = fMountVNodes->Put(vnid, node);
			if (error != B_OK) {
				ERROR(("Volume::NewVNode(): Failed to add vnode to mount "
					"vnode map!\n"));
				put_vnode(fID, vnid);
				return error;
			}
		}
		_IncrementVNodeCount(vnid);
	}
	return error;
}

// RemoveVNode
status_t
Volume::RemoveVNode(vnode_id vnid)
{
PRINT(("remove_vnode(%ld, %Ld)\n", fID, vnid));
	return remove_vnode(fID, vnid);
}

// UnremoveVNode
status_t
Volume::UnremoveVNode(vnode_id vnid)
{
PRINT(("unremove_vnode(%ld, %Ld)\n", fID, vnid));
	return unremove_vnode(fID, vnid);
}

// IsVNodeRemoved
status_t
Volume::IsVNodeRemoved(vnode_id vnid)
{
PRINT(("is_vnode_removed(%ld, %Ld)\n", fID, vnid));
	return is_vnode_removed(fID, vnid);
}

// #pragma mark -
// #pragma mark ----- FS -----

// Mount
status_t
Volume::Mount(const char* device, ulong flags, const char* parameters,
	int32 len)
{
	// Create a map that holds vnode_id->void* mappings of all vnodes
	// created while mounting. We need it to get the root node.
	MountVNodeMap vnodeMap;
	status_t error = vnodeMap.InitCheck();
	if (error != B_OK)
		RETURN_ERROR(error);
	fMountVNodes = &vnodeMap;
	error = _Mount(device, flags, parameters, len);
	fMountVNodes = NULL;
	if (error == B_OK) {
		// fetch the root node, so that we can serve Walk() requests on it,
		// after the connection to the userland server is gone
		if (!vnodeMap.ContainsKey(fRootID)) {
			// The root node was not added while mounting. That's a serious
			// problem -- not only because we don't have it, but also because
			// the VFS requires new_vnode() to be invoked for the root node.
			ERROR(("Volume::Mount(): new_vnode() was not called for root node! "
				"Unmounting...\n"));
			Unmount();
			return B_ERROR;
		}
		fRootNode = vnodeMap.Get(fRootID);
	}
	return error;
}

// Unmount
status_t
Volume::Unmount()
{
	status_t error = _Unmount();
	// free the memory associated with the vnode count map
	if (fVNodeCountMap) {
		AutoLocker<VNodeCountMap> _(fVNodeCountMap);
		fVNodeCountingEnabled = false;
		for (VNodeCountMap::Iterator it = fVNodeCountMap->GetIterator();
			 it.HasNext();) {
			VNodeCountMap::Entry entry = it.Next();
			delete entry.value;
		}
		delete fVNodeCountMap;
		fVNodeCountMap = NULL;
	}
	fFileSystem->VolumeUnmounted(this);
	return error;
}

// Sync
status_t
Volume::Sync()
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	SyncVolumeRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	// send the request
	KernelRequestHandler handler(this, SYNC_VOLUME_REPLY);
	SyncVolumeReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// ReadFSStat
status_t
Volume::ReadFSStat(fs_info* info)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadFSStatRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	// send the request
	KernelRequestHandler handler(this, READ_FS_STAT_REPLY);
	ReadFSStatReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	*info = reply->info;
	return error;
}

// WriteFSStat
status_t
Volume::WriteFSStat(struct fs_info *info, long mask)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	WriteFSStatRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->info = *info;
	request->mask = mask;
	// send the request
	KernelRequestHandler handler(this, WRITE_FS_STAT_REPLY);
	WriteFSStatReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// #pragma mark -
// #pragma mark ----- vnodes -----

// ReadVNode
status_t
Volume::ReadVNode(vnode_id vnid, char reenter, void** node)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadVNodeRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->vnid = vnid;
	request->reenter = reenter;
	// send the request
	KernelRequestHandler handler(this, READ_VNODE_REPLY);
	ReadVNodeReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	*node = reply->node;
	return error;
}

// WriteVNode
status_t
Volume::WriteVNode(void* node, char reenter)
{
	status_t error = _WriteVNode(node, reenter);
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, since the VFS basically ignores the
		// return value -- at least OBOS. The fshell panic()s; didn't check
		// BeOS. It doesn't harm to appear to behave nicely. :-)
		WARN(("Volume::WriteVNode(): connection lost, forcing write vnode\n"));
		return B_OK;
	}
	return error;
}

// RemoveVNode
status_t
Volume::RemoveVNode(void* node, char reenter)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FSRemoveVNodeRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->reenter = reenter;
	// send the request
	KernelRequestHandler handler(this, FS_REMOVE_VNODE_REPLY);
	FSRemoveVNodeReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// #pragma mark -
// #pragma mark ----- nodes -----

// FSync
status_t
Volume::FSync(void* node)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FSyncRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	// send the request
	KernelRequestHandler handler(this, FSYNC_REPLY);
	FSyncReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// ReadStat
status_t
Volume::ReadStat(void* node, struct stat* st)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadStatRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	// send the request
	KernelRequestHandler handler(this, READ_STAT_REPLY);
	ReadStatReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	*st = reply->st;
	return error;
}

// WriteStat
status_t
Volume::WriteStat(void* node, struct stat* st, long mask)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	WriteStatRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->st = *st;
	request->mask = mask;
	// send the request
	KernelRequestHandler handler(this, WRITE_STAT_REPLY);
	WriteStatReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// Access
status_t
Volume::Access(void* node, int mode)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	AccessRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->mode = mode;
	// send the request
	KernelRequestHandler handler(this, ACCESS_REPLY);
	AccessReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// #pragma mark -
// #pragma mark ----- files -----

// Create
status_t
Volume::Create(void* dir, const char* name, int openMode, int mode,
	vnode_id* vnid, void** cookie)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	CreateRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = dir;
	error = allocator.AllocateString(request->name, name);
	request->openMode = openMode;
	request->mode = mode;
	if (error != B_OK)
		return error;
	// send the request
	KernelRequestHandler handler(this, CREATE_REPLY);
	CreateReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	*vnid = reply->vnid;
	*cookie = reply->fileCookie;
	if (error == B_OK)
		_DecrementVNodeCount(*vnid);
			// The VFS will balance the new_vnode() call for the FS.
	return error;
}

// Open
status_t
Volume::Open(void* node, int openMode, void** cookie)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	AutoIncrementer incrementer(&fOpenFiles);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	OpenRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->openMode = openMode;
	// send the request
	KernelRequestHandler handler(this, OPEN_REPLY);
	OpenReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	incrementer.Keep();
	*cookie = reply->fileCookie;
	return error;
}

// Close
status_t
Volume::Close(void* node, void* cookie)
{
	status_t error = _Close(node, cookie);
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. OBOS ignores it completely. The fsshell returns it to the
		// userland, but considers the node closed anyway.
		WARN(("Volume::Close(): connection lost, forcing close\n"));
		return B_OK;
	}
	return error;
}

// FreeCookie
status_t
Volume::FreeCookie(void* node, void* cookie)
{
	status_t error = _FreeCookie(node, cookie);
	bool disconnected = false;
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. It's completely ignored by OBOS as well as by the fsshell.
		WARN(("Volume::FreeCookie(): connection lost, forcing free cookie\n"));
		error = B_OK;
		disconnected = true;
	}
	int32 openFiles = atomic_add(&fOpenFiles, -1);
	if (openFiles <= 1 && disconnected)
		_PutAllPendingVNodes();
	return error;
}

// Read
status_t
Volume::Read(void* node, void* cookie, off_t pos, void* buffer,
	size_t bufferSize, size_t* bytesRead)
{
	*bytesRead = 0;
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->fileCookie = cookie;
	request->pos = pos;
	request->size = bufferSize;
	// send the request
	KernelRequestHandler handler(this, READ_REPLY);
	ReadReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	void* readBuffer = reply->buffer.GetData();
	if (reply->bytesRead > (uint32)reply->buffer.GetSize()
		|| reply->bytesRead > bufferSize) {
		return B_BAD_DATA;
	}
	if (reply->bytesRead > 0)
		memcpy(buffer, readBuffer, reply->bytesRead);
	*bytesRead = reply->bytesRead;
	_SendReceiptAck(port);
	return error;
}

// Write
status_t
Volume::Write(void* node, void* cookie, off_t pos, const void* buffer,
	size_t size, size_t* bytesWritten)
{
	*bytesWritten = 0;
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	WriteRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->fileCookie = cookie;
	request->pos = pos;
	error = allocator.AllocateData(request->buffer, buffer, size, 1);
	if (error != B_OK)
		return error;
	// send the request
	KernelRequestHandler handler(this, WRITE_REPLY);
	WriteReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	*bytesWritten = reply->bytesWritten;
	return error;
}

// IOCtl
status_t
Volume::IOCtl(void* node, void* cookie, int command, void *buffer,
	size_t len)
{
	// check the command and its parameters
	bool isBuffer = false;
	int32 bufferSize = 0;
	int32 writeSize = 0;
	switch (command) {
		case IOCTL_FILE_UNCACHED_IO:
			buffer = NULL;
			break;
		case IOCTL_CREATE_TIME:
		case IOCTL_MODIFIED_TIME:
			isBuffer = 0;
			bufferSize = 0;
			writeSize = sizeof(bigtime_t);
			break;
		case USERLANDFS_IOCTL:
			area_id area;
			area_info info;
			PRINT(("Volume::IOCtl(): USERLANDFS_IOCTL\n"));
			if ((area = area_for(buffer)) >= 0) {
				if (get_area_info(area, &info) == B_OK) {
					if ((uint8*)buffer - (uint8*)info.address
							+ sizeof(userlandfs_ioctl) <= info.size) {
						if (strncmp(((userlandfs_ioctl*)buffer)->magic,
								kUserlandFSIOCtlMagic,
								USERLAND_IOCTL_MAGIC_LENGTH) == 0) {
							return _InternalIOCtl((userlandfs_ioctl*)buffer,
								bufferSize);
						} else
							PRINT(("Volume::IOCtl(): bad magic\n"));
					} else
						PRINT(("Volume::IOCtl(): bad buffer size\n"));
				} else
					PRINT(("Volume::IOCtl(): failed to get area info\n"));
			} else
				PRINT(("Volume::IOCtl(): bad area\n"));
			// fall through...
		default:
		{
			// We don't know the command. Check whether the FileSystem knows
			// about it.
			const IOCtlInfo* info = fFileSystem->GetIOCtlInfo(command);
			if (!info) {
				PRINT(("Volume::IOCtl(): unknown command\n"));
				return B_BAD_VALUE;
			}
			isBuffer = info->isBuffer;
			bufferSize = info->bufferSize;
			writeSize = info->writeBufferSize;
			// If the buffer shall indeed specify a buffer, check it.
			if (info->isBuffer) {
				if (!buffer) {
					PRINT(("Volume::IOCtl(): buffer is NULL\n"));
					return B_BAD_VALUE;
				}
				area_id area = area_for(buffer);
				if (area < 0) {
					PRINT(("Volume::IOCtl(): bad area\n"));
					return B_BAD_VALUE;
				}
				area_info info;
				if (get_area_info(area, &info) != B_OK) {
					PRINT(("Volume::IOCtl(): failed to get area info\n"));
					return B_BAD_VALUE;
				}
				int32 areaSize = info.size - ((uint8*)buffer
					- (uint8*)info.address);
				if (bufferSize > areaSize || writeSize > areaSize) {
					PRINT(("Volume::IOCtl(): bad buffer size\n"));
					return B_BAD_VALUE;
				}
				if (writeSize > 0 && !(info.protection & B_WRITE_AREA)) {
					PRINT(("Volume::IOCtl(): buffer not writable\n"));
					return B_BAD_VALUE;
				}
			}
			break;
		}
	}
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	IOCtlRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->fileCookie = cookie;
	request->command = command;
	request->bufferParameter = buffer;
	request->isBuffer = isBuffer;
	request->lenParameter = len;
	request->writeSize = writeSize;
	if (isBuffer && bufferSize > 0) {
		error = allocator.AllocateData(request->buffer, buffer, bufferSize, 8);
		if (error != B_OK)
			return error;
	}
	// send the request
	KernelRequestHandler handler(this, IOCTL_REPLY);
	IOCtlReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	// Copy back the buffer even if the result is not B_OK. The protocol
	// is defined by the FS developer and may include writing data into
	// the buffer in some error cases.
	if (isBuffer && writeSize > 0 && reply->buffer.GetData()) {
		if (writeSize > reply->buffer.GetSize())
			writeSize = reply->buffer.GetSize();
		memcpy(buffer, reply->buffer.GetData(), writeSize);
		_SendReceiptAck(port);
	}
	return reply->ioctlError;
}

// SetFlags
status_t
Volume::SetFlags(void* node, void* cookie, int flags)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	SetFlagsRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->fileCookie = cookie;
	request->flags = flags;
	// send the request
	KernelRequestHandler handler(this, SET_FLAGS_REPLY);
	SetFlagsReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// Select
status_t
Volume::Select(void* node, void* cookie, uint8 event, uint32 ref,
	selectsync* sync)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	SelectRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->fileCookie = cookie;
	request->event = event;
	request->ref = ref;
	request->sync = sync;
	// add a selectsync entry
	error = fFileSystem->AddSelectSyncEntry(sync);
	if (error != B_OK)
		return error;
	// send the request
	KernelRequestHandler handler(this, SELECT_REPLY);
	SelectReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK) {
		fFileSystem->RemoveSelectSyncEntry(sync);
		return error;
	}
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK) {
		fFileSystem->RemoveSelectSyncEntry(sync);
		return reply->error;
	}
	return error;
}

// Deselect
status_t
Volume::Deselect(void* node, void* cookie, uint8 event, selectsync* sync)
{
	struct SyncRemover {
		SyncRemover(FileSystem* fs, selectsync* sync)
			: fs(fs), sync(sync) {}
		~SyncRemover() { fs->RemoveSelectSyncEntry(sync); }

		FileSystem*	fs;
		selectsync*	sync;
	} syncRemover(fFileSystem, sync);
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	DeselectRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->fileCookie = cookie;
	request->event = event;
	request->sync = sync;
	// send the request
	KernelRequestHandler handler(this, DESELECT_REPLY);
	DeselectReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// #pragma mark -
// #pragma mark ----- hard links / symlinks -----

// Link
status_t
Volume::Link(void* dir, const char* name, void* node)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	LinkRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = dir;
	error = allocator.AllocateString(request->name, name);
	request->target = node;
	if (error != B_OK)
		return error;
	// send the request
	KernelRequestHandler handler(this, LINK_REPLY);
	LinkReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// Unlink
status_t
Volume::Unlink(void* dir, const char* name)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	UnlinkRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = dir;
	error = allocator.AllocateString(request->name, name);
	if (error != B_OK)
		return error;
	// send the request
	KernelRequestHandler handler(this, UNLINK_REPLY);
	UnlinkReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// Symlink
status_t
Volume::Symlink(void* dir, const char* name, const char* target)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	SymlinkRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = dir;
	error = allocator.AllocateString(request->name, name);
	if (error == B_OK)
		error = allocator.AllocateString(request->target, target);
	if (error != B_OK)
		return error;
	// send the request
	KernelRequestHandler handler(this, SYMLINK_REPLY);
	SymlinkReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// ReadLink
status_t
Volume::ReadLink(void* node, char* buffer, size_t bufferSize, size_t* bytesRead)
{
	*bytesRead = 0;
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadLinkRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->size = bufferSize;
	// send the request
	KernelRequestHandler handler(this, READ_LINK_REPLY);
	ReadLinkReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	void* readBuffer = reply->buffer.GetData();
	if (reply->bytesRead > (uint32)reply->buffer.GetSize()
		|| reply->bytesRead > bufferSize) {
		return B_BAD_DATA;
	}
	if (reply->bytesRead > 0)
		memcpy(buffer, readBuffer, reply->bytesRead);
	*bytesRead = reply->bytesRead;
	_SendReceiptAck(port);
	return error;
}

// Rename
status_t
Volume::Rename(void* oldDir, const char* oldName, void* newDir,
	const char* newName)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	RenameRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->oldDir = oldDir;
	request->newDir = newDir;
	error = allocator.AllocateString(request->oldName, oldName);
	if (error == B_OK)
		error = allocator.AllocateString(request->newName, newName);
	if (error != B_OK)
		return error;
	// send the request
	KernelRequestHandler handler(this, RENAME_REPLY);
	RenameReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// #pragma mark -
// #pragma mark ----- directories -----

// MkDir
status_t
Volume::MkDir(void* dir, const char* name, int mode)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	MkDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = dir;
	error = allocator.AllocateString(request->name, name);
	request->mode = mode;
	if (error != B_OK)
		return error;
	// send the request
	KernelRequestHandler handler(this, MKDIR_REPLY);
	MkDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// RmDir
status_t
Volume::RmDir(void* dir, const char* name)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	RmDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = dir;
	error = allocator.AllocateString(request->name, name);
	if (error != B_OK)
		return error;
	// send the request
	KernelRequestHandler handler(this, RMDIR_REPLY);
	RmDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// OpenDir
status_t
Volume::OpenDir(void* node, void** cookie)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	AutoIncrementer incrementer(&fOpenDirectories);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	OpenDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	// send the request
	KernelRequestHandler handler(this, OPEN_DIR_REPLY);
	OpenDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	incrementer.Keep();
	*cookie = reply->dirCookie;
	return error;
}

// CloseDir
status_t
Volume::CloseDir(void* node, void* cookie)
{
	status_t error = _CloseDir(node, cookie);
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. OBOS ignores it completely. The fsshell returns it to the
		// userland, but considers the node closed anyway.
		WARN(("Volume::CloseDir(): connection lost, forcing close dir\n"));
		return B_OK;
	}
	return error;
}

// FreeDirCookie
status_t
Volume::FreeDirCookie(void* node, void* cookie)
{
	status_t error = _FreeDirCookie(node, cookie);
	bool disconnected = false;
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. It's completely ignored by OBOS as well as by the fsshell.
		WARN(("Volume::FreeDirCookie(): connection lost, forcing free dir "
			"cookie\n"));
		error = B_OK;
		disconnected = true;
	}
	int32 openDirs = atomic_add(&fOpenDirectories, -1);
	if (openDirs <= 1 && disconnected)
		_PutAllPendingVNodes();
	return error;
}

// ReadDir
status_t
Volume::ReadDir(void* node, void* cookie, void* buffer, size_t bufferSize,
	int32 count, int32* countRead)
{
	*countRead = 0;
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->dirCookie = cookie;
	request->bufferSize = bufferSize;
	request->count = count;
	// send the request
	KernelRequestHandler handler(this, READ_DIR_REPLY);
	ReadDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	if (reply->count < 0 || reply->count > count)
		return B_BAD_DATA;
	if ((int32)bufferSize < reply->buffer.GetSize())
		return B_BAD_DATA;
PRINT(("Volume::ReadDir(): buffer returned: %ld bytes\n",
reply->buffer.GetSize()));
	*countRead = reply->count;
	if (*countRead > 0) {
		// copy the buffer -- limit the number of bytes to copy
		int32 maxBytes = *countRead
			* (sizeof(struct dirent) + B_FILE_NAME_LENGTH);
		int32 copyBytes = reply->buffer.GetSize();
		if (copyBytes > maxBytes)
			copyBytes = maxBytes;
		memcpy(buffer, reply->buffer.GetData(), copyBytes);
	}
	_SendReceiptAck(port);
	return error;
}

// RewindDir
status_t
Volume::RewindDir(void* node, void* cookie)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	RewindDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->dirCookie = cookie;
	// send the request
	KernelRequestHandler handler(this, REWIND_DIR_REPLY);
	RewindDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// Walk
status_t
Volume::Walk(void* dir, const char* entryName, char** resolvedPath,
	vnode_id* vnid)
{
	// When the connection to the userland server is lost, we serve
	// walk(fRootNode, `.') requests manually to allow clean unmounting.
	status_t error = _Walk(dir, entryName, resolvedPath, vnid);
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()
		&& dir == fRootNode && strcmp(entryName, ".") == 0) {
		WARN(("Volume::Walk(): connection lost, emulating walk `.'\n"));
		void* entryNode;
		if (GetVNode(fRootID, &entryNode) != B_OK)
			RETURN_ERROR(B_BAD_VALUE);
		*vnid = fRootID;
		// The VFS will balance the get_vnode() call for the FS.
		_DecrementVNodeCount(*vnid);
		return B_OK;
	}
	return error;
}

// #pragma mark -
// #pragma mark ----- attributes -----

// OpenAttrDir
status_t
Volume::OpenAttrDir(void* node, void** cookie)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	AutoIncrementer incrementer(&fOpenAttributeDirectories);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	OpenAttrDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	// send the request
	KernelRequestHandler handler(this, OPEN_ATTR_DIR_REPLY);
	OpenAttrDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	incrementer.Keep();
	*cookie = reply->attrDirCookie;
	return error;
}

// CloseAttrDir
status_t
Volume::CloseAttrDir(void* node, void* cookie)
{
	status_t error = _CloseAttrDir(node, cookie);
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. OBOS ignores it completely. The fsshell returns it to the
		// userland, but considers the node closed anyway.
		WARN(("Volume::CloseAttrDir(): connection lost, forcing close attr "
			"dir\n"));
		return B_OK;
	}
	return error;
}

// FreeAttrDirCookie
status_t
Volume::FreeAttrDirCookie(void* node, void* cookie)
{
	status_t error = _FreeAttrDirCookie(node, cookie);
	bool disconnected = false;
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. It's completely ignored by OBOS as well as by the fsshell.
		WARN(("Volume::FreeAttrDirCookie(): connection lost, forcing free attr "
			"dir cookie\n"));
		error = B_OK;
		disconnected = true;
	}
	int32 openAttrDirs = atomic_add(&fOpenAttributeDirectories, -1);
	if (openAttrDirs <= 1 && disconnected)
		_PutAllPendingVNodes();
	return error;
}

// ReadAttrDir
status_t
Volume::ReadAttrDir(void* node, void* cookie, void* buffer, size_t bufferSize,
	int32 count, int32* countRead)
{
	*countRead = 0;
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadAttrDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->attrDirCookie = cookie;
	request->bufferSize = bufferSize;
	request->count = count;
	// send the request
	KernelRequestHandler handler(this, READ_ATTR_DIR_REPLY);
	ReadAttrDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	if (reply->count < 0 || reply->count > count)
		return B_BAD_DATA;
	if ((int32)bufferSize < reply->buffer.GetSize())
		return B_BAD_DATA;
	*countRead = reply->count;
	if (*countRead > 0) {
		// copy the buffer -- limit the number of bytes to copy
		int32 maxBytes = *countRead
			* (sizeof(struct dirent) + B_ATTR_NAME_LENGTH);
		int32 copyBytes = reply->buffer.GetSize();
		if (copyBytes > maxBytes)
			copyBytes = maxBytes;
		memcpy(buffer, reply->buffer.GetData(), copyBytes);
	}
	_SendReceiptAck(port);
	return error;
}

// RewindAttrDir
status_t
Volume::RewindAttrDir(void* node, void* cookie)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	RewindAttrDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->attrDirCookie = cookie;
	// send the request
	KernelRequestHandler handler(this, REWIND_ATTR_DIR_REPLY);
	RewindAttrDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// ReadAttr
status_t
Volume::ReadAttr(void* node, const char* name, int type, off_t pos,
	void* buffer, size_t bufferSize, size_t* bytesRead)
{
	*bytesRead = 0;
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadAttrRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	error = allocator.AllocateString(request->name, name);
	request->type = type;
	request->pos = pos;
	request->size = bufferSize;
	if (error != B_OK)
		return error;
	// send the request
	KernelRequestHandler handler(this, READ_ATTR_REPLY);
	ReadAttrReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	void* readBuffer = reply->buffer.GetData();
	if (reply->bytesRead > (uint32)reply->buffer.GetSize()
		|| reply->bytesRead > bufferSize) {
		return B_BAD_DATA;
	}
	if (reply->bytesRead > 0)
		memcpy(buffer, readBuffer, reply->bytesRead);
	*bytesRead = reply->bytesRead;
	_SendReceiptAck(port);
	return error;
}

// WriteAttr
status_t
Volume::WriteAttr(void* node, const char* name, int type, off_t pos,
	const void* buffer, size_t bufferSize, size_t* bytesWritten)
{
	*bytesWritten = 0;
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	WriteAttrRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	error = allocator.AllocateString(request->name, name);
	request->type = type;
	request->pos = pos;
	if (error == B_OK)
		error = allocator.AllocateData(request->buffer, buffer, bufferSize, 1);
	if (error != B_OK)
		return error;
	// send the request
	KernelRequestHandler handler(this, WRITE_ATTR_REPLY);
	WriteAttrReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	*bytesWritten = reply->bytesWritten;
	return error;
}

// RemoveAttr
status_t
Volume::RemoveAttr(void* node, const char* name)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	RemoveAttrRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	error = allocator.AllocateString(request->name, name);
	if (error != B_OK)
		return error;
	// send the request
	KernelRequestHandler handler(this, REMOVE_ATTR_REPLY);
	RemoveAttrReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// RenameAttr
status_t
Volume::RenameAttr(void* node, const char* oldName, const char* newName)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	RenameAttrRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	error = allocator.AllocateString(request->oldName, oldName);
	if (error == B_OK)
		error = allocator.AllocateString(request->newName, newName);
	if (error != B_OK)
		return error;
	// send the request
	KernelRequestHandler handler(this, RENAME_ATTR_REPLY);
	RenameAttrReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// StatAttr
status_t
Volume::StatAttr(void* node, const char* name, struct attr_info* attrInfo)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	StatAttrRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	error = allocator.AllocateString(request->name, name);
	if (error != B_OK)
		return error;
	// send the request
	KernelRequestHandler handler(this, STAT_ATTR_REPLY);
	StatAttrReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	*attrInfo = reply->info;
	return error;
}

// #pragma mark -
// #pragma mark ----- indices -----

// OpenIndexDir
status_t
Volume::OpenIndexDir(void** cookie)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	AutoIncrementer incrementer(&fOpenIndexDirectories);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	OpenIndexDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	// send the request
	KernelRequestHandler handler(this, OPEN_INDEX_DIR_REPLY);
	OpenIndexDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	incrementer.Keep();
	*cookie = reply->indexDirCookie;
	return error;
}

// CloseIndexDir
status_t
Volume::CloseIndexDir(void* cookie)
{
	status_t error = _CloseIndexDir(cookie);
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. OBOS ignores it completely. The fsshell returns it to the
		// userland, but considers the node closed anyway.
		WARN(("Volume::CloseIndexDir(): connection lost, forcing close "
			"index dir\n"));
		return B_OK;
	}
	return error;
}

// FreeIndexDirCookie
status_t
Volume::FreeIndexDirCookie(void* cookie)
{
	status_t error = _FreeIndexDirCookie(cookie);
	bool disconnected = false;
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. It's completely ignored by OBOS as well as by the fsshell.
		WARN(("Volume::FreeIndexDirCookie(): connection lost, forcing free "
			"index dir cookie\n"));
		error = B_OK;
		disconnected = true;
	}
	int32 openIndexDirs = atomic_add(&fOpenIndexDirectories, -1);
	if (openIndexDirs <= 1 && disconnected)
		_PutAllPendingVNodes();
	return error;
}

// ReadIndexDir
status_t
Volume::ReadIndexDir(void* cookie, void* buffer, size_t bufferSize, int32 count,
	int32* countRead)
{
	*countRead = 0;
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadIndexDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->indexDirCookie = cookie;
	request->bufferSize = bufferSize;
	request->count = count;
	// send the request
	KernelRequestHandler handler(this, READ_INDEX_DIR_REPLY);
	ReadIndexDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	if (reply->count < 0 || reply->count > count)
		return B_BAD_DATA;
	if ((int32)bufferSize < reply->buffer.GetSize())
		return B_BAD_DATA;
	*countRead = reply->count;
	if (*countRead > 0) {
		// copy the buffer -- limit the number of bytes to copy
		int32 maxBytes = *countRead
			* (sizeof(struct dirent) + B_FILE_NAME_LENGTH);
		int32 copyBytes = reply->buffer.GetSize();
		if (copyBytes > maxBytes)
			copyBytes = maxBytes;
		memcpy(buffer, reply->buffer.GetData(), copyBytes);
	}
	_SendReceiptAck(port);
	return error;
}

// RewindIndexDir
status_t
Volume::RewindIndexDir(void* cookie)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	RewindIndexDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->indexDirCookie = cookie;
	// send the request
	KernelRequestHandler handler(this, REWIND_INDEX_DIR_REPLY);
	RewindIndexDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// CreateIndex
status_t
Volume::CreateIndex(const char* name, int type, int flags)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	CreateIndexRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	error = allocator.AllocateString(request->name, name);
	request->type = type;
	request->flags = flags;
	if (error != B_OK)
		return error;
	// send the request
	KernelRequestHandler handler(this, CREATE_INDEX_REPLY);
	CreateIndexReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// RemoveIndex
status_t
Volume::RemoveIndex(const char* name)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	RemoveIndexRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	error = allocator.AllocateString(request->name, name);
	if (error != B_OK)
		return error;
	// send the request
	KernelRequestHandler handler(this, REMOVE_INDEX_REPLY);
	RemoveIndexReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// RenameIndex
status_t
Volume::RenameIndex(const char* oldName, const char* newName)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	RenameIndexRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	error = allocator.AllocateString(request->oldName, oldName);
	if (error == B_OK)
		error = allocator.AllocateString(request->newName, newName);
	if (error != B_OK)
		return error;
	// send the request
	KernelRequestHandler handler(this, RENAME_INDEX_REPLY);
	RenameIndexReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// StatIndex
status_t
Volume::StatIndex(const char *name, struct index_info* indexInfo)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	StatIndexRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	error = allocator.AllocateString(request->name, name);
	if (error != B_OK)
		return error;
	// send the request
	KernelRequestHandler handler(this, STAT_INDEX_REPLY);
	StatIndexReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	*indexInfo = reply->info;
	return error;
}

// #pragma mark -
// #pragma mark ----- queries -----

// OpenQuery
status_t
Volume::OpenQuery(const char* queryString, ulong flags, port_id targetPort,
	long token, void** cookie)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	AutoIncrementer incrementer(&fOpenQueries);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	OpenQueryRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	error = allocator.AllocateString(request->queryString, queryString);
	if (error != B_OK)
		return error;
	request->flags = flags;
	request->port = targetPort;
	request->token = token;
	// send the request
	KernelRequestHandler handler(this, OPEN_QUERY_REPLY);
	OpenQueryReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	incrementer.Keep();
	*cookie = reply->queryCookie;
	return error;
}

// CloseQuery
status_t
Volume::CloseQuery(void* cookie)
{
	status_t error = _CloseQuery(cookie);
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. OBOS ignores it completely. The fsshell returns it to the
		// userland, but considers the node closed anyway.
		WARN(("Volume::CloseQuery(): connection lost, forcing close query\n"));
		return B_OK;
	}
	return error;
}

// FreeQueryCookie
status_t
Volume::FreeQueryCookie(void* cookie)
{
	status_t error = _FreeQueryCookie(cookie);
	bool disconnected = false;
	if (error != B_OK && fFileSystem->GetPortPool()->IsDisconnected()) {
		// This isn't really necessary, as the return value is irrelevant to
		// the VFS. It's completely ignored by OBOS as well as by the fsshell.
		WARN(("Volume::FreeQueryCookie(): connection lost, forcing free "
			"query cookie\n"));
		error = B_OK;
		disconnected = true;
	}
	int32 openQueries = atomic_add(&fOpenQueries, -1);
	if (openQueries <= 1 && disconnected)
		_PutAllPendingVNodes();
	return error;
}

// ReadQuery
status_t
Volume::ReadQuery(void* cookie, void* buffer, size_t bufferSize, int32 count,
	int32* countRead)
{
	*countRead = 0;
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	ReadQueryRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->queryCookie = cookie;
	request->bufferSize = bufferSize;
	request->count = count;
	// send the request
	KernelRequestHandler handler(this, READ_QUERY_REPLY);
	ReadQueryReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	if (reply->count < 0 || reply->count > count)
		return B_BAD_DATA;
	if ((int32)bufferSize < reply->buffer.GetSize())
		return B_BAD_DATA;
	*countRead = reply->count;
	if (*countRead > 0) {
		// copy the buffer -- limit the number of bytes to copy
		int32 maxBytes = *countRead
			* (sizeof(struct dirent) + B_FILE_NAME_LENGTH);
		int32 copyBytes = reply->buffer.GetSize();
		if (copyBytes > maxBytes)
			copyBytes = maxBytes;
		memcpy(buffer, reply->buffer.GetData(), copyBytes);
	}
	_SendReceiptAck(port);
	return error;
}

// #pragma mark -
// #pragma mark ----- private implementations -----

// _Mount
status_t
Volume::_Mount(const char* device, ulong flags, const char* parameters,
	int32 len)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);

	// get the current working directory
	char cwd[B_PATH_NAME_LENGTH];
	if (!getcwd(cwd, sizeof(cwd)))
		return errno;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	MountVolumeRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->nsid = fID;
	error = allocator.AllocateString(request->cwd, cwd);
	if (error == B_OK)
		error = allocator.AllocateString(request->device, device);
	request->flags = flags;
	if (error == B_OK)
		error = allocator.AllocateData(request->parameters, parameters, len, 1);
	if (error != B_OK)
		return error;

	// send the request
	KernelRequestHandler handler(this, MOUNT_VOLUME_REPLY);
	MountVolumeReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	fRootID = reply->rootID;
	fUserlandVolume = reply->volume;

	// enable vnode counting
	fVNodeCountMap = new(nothrow) VNodeCountMap;
	if (fVNodeCountMap)
		fVNodeCountingEnabled = true;
	else
		ERROR(("Failed to allocate vnode count map."));
	return error;
}

// _Unmount
status_t
Volume::_Unmount()
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	UnmountVolumeRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	// send the request
	KernelRequestHandler handler(this, UNMOUNT_VOLUME_REPLY);
	UnmountVolumeReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _WriteVNode
status_t
Volume::_WriteVNode(void* node, char reenter)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	WriteVNodeRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->reenter = reenter;
	// send the request
	KernelRequestHandler handler(this, WRITE_VNODE_REPLY);
	WriteVNodeReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _Close
status_t
Volume::_Close(void* node, void* cookie)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	CloseRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->fileCookie = cookie;
	// send the request
	KernelRequestHandler handler(this, CLOSE_REPLY);
	CloseReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _FreeCookie
status_t
Volume::_FreeCookie(void* node, void* cookie)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FreeCookieRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->fileCookie = cookie;
	// send the request
	KernelRequestHandler handler(this, FREE_COOKIE_REPLY);
	FreeCookieReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _CloseDir
status_t
Volume::_CloseDir(void* node, void* cookie)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	CloseDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->dirCookie = cookie;
	// send the request
	KernelRequestHandler handler(this, CLOSE_DIR_REPLY);
	CloseDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _FreeDirCookie
status_t
Volume::_FreeDirCookie(void* node, void* cookie)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FreeDirCookieRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->dirCookie = cookie;
	// send the request
	KernelRequestHandler handler(this, FREE_DIR_COOKIE_REPLY);
	FreeDirCookieReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _Walk
status_t
Volume::_Walk(void* dir, const char* entryName, char** resolvedPath,
	vnode_id* vnid)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	WalkRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = dir;
	error = allocator.AllocateString(request->entryName, entryName);
	request->traverseLink = resolvedPath;
	if (error != B_OK)
		return error;
	// send the request
	KernelRequestHandler handler(this, WALK_REPLY);
	WalkReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	*vnid = reply->vnid;
	if (resolvedPath) {
		const char* readPath = (const char*)reply->resolvedPath.GetData();
		if (readPath) {
			int32 len = strnlen(readPath, reply->resolvedPath.GetSize());
			*resolvedPath = (char*)malloc(len + 1);
			if (*resolvedPath) {
				memcpy(*resolvedPath, readPath, len);
				(*resolvedPath)[len] = '\0';
			} else
				error = B_NO_MEMORY;
			_SendReceiptAck(port);
		} else
			_DecrementVNodeCount(*vnid);
	} else
		_DecrementVNodeCount(*vnid);
			// The VFS will balance the get_vnode() call for the FS.
	return error;
}

// _CloseAttrDir
status_t
Volume::_CloseAttrDir(void* node, void* cookie)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	CloseAttrDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->attrDirCookie = cookie;
	// send the request
	KernelRequestHandler handler(this, CLOSE_ATTR_DIR_REPLY);
	CloseAttrDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _FreeAttrDirCookie
status_t
Volume::_FreeAttrDirCookie(void* node, void* cookie)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FreeAttrDirCookieRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->node = node;
	request->attrDirCookie = cookie;
	// send the request
	KernelRequestHandler handler(this, FREE_ATTR_DIR_COOKIE_REPLY);
	FreeAttrDirCookieReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _CloseIndexDir
status_t
Volume::_CloseIndexDir(void* cookie)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	CloseIndexDirRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->indexDirCookie = cookie;
	// send the request
	KernelRequestHandler handler(this, CLOSE_INDEX_DIR_REPLY);
	CloseIndexDirReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _FreeIndexDirCookie
status_t
Volume::_FreeIndexDirCookie(void* cookie)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FreeIndexDirCookieRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->indexDirCookie = cookie;
	// send the request
	KernelRequestHandler handler(this, FREE_INDEX_DIR_COOKIE_REPLY);
	FreeIndexDirCookieReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _CloseQuery
status_t
Volume::_CloseQuery(void* cookie)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	CloseQueryRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->queryCookie = cookie;
	// send the request
	KernelRequestHandler handler(this, CLOSE_QUERY_REPLY);
	CloseQueryReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _FreeQueryCookie
status_t
Volume::_FreeQueryCookie(void* cookie)
{
	// get a free port
	RequestPort* port = fFileSystem->GetPortPool()->AcquirePort();
	if (!port)
		return B_ERROR;
	PortReleaser _(fFileSystem->GetPortPool(), port);
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	FreeQueryCookieRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->volume = fUserlandVolume;
	request->queryCookie = cookie;
	// send the request
	KernelRequestHandler handler(this, FREE_QUERY_COOKIE_REPLY);
	FreeQueryCookieReply* reply;
	error = _SendRequest(port, &allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// _SendRequest
status_t
Volume::_SendRequest(RequestPort* port, RequestAllocator* allocator,
	RequestHandler* handler, Request** reply)
{
	if (!fFileSystem->IsUserlandServerThread())
		return port->SendRequest(allocator, handler, reply);
	// Here it gets dangerous: a thread of the userland server team being here
	// calls for trouble. We try receiving the request with a timeout, and
	// close the port -- which will disconnect the whole FS.
	status_t error = port->SendRequest(allocator, handler, reply,
		kUserlandServerlandPortTimeout);
	if (error == B_TIMED_OUT || error == B_WOULD_BLOCK)
		port->Close();
	return error;
}

// _SendReceiptAck
status_t
Volume::_SendReceiptAck(RequestPort* port)
{
	RequestAllocator allocator(port->GetPort());
	ReceiptAckReply* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	return port->SendRequest(&allocator);
}

// _IncrementVNodeCount
void
Volume::_IncrementVNodeCount(vnode_id vnid)
{
	if (!fVNodeCountingEnabled)
		return;
	AutoLocker<VNodeCountMap> _(fVNodeCountMap);
	if (!fVNodeCountingEnabled)	// someone may have changed it
		return;
	// get the counter
	int32* count = fVNodeCountMap->Get(vnid);
	if (!count) {
		// vnode not known yet: create and add a new counter
		count = new(nothrow) int32(0);
		if (!count) {
			ERROR(("Volume::_IncrementVNodeCount(): Failed to allocate "
				"counter. Disabling vnode counting.\n"));
			fVNodeCountingEnabled = false;
			return;
		}
		if (fVNodeCountMap->Put(vnid, count) != B_OK) {
			ERROR(("Volume::_IncrementVNodeCount(): Failed to add counter. "
				"Disabling vnode counting.\n"));
			delete count;
			fVNodeCountingEnabled = false;
			return;
		}
	}
	// increment the counter
	(*count)++;
//PRINT(("_IncrementVNodeCount(%Ld): count: %ld, fVNodeCountMap size: %ld\n", vnid, *count, fVNodeCountMap->Size()));
}

// _DecrementVNodeCount
void
Volume::_DecrementVNodeCount(vnode_id vnid)
{
	if (!fVNodeCountingEnabled)
		return;
	AutoLocker<VNodeCountMap> _(fVNodeCountMap);
	if (!fVNodeCountingEnabled)	// someone may have changed it
		return;
	int32* count = fVNodeCountMap->Get(vnid);
	if (!count) {
		// that should never happen
		ERROR(("Volume::_DecrementVNodeCount(): Failed to get counter. "
			"Disabling vnode counting.\n"));
		fVNodeCountingEnabled = false;
		return;
	}
	(*count)--;
//int32 tmpCount = *count;
	if (*count == 0)
		fVNodeCountMap->Remove(vnid);
//PRINT(("_DecrementVNodeCount(%Ld): count: %ld, fVNodeCountMap size: %ld\n", vnid, tmpCount, fVNodeCountMap->Size()));
}

// _InternalIOCtl
status_t
Volume::_InternalIOCtl(userlandfs_ioctl* buffer, int32 bufferSize)
{
	if (buffer->version != USERLAND_IOCTL_CURRENT_VERSION)
		return B_BAD_VALUE;
	status_t result = B_OK;
	switch (buffer->command) {
		case USERLAND_IOCTL_PUT_ALL_PENDING_VNODES:
			result = _PutAllPendingVNodes();
			break;
		default:
			return B_BAD_VALUE;
	}
	buffer->error = result;
	return B_OK;
}

// _PutAllPendingVNodes
status_t
Volume::_PutAllPendingVNodes()
{
PRINT(("Volume::_PutAllPendingVNodes()\n"));
	if (!fFileSystem->GetPortPool()->IsDisconnected()) {
		PRINT(("Volume::_PutAllPendingVNodes() failed: still connected\n"));
		return USERLAND_IOCTL_STILL_CONNECTED;
	}
	if (!fVNodeCountingEnabled) {
		PRINT(("Volume::_PutAllPendingVNodes() failed: vnode counting "
			"disabled\n"));
		return USERLAND_IOCTL_VNODE_COUNTING_DISABLED;
	}
	{
		AutoLocker<VNodeCountMap> _(fVNodeCountMap);
		if (!fVNodeCountingEnabled)	{// someone may have changed it
			PRINT(("Volume::_PutAllPendingVNodes() failed: vnode counting "
				"disabled\n"));
			return USERLAND_IOCTL_VNODE_COUNTING_DISABLED;
		}
		// Check whether there are open entities at the moment.
		if (fOpenFiles > 0) {
			PRINT(("Volume::_PutAllPendingVNodes() failed: open files\n"));
			return USERLAND_IOCTL_OPEN_FILES;
		}
		if (fOpenDirectories > 0) {
			PRINT(("Volume::_PutAllPendingVNodes() failed: open dirs\n"));
			return USERLAND_IOCTL_OPEN_DIRECTORIES;
		}
		if (fOpenAttributeDirectories > 0) {
			PRINT(("Volume::_PutAllPendingVNodes() failed: open attr dirs\n"));
			return USERLAND_IOCTL_OPEN_ATTRIBUTE_DIRECTORIES;
		}
		if (fOpenIndexDirectories > 0) {
			PRINT(("Volume::_PutAllPendingVNodes() failed: open index dirs\n"));
			return USERLAND_IOCTL_OPEN_INDEX_DIRECTORIES;
		}
		if (fOpenQueries > 0) {
			PRINT(("Volume::_PutAllPendingVNodes() failed: open queries\n"));
			return USERLAND_IOCTL_OPEN_QUERIES;
		}
		// No open entities. Since the port pool is disconnected, no new
		// entities can be opened. Disable node counting and put all pending
		// vnodes.
		fVNodeCountingEnabled = false;
	}
	int32 putVNodeCount = 0;
	for (VNodeCountMap::Iterator it = fVNodeCountMap->GetIterator();
		 it.HasNext();) {
		VNodeCountMap::Entry entry = it.Next();
		int32 count = *entry.value;
		for (int32 i = 0; i < count; i++) {
			PutVNode(entry.key.value);
			putVNodeCount++;
		}
	}
	PRINT(("Volume::_PutAllPendingVNodes() successful: Put %ld vnodes\n",
		putVNodeCount));
	return B_OK;
}

