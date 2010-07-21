/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "checksum_device.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>

#include <device_manager.h>

#include <AutoDeleter.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>

#include <fs/KPath.h>
#include <lock.h>
#include <vm/vm.h>

#include "dma_resources.h"
#include "io_requests.h"
#include "IOSchedulerSimple.h"

#include "CheckSum.h"


//#define TRACE_CHECK_SUM_DEVICE
#ifdef TRACE_CHECK_SUM_DEVICE
#	define TRACE(x...)	dprintf(x)
#else
#	define TRACE(x) do {} while (false)
#endif


// parameters for the DMA resource
static const uint32 kDMAResourceBufferCount			= 16;
static const uint32 kDMAResourceBounceBufferCount	= 16;

static const size_t kCheckSumLength = sizeof(CheckSum);
static const uint32 kCheckSumsPerBlock = B_PAGE_SIZE / sizeof(CheckSum);

static const char* const kDriverModuleName
	= "drivers/disk/virtual/checksum_device/driver_v1";
static const char* const kControlDeviceModuleName
	= "drivers/disk/virtual/checksum_device/control/device_v1";
static const char* const kRawDeviceModuleName
	= "drivers/disk/virtual/checksum_device/raw/device_v1";

static const char* const kControlDeviceName
	= "disk/virtual/checksum_device/control";
static const char* const kRawDeviceBaseName = "disk/virtual/checksum_device";

static const char* const kFilePathItem = "checksum_device/file_path";


struct RawDevice;
typedef DoublyLinkedList<RawDevice> RawDeviceList;

struct device_manager_info* sDeviceManager;

static RawDeviceList sDeviceList;
static mutex sDeviceListLock = MUTEX_INITIALIZER("checksum device list");


struct CheckSumBlock : public DoublyLinkedListLinkImpl<CheckSumBlock> {
	uint64		blockIndex;
	bool		used;
	bool		dirty;
	CheckSum	checkSums[kCheckSumsPerBlock];

	CheckSumBlock()
		:
		used(false)
	{
	}
};


struct CheckSumCache {
	CheckSumCache()
	{
		mutex_init(&fLock, "check sum cache");
	}

	~CheckSumCache()
	{
		while (CheckSumBlock* block = fBlocks.RemoveHead())
			delete block;

		mutex_destroy(&fLock);
	}

	status_t Init(int fd, uint64 blockCount, uint32 cachedBlockCount)
	{
		fBlockCount = blockCount;
		fFD = fd;

		for (uint32 i = 0; i < cachedBlockCount; i++) {
			CheckSumBlock* block = new(std::nothrow) CheckSumBlock;
			if (block == NULL)
				return B_NO_MEMORY;

			fBlocks.Add(block);
		}

		return B_OK;
	}

	status_t GetCheckSum(uint64 blockIndex, CheckSum& checkSum)
	{
		ASSERT(blockIndex < fBlockCount);

		MutexLocker locker(fLock);

		CheckSumBlock* block;
		status_t error = _GetBlock(
			fBlockCount + blockIndex / kCheckSumsPerBlock, block);
		if (error != B_OK)
			return error;

		checkSum = block->checkSums[blockIndex % kCheckSumsPerBlock];

		return B_OK;
	}

	status_t SetCheckSum(uint64 blockIndex, const CheckSum& checkSum)
	{
		ASSERT(blockIndex < fBlockCount);

		MutexLocker locker(fLock);

		CheckSumBlock* block;
		status_t error = _GetBlock(
			fBlockCount + blockIndex / kCheckSumsPerBlock, block);
		if (error != B_OK)
			return error;

		block->checkSums[blockIndex % kCheckSumsPerBlock] = checkSum;
		block->dirty = true;

#ifdef TRACE_CHECK_SUM_DEVICE
		TRACE("checksum_device: setting check sum of block %" B_PRIu64 " to: ",
			blockIndex);
		for (size_t i = 0; i < kCheckSumLength; i++)
			TRACE("%02x", checkSum.Data()[i]);
		TRACE("\n");
#endif

		return B_OK;
	}

private:
	typedef DoublyLinkedList<CheckSumBlock> BlockList;

private:
	status_t _GetBlock(uint64 blockIndex, CheckSumBlock*& _block)
	{
		// check whether we have already cached the block
		for (BlockList::Iterator it = fBlocks.GetIterator();
			CheckSumBlock* block = it.Next();) {
			if (block->used && blockIndex == block->blockIndex) {
				// we know it -- requeue and return
				it.Remove();
				fBlocks.Add(block);
				_block = block;
				return B_OK;
			}
		}

		// flush the least recently used block and recycle it
		CheckSumBlock* block = fBlocks.Head();
		status_t error = _FlushBlock(block);
		if (error != B_OK)
			return error;

		error = _ReadBlock(block, blockIndex);
		if (error != B_OK)
			return error;

		// requeue
		fBlocks.Remove(block);
		fBlocks.Add(block);

		_block = block;
		return B_OK;
	}

	status_t _FlushBlock(CheckSumBlock* block)
	{
		if (!block->used || !block->dirty)
			return B_OK;

		ssize_t written = pwrite(fFD, block->checkSums, B_PAGE_SIZE,
			block->blockIndex * B_PAGE_SIZE);
		if (written < 0)
			return errno;
		if (written != B_PAGE_SIZE)
			return B_ERROR;

		block->dirty = false;
		return B_OK;
	}

	status_t _ReadBlock(CheckSumBlock* block, uint64 blockIndex)
	{
		// mark unused for the failure cases -- reset later
		block->used = false;

		ssize_t bytesRead = pread(fFD, block->checkSums, B_PAGE_SIZE,
			blockIndex * B_PAGE_SIZE);
		if (bytesRead < 0)
			return errno;
		if (bytesRead != B_PAGE_SIZE)
			return B_ERROR;

		block->blockIndex = blockIndex;
		block->used = true;
		block->dirty = false;

		return B_OK;
	}

private:
	mutex		fLock;
	uint64		fBlockCount;
	int			fFD;
	BlockList	fBlocks;	// LRU first
};


struct Device {
	Device(device_node* node)
		:
		fNode(node)
	{
		mutex_init(&fLock, "checksum device");
	}

	virtual ~Device()
	{
		mutex_destroy(&fLock);
	}

	bool Lock()		{ mutex_lock(&fLock); return true; }
	void Unlock()	{ mutex_unlock(&fLock); }

	device_node* Node() const	{ return fNode; }

	virtual status_t PublishDevice() = 0;

protected:
	mutex			fLock;
	device_node*	fNode;
};


struct ControlDevice : Device {
	ControlDevice(device_node* node)
		:
		Device(node)
	{
	}

	status_t Register(const char* fileName)
	{
		device_attr attrs[] = {
			{B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
				{string: "Checksum Raw Device"}},
			{kFilePathItem, B_STRING_TYPE, {string: fileName}},
			{NULL}
		};

		return sDeviceManager->register_node(
			sDeviceManager->get_parent_node(Node()), kDriverModuleName, attrs,
			NULL, NULL);
	}

	virtual status_t PublishDevice()
	{
		return sDeviceManager->publish_device(Node(), kControlDeviceName,
			kControlDeviceModuleName);
	}
};


struct RawDevice : Device, DoublyLinkedListLinkImpl<RawDevice> {
	RawDevice(device_node* node)
		:
		Device(node),
		fIndex(-1),
		fFD(-1),
		fFileSize(0),
		fDeviceSize(0),
		fDeviceName(NULL),
		fDMAResource(NULL),
		fIOScheduler(NULL),
		fTransferBuffer(NULL),
		fCheckSumCache(NULL)
	{
	}

	virtual ~RawDevice()
	{
		if (fIndex >= 0) {
			MutexLocker locker(sDeviceListLock);
			sDeviceList.Remove(this);
		}

		if (fFD >= 0)
			close(fFD);

		free(fDeviceName);
	}

	int32 Index() const				{ return fIndex; }
	off_t DeviceSize() const		{ return fDeviceSize; }
	const char* DeviceName() const	{ return fDeviceName; }

	status_t Init(const char* fileName)
	{
		// open file/device
		fFD = open(fileName, O_RDWR | O_NOCACHE);
			// TODO: The O_NOCACHE is a work-around for a page writer problem.
			// Since it collects pages for writing back without regard for
			// which caches and file systems they belong to, a deadlock can
			// result when pages from both the underlying file system and the
			// one using the checksum device are collected in one run.
		if (fFD < 0)
			return errno;

		// get the size
		struct stat st;
		if (fstat(fFD, &st) < 0)
			return errno;

		switch (st.st_mode & S_IFMT) {
			case S_IFREG:
				fFileSize = st.st_size;
				break;
			case S_IFCHR:
			case S_IFBLK:
			{
				device_geometry geometry;
				if (ioctl(fFD, B_GET_GEOMETRY, &geometry, sizeof(geometry)) < 0)
					return errno;

				fFileSize = (off_t)geometry.bytes_per_sector
					* geometry.sectors_per_track
					* geometry.cylinder_count * geometry.head_count;
				break;
			}
			default:
				return B_BAD_VALUE;
		}

		fFileSize = fFileSize / B_PAGE_SIZE * B_PAGE_SIZE;
		fDeviceSize = fFileSize / (B_PAGE_SIZE + kCheckSumLength) * B_PAGE_SIZE;

		// find a free slot
		fIndex = 0;
		RawDevice* nextDevice = NULL;
		MutexLocker locker(sDeviceListLock);
		for (RawDeviceList::Iterator it = sDeviceList.GetIterator();
				(nextDevice = it.Next()) != NULL;) {
			if (nextDevice->Index() > fIndex)
				break;
			fIndex = nextDevice->Index() + 1;
		}

		sDeviceList.InsertBefore(nextDevice, this);

		// construct our device path
		KPath path(kRawDeviceBaseName);
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "%" B_PRId32 "/raw", fIndex);

		status_t error = path.Append(buffer);
		if (error != B_OK)
			return error;

		fDeviceName = path.DetachBuffer();

		return B_OK;
	}

	status_t Prepare()
	{
		fCheckSumCache = new(std::nothrow) CheckSumCache;
		if (fCheckSumCache == NULL) {
			Unprepare();
			return B_NO_MEMORY;
		}

		status_t error = fCheckSumCache->Init(fFD, fDeviceSize / B_PAGE_SIZE,
			16);
		if (error != B_OK) {
			Unprepare();
			return error;
		}

		// no DMA restrictions
		const dma_restrictions restrictions = {};

		fDMAResource = new(std::nothrow) DMAResource;
		if (fDMAResource == NULL) {
			Unprepare();
			return B_NO_MEMORY;
		}

		error = fDMAResource->Init(restrictions, B_PAGE_SIZE,
			kDMAResourceBufferCount, kDMAResourceBounceBufferCount);
		if (error != B_OK) {
			Unprepare();
			return error;
		}

		fIOScheduler = new(std::nothrow) IOSchedulerSimple(fDMAResource);
		if (fIOScheduler == NULL) {
			Unprepare();
			return B_NO_MEMORY;
		}

		error = fIOScheduler->Init("checksum device scheduler");
		if (error != B_OK) {
			Unprepare();
			return error;
		}

		fIOScheduler->SetCallback(&_DoIOEntry, this);

		fTransferBuffer = malloc(B_PAGE_SIZE);
		if (fTransferBuffer == NULL) {
			Unprepare();
			return B_NO_MEMORY;
		}

		return B_OK;
	}

	void Unprepare()
	{
		free(fTransferBuffer);
		fTransferBuffer = NULL;

		delete fIOScheduler;
		fIOScheduler = NULL;

		delete fDMAResource;
		fDMAResource = NULL;

		delete fCheckSumCache;
		fCheckSumCache = NULL;
	}

	status_t DoIO(IORequest* request)
	{
		return fIOScheduler->ScheduleRequest(request);
	}

	virtual status_t PublishDevice()
	{
		return sDeviceManager->publish_device(Node(), fDeviceName,
			kRawDeviceModuleName);
	}

	status_t GetBlockCheckSum(uint64 blockIndex, CheckSum& checkSum)
	{
		return fCheckSumCache->GetCheckSum(blockIndex, checkSum);
	}

	status_t SetBlockCheckSum(uint64 blockIndex, const CheckSum& checkSum)
	{
		return fCheckSumCache->SetCheckSum(blockIndex, checkSum);
	}

private:
	static status_t _DoIOEntry(void* data, IOOperation* operation)
	{
		return ((RawDevice*)data)->_DoIO(operation);
	}

	status_t _DoIO(IOOperation* operation)
	{
		off_t offset = operation->Offset();
		generic_size_t length = operation->Length();

		ASSERT(offset % B_PAGE_SIZE == 0);
		ASSERT(length % B_PAGE_SIZE == 0);

		const generic_io_vec* vecs = operation->Vecs();
		generic_size_t vecOffset = 0;
		bool isWrite = operation->IsWrite();

		while (length > 0) {
			status_t error = _TransferBlock(vecs, vecOffset, offset, isWrite);
			if (error != B_OK) {
				fIOScheduler->OperationCompleted(operation, error, 0);
				return error;
			}

			offset += B_PAGE_SIZE;
			length -= B_PAGE_SIZE;
		}

		fIOScheduler->OperationCompleted(operation, B_OK, operation->Length());
		return B_OK;
	}

	status_t _TransferBlock(const generic_io_vec*& vecs,
		generic_size_t& vecOffset, off_t offset, bool isWrite)
	{
		if (isWrite) {
			// write -- copy data to transfer buffer
			status_t error = _CopyData(vecs, vecOffset, true);
			if (error != B_OK)
				return error;
			_CheckCheckSum(offset / B_PAGE_SIZE);
		}

		ssize_t transferred = isWrite
			? pwrite(fFD, fTransferBuffer, B_PAGE_SIZE, offset)
			: pread(fFD, fTransferBuffer, B_PAGE_SIZE, offset);

		if (transferred < 0)
			return errno;
		if (transferred != B_PAGE_SIZE)
			return B_ERROR;

		if (!isWrite) {
			// read -- copy data from transfer buffer
			status_t error =_CopyData(vecs, vecOffset, false);
			if (error != B_OK)
				return error;
		}

		return B_OK;
	}

	status_t _CopyData(const generic_io_vec*& vecs, generic_size_t& vecOffset,
		bool toBuffer)
	{
		uint8* buffer = (uint8*)fTransferBuffer;
		size_t length = B_PAGE_SIZE;
		while (length > 0) {
			size_t toCopy = std::min((generic_size_t)length,
				vecs->length - vecOffset);

			if (toCopy == 0) {
				vecs++;
				vecOffset = 0;
				continue;
			}

			phys_addr_t vecAddress = vecs->base + vecOffset;

			status_t error = toBuffer
				? vm_memcpy_from_physical(buffer, vecAddress, toCopy, false)
				: vm_memcpy_to_physical(vecAddress, buffer, toCopy, false);
			if (error != B_OK)
				return error;

			buffer += toCopy;
			length -= toCopy;
			vecOffset += toCopy;
		}

		return B_OK;
	}

	void _CheckCheckSum(uint64 blockIndex)
	{
		// get the checksum the block should have
		CheckSum expectedCheckSum;
		if (fCheckSumCache->GetCheckSum(blockIndex, expectedCheckSum) != B_OK)
			return;

		// if the checksum is clear, we aren't supposed to check
		if (expectedCheckSum.IsZero()) {
			dprintf("checksum_device: skipping check sum check for block %"
				B_PRIu64 "\n", blockIndex);
			return;
		}

		// compute the transfer buffer check sum
		fSHA256.Init();
		fSHA256.Update(fTransferBuffer, B_PAGE_SIZE);

		if (expectedCheckSum != fSHA256.Digest())
			panic("Check sum mismatch for block %" B_PRIu64 " (exptected at %p"
				", actual at %p)", blockIndex, &expectedCheckSum,
				fSHA256.Digest());
	}

private:
	int32			fIndex;
	int				fFD;
	off_t			fFileSize;
	off_t			fDeviceSize;
	char*			fDeviceName;
	DMAResource*	fDMAResource;
	IOScheduler*	fIOScheduler;
	void*			fTransferBuffer;
	CheckSumCache*	fCheckSumCache;
	SHA256			fSHA256;
};


struct RawDeviceCookie {
	RawDeviceCookie(RawDevice* device, int openMode)
		:
		fDevice(device),
		fOpenMode(openMode)
	{
	}

	RawDevice* Device() const	{ return fDevice; }
	int OpenMode() const		{ return fOpenMode; }

private:
	RawDevice*	fDevice;
	int			fOpenMode;
};


// #pragma mark -


static bool
parse_command_line(char* buffer, char**& _argv, int& _argc)
{
	// Process the argument string. We split at whitespace, heeding quotes and
	// escaped characters. The processed arguments are written to the given
	// buffer, separated by single null chars.
	char* start = buffer;
	char* out = buffer;
	bool pendingArgument = false;
	int argc = 0;
	while (*start != '\0') {
		// ignore whitespace
		if (isspace(*start)) {
			if (pendingArgument) {
				*out = '\0';
				out++;
				argc++;
				pendingArgument = false;
			}
			start++;
			continue;
		}

		pendingArgument = true;

		if (*start == '"' || *start == '\'') {
			// quoted text -- continue until closing quote
			char quote = *start;
			start++;
			while (*start != '\0' && *start != quote) {
				if (*start == '\\' && quote == '"') {
					start++;
					if (*start == '\0')
						break;
				}
				*out = *start;
				start++;
				out++;
			}

			if (*start != '\0')
				start++;
		} else {
			// unquoted text
			if (*start == '\\') {
				// escaped char
				start++;
				if (start == '\0')
					break;
			}

			*out = *start;
			start++;
			out++;
		}
	}

	if (pendingArgument) {
		*out = '\0';
		argc++;
	}

	// allocate argument vector
	char** argv = new(std::nothrow) char*[argc + 1];
	if (argv == NULL)
		return false;

	// fill vector
	start = buffer;
	for (int i = 0; i < argc; i++) {
		argv[i] = start;
		start += strlen(start) + 1;
	}
	argv[argc] = NULL;

	_argv = argv;
	_argc = argc;
	return true;
}


//	#pragma mark - driver


static float
checksum_driver_supports_device(device_node* parent)
{
	const char* bus = NULL;
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false)
			== B_OK && !strcmp(bus, "generic"))
		return 0.8;

	return -1;
}


static status_t
checksum_driver_register_device(device_node* parent)
{
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{string: "Checksum Control Device"}},
		{NULL}
	};

	return sDeviceManager->register_node(parent, kDriverModuleName, attrs, NULL,
		NULL);
}


static status_t
checksum_driver_init_driver(device_node* node, void** _driverCookie)
{
	const char* fileName;
	if (sDeviceManager->get_attr_string(node, kFilePathItem, &fileName, false)
			== B_OK) {
		RawDevice* device = new(std::nothrow) RawDevice(node);
		if (device == NULL)
			return B_NO_MEMORY;

		status_t error = device->Init(fileName);
		if (error != B_OK) {
			delete device;
			return error;
		}

		*_driverCookie = (Device*)device;
	} else {
		ControlDevice* device = new(std::nothrow) ControlDevice(node);
		if (device == NULL)
			return B_NO_MEMORY;

		*_driverCookie = (Device*)device;
	}

	return B_OK;
}


static void
checksum_driver_uninit_driver(void* driverCookie)
{
	Device* device = (Device*)driverCookie;
	delete device;
}


static status_t
checksum_driver_register_child_devices(void* driverCookie)
{
	Device* device = (Device*)driverCookie;
	return device->PublishDevice();
}


//	#pragma mark - control device


static status_t
checksum_control_device_init_device(void* driverCookie, void** _deviceCookie)
{
	*_deviceCookie = driverCookie;
	return B_OK;
}


static void
checksum_control_device_uninit_device(void* deviceCookie)
{
}


static status_t
checksum_control_device_open(void* deviceCookie, const char* path, int openMode,
	void** _cookie)
{
	*_cookie = deviceCookie;
	return B_OK;
}


static status_t
checksum_control_device_close(void* cookie)
{
	return B_OK;
}


static status_t
checksum_control_device_free(void* cookie)
{
	return B_OK;
}


static status_t
checksum_control_device_read(void* cookie, off_t position, void* buffer,
	size_t* _length)
{
	*_length = 0;
	return B_OK;
}


static status_t
checksum_control_device_write(void* cookie, off_t position, const void* data,
	size_t* _length)
{
	ControlDevice* device = (ControlDevice*)cookie;

	if (position != 0)
		return B_BAD_VALUE;

	// copy data to stack buffer
	char* buffer = (char*)malloc(*_length + 1);
	if (buffer == NULL)
		return B_NO_MEMORY;
	MemoryDeleter bufferDeleter(buffer);

	if (IS_USER_ADDRESS(data)) {
		if (user_memcpy(buffer, data, *_length) != B_OK)
			return B_BAD_ADDRESS;
	} else
		memcpy(buffer, data, *_length);

	buffer[*_length] = '\0';

	// parse arguments
	char** argv;
	int argc;
	if (!parse_command_line(buffer, argv, argc))
		return B_NO_MEMORY;
	ArrayDeleter<char*> argvDeleter(argv);

	if (argc == 0) {
		dprintf("\"help\" for usage!\n");
		return B_BAD_VALUE;
	}

	// execute command
	if (strcmp(argv[0], "help") == 0) {
		// help
		dprintf("register <path>\n");
		dprintf("  Registers file <path> as a new checksum device.\n");
		dprintf("unregister <device>\n");
		dprintf("  Unregisters <device>.\n");
	} else if (strcmp(argv[0], "register") == 0) {
		// register
		if (argc != 2) {
			dprintf("Usage: register <path>\n");
			return B_BAD_VALUE;
		}

		return device->Register(argv[1]);
	} else if (strcmp(argv[0], "unregister") == 0) {
		// unregister
		if (argc != 2) {
			dprintf("Usage: unregister <device>\n");
			return B_BAD_VALUE;
		}

		const char* deviceName = argv[1];
		if (strncmp(deviceName, "/dev/", 5) == 0)
			deviceName += 5;

		// find the device in the list and unregister it
		MutexLocker locker(sDeviceListLock);
		for (RawDeviceList::Iterator it = sDeviceList.GetIterator();
				RawDevice* device = it.Next();) {
			if (strcmp(device->DeviceName(), deviceName) == 0) {
				// TODO: Race condition: We should mark the device as going to
				// be unregistered, so no one else can try the same after we
				// unlock!
				locker.Unlock();
// TODO: The following doesn't work! unpublish_device(), as per implementation
// (partially commented out) and unregister_node() returns B_BUSY.
				status_t error = sDeviceManager->unpublish_device(
					device->Node(), device->DeviceName());
				if (error != B_OK) {
					dprintf("Failed to unpublish device \"%s\": %s\n",
						deviceName, strerror(error));
					return error;
				}

				error = sDeviceManager->unregister_node(device->Node());
				if (error != B_OK) {
					dprintf("Failed to unregister node \"%s\": %s\n",
						deviceName, strerror(error));
					return error;
				}

				return B_OK;
			}
		}

		dprintf("Device \"%s\" not found!\n", deviceName);
		return B_BAD_VALUE;
	} else {
		dprintf("Invalid command \"%s\"!\n", argv[0]);
		return B_BAD_VALUE;
	}

	return B_OK;
}


static status_t
checksum_control_device_control(void* cookie, uint32 op, void* buffer,
	size_t length)
{
	return B_BAD_VALUE;
}


//	#pragma mark - raw device


static status_t
checksum_raw_device_init_device(void* driverCookie, void** _deviceCookie)
{
	RawDevice* device = static_cast<RawDevice*>((Device*)driverCookie);

	status_t error = device->Prepare();
	if (error != B_OK)
		return error;

	*_deviceCookie = device;
	return B_OK;
}


static void
checksum_raw_device_uninit_device(void* deviceCookie)
{
	RawDevice* device = (RawDevice*)deviceCookie;
	device->Unprepare();
}


static status_t
checksum_raw_device_open(void* deviceCookie, const char* path, int openMode,
	void** _cookie)
{
	RawDevice* device = (RawDevice*)deviceCookie;

	RawDeviceCookie* cookie = new(std::nothrow) RawDeviceCookie(device,
		openMode);
	if (cookie == NULL)
		return B_NO_MEMORY;

	*_cookie = cookie;
	return B_OK;
}


static status_t
checksum_raw_device_close(void* cookie)
{
	return B_OK;
}


static status_t
checksum_raw_device_free(void* _cookie)
{
	RawDeviceCookie* cookie = (RawDeviceCookie*)_cookie;
	delete cookie;
	return B_OK;
}


static status_t
checksum_raw_device_read(void* _cookie, off_t pos, void* buffer,
	size_t* _length)
{
	RawDeviceCookie* cookie = (RawDeviceCookie*)_cookie;
	RawDevice* device = cookie->Device();

	size_t length = *_length;

	if (pos >= device->DeviceSize())
		return B_BAD_VALUE;
	if (pos + length > device->DeviceSize())
		length = device->DeviceSize() - pos;

	IORequest request;
	status_t status = request.Init(pos, (addr_t)buffer, length, false, 0);
	if (status != B_OK)
		return status;

	status = device->DoIO(&request);
	if (status != B_OK)
		return status;

	status = request.Wait(0, 0);
	if (status == B_OK)
		*_length = length;
	return status;
}


static status_t
checksum_raw_device_write(void* _cookie, off_t pos, const void* buffer,
	size_t* _length)
{
	RawDeviceCookie* cookie = (RawDeviceCookie*)_cookie;
	RawDevice* device = cookie->Device();

	size_t length = *_length;

	if (pos >= device->DeviceSize())
		return B_BAD_VALUE;
	if (pos + length > device->DeviceSize())
		length = device->DeviceSize() - pos;

	IORequest request;
	status_t status = request.Init(pos, (addr_t)buffer, length, true, 0);
	if (status != B_OK)
		return status;

	status = device->DoIO(&request);
	if (status != B_OK)
		return status;

	status = request.Wait(0, 0);
	if (status == B_OK)
		*_length = length;

	return status;
}


static status_t
checksum_raw_device_io(void* _cookie, io_request* request)
{
	RawDeviceCookie* cookie = (RawDeviceCookie*)_cookie;
	RawDevice* device = cookie->Device();

	return device->DoIO(request);
}


static status_t
checksum_raw_device_control(void* _cookie, uint32 op, void* buffer,
	size_t length)
{
	RawDeviceCookie* cookie = (RawDeviceCookie*)_cookie;
	RawDevice* device = cookie->Device();

	switch (op) {
		case B_GET_DEVICE_SIZE:
		{
			size_t size = device->DeviceSize();
			return user_memcpy(buffer, &size, sizeof(size_t));
		}

		case B_SET_NONBLOCKING_IO:
		case B_SET_BLOCKING_IO:
			return B_OK;

		case B_GET_READ_STATUS:
		case B_GET_WRITE_STATUS:
		{
			bool value = true;
			return user_memcpy(buffer, &value, sizeof(bool));
		}

		case B_GET_GEOMETRY:
		case B_GET_BIOS_GEOMETRY:
		{
			device_geometry geometry;
			geometry.bytes_per_sector = B_PAGE_SIZE;
			geometry.sectors_per_track = 1;
			geometry.cylinder_count = device->DeviceSize() / B_PAGE_SIZE;
				// TODO: We're limited to 2^32 * B_PAGE_SIZE, if we don't use
				// sectors_per_track and head_count.
			geometry.head_count = 1;
			geometry.device_type = B_DISK;
			geometry.removable = true;
			geometry.read_only = false;
			geometry.write_once = false;

			return user_memcpy(buffer, &geometry, sizeof(device_geometry));
		}

		case B_GET_MEDIA_STATUS:
		{
			status_t status = B_OK;
			return user_memcpy(buffer, &status, sizeof(status_t));
		}

		case B_SET_UNINTERRUPTABLE_IO:
		case B_SET_INTERRUPTABLE_IO:
		case B_FLUSH_DRIVE_CACHE:
			return B_OK;

		case CHECKSUM_DEVICE_IOCTL_GET_CHECK_SUM:
		{
			if (IS_USER_ADDRESS(buffer)) {
				checksum_device_ioctl_check_sum getCheckSum;
				if (user_memcpy(&getCheckSum, buffer, sizeof(getCheckSum))
						!= B_OK) {
					return B_BAD_ADDRESS;
				}

				status_t error = device->GetBlockCheckSum(
					getCheckSum.blockIndex, getCheckSum.checkSum);
				if (error != B_OK)
					return error;

				return user_memcpy(buffer, &getCheckSum, sizeof(getCheckSum));
			}

			checksum_device_ioctl_check_sum* getCheckSum
				= (checksum_device_ioctl_check_sum*)buffer;
			return device->GetBlockCheckSum(getCheckSum->blockIndex,
				getCheckSum->checkSum);
		}

		case CHECKSUM_DEVICE_IOCTL_SET_CHECK_SUM:
		{
			if (IS_USER_ADDRESS(buffer)) {
				checksum_device_ioctl_check_sum setCheckSum;
				if (user_memcpy(&setCheckSum, buffer, sizeof(setCheckSum))
						!= B_OK) {
					return B_BAD_ADDRESS;
				}

				return device->SetBlockCheckSum(setCheckSum.blockIndex,
					setCheckSum.checkSum);
			}

			checksum_device_ioctl_check_sum* setCheckSum
				= (checksum_device_ioctl_check_sum*)buffer;
			return device->SetBlockCheckSum(setCheckSum->blockIndex,
				setCheckSum->checkSum);
		}
	}
	return B_BAD_VALUE;
}


// #pragma mark -


module_dependency module_dependencies[] = {
	{B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager},
	{}
};


static const struct driver_module_info sChecksumDeviceDriverModule = {
	{
		kDriverModuleName,
		0,
		NULL
	},

	checksum_driver_supports_device,
	checksum_driver_register_device,
	checksum_driver_init_driver,
	checksum_driver_uninit_driver,
	checksum_driver_register_child_devices
};

static const struct device_module_info sChecksumControlDeviceModule = {
	{
		kControlDeviceModuleName,
		0,
		NULL
	},

	checksum_control_device_init_device,
	checksum_control_device_uninit_device,
	NULL,

	checksum_control_device_open,
	checksum_control_device_close,
	checksum_control_device_free,

	checksum_control_device_read,
	checksum_control_device_write,
	NULL,	// io

	checksum_control_device_control,

	NULL,	// select
	NULL	// deselect
};

static const struct device_module_info sChecksumRawDeviceModule = {
	{
		kRawDeviceModuleName,
		0,
		NULL
	},

	checksum_raw_device_init_device,
	checksum_raw_device_uninit_device,
	NULL,

	checksum_raw_device_open,
	checksum_raw_device_close,
	checksum_raw_device_free,

	checksum_raw_device_read,
	checksum_raw_device_write,
	checksum_raw_device_io,

	checksum_raw_device_control,

	NULL,	// select
	NULL	// deselect
};

const module_info* modules[] = {
	(module_info*)&sChecksumDeviceDriverModule,
	(module_info*)&sChecksumControlDeviceModule,
	(module_info*)&sChecksumRawDeviceModule,
	NULL
};
