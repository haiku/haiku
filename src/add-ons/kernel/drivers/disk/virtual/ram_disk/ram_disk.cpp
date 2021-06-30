/*
 * Copyright 2010-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <file_systems/ram_disk/ram_disk.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>

#include <device_manager.h>
#include <Drivers.h>

#include <AutoDeleter.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>

#include <fs/KPath.h>
#include <lock.h>
#include <util/fs_trim_support.h>
#include <vm/vm.h>
#include <vm/VMCache.h>
#include <vm/vm_page.h>

#include "dma_resources.h"
#include "io_requests.h"
#include "IOSchedulerSimple.h"


//#define TRACE_RAM_DISK
#ifdef TRACE_RAM_DISK
#	define TRACE(x...)	dprintf(x)
#else
#	define TRACE(x...) do {} while (false)
#endif


static const unsigned char kRamdiskIcon[] = {
	0x6e, 0x63, 0x69, 0x66, 0x0e, 0x03, 0x01, 0x00, 0x00, 0x02, 0x00, 0x16,
	0x02, 0x3c, 0xc7, 0xee, 0x38, 0x9b, 0xc0, 0xba, 0x16, 0x57, 0x3e, 0x39,
	0xb0, 0x49, 0x77, 0xc8, 0x42, 0xad, 0xc7, 0x00, 0xff, 0xff, 0xd3, 0x02,
	0x00, 0x06, 0x02, 0x3c, 0x96, 0x32, 0x3a, 0x4d, 0x3f, 0xba, 0xfc, 0x01,
	0x3d, 0x5a, 0x97, 0x4b, 0x57, 0xa5, 0x49, 0x84, 0x4d, 0x00, 0x47, 0x47,
	0x47, 0xff, 0xa5, 0xa0, 0xa0, 0x02, 0x00, 0x16, 0x02, 0xbc, 0x59, 0x2f,
	0xbb, 0x29, 0xa7, 0x3c, 0x0c, 0xe4, 0xbd, 0x0b, 0x7c, 0x48, 0x92, 0xc0,
	0x4b, 0x79, 0x66, 0x00, 0x7d, 0xff, 0xd4, 0x02, 0x00, 0x06, 0x02, 0x38,
	0xdb, 0xb4, 0x39, 0x97, 0x33, 0xbc, 0x4a, 0x33, 0x3b, 0xa5, 0x42, 0x48,
	0x6e, 0x66, 0x49, 0xee, 0x7b, 0x00, 0x59, 0x67, 0x56, 0xff, 0xeb, 0xb2,
	0xb2, 0x03, 0xa7, 0xff, 0x00, 0x03, 0xff, 0x00, 0x00, 0x04, 0x01, 0x80,
	0x03, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x6a, 0x05, 0x33, 0x02,
	0x00, 0x06, 0x02, 0x3a, 0x5d, 0x2c, 0x39, 0xf8, 0xb1, 0xb9, 0xdb, 0xf1,
	0x3a, 0x4c, 0x0f, 0x48, 0xae, 0xea, 0x4a, 0xc0, 0x91, 0x00, 0x74, 0x74,
	0x74, 0xff, 0x3e, 0x3d, 0x3d, 0x02, 0x00, 0x16, 0x02, 0x38, 0x22, 0x1b,
	0x3b, 0x11, 0x73, 0xbc, 0x5e, 0xb5, 0x39, 0x4b, 0xaa, 0x4a, 0x47, 0xf1,
	0x49, 0xc2, 0x1d, 0x00, 0xb0, 0xff, 0x83, 0x02, 0x00, 0x16, 0x03, 0x36,
	0xed, 0xe9, 0x36, 0xb9, 0x49, 0xba, 0x0a, 0xf6, 0x3a, 0x32, 0x6f, 0x4a,
	0x79, 0xef, 0x4b, 0x03, 0xe7, 0x00, 0x5a, 0x38, 0xdc, 0xff, 0x7e, 0x0d,
	0x0a, 0x06, 0x22, 0x3c, 0x22, 0x49, 0x44, 0x5b, 0x5a, 0x3e, 0x5a, 0x31,
	0x39, 0x25, 0x0a, 0x04, 0x22, 0x3c, 0x44, 0x4b, 0x5a, 0x31, 0x39, 0x25,
	0x0a, 0x04, 0x44, 0x4b, 0x44, 0x5b, 0x5a, 0x3e, 0x5a, 0x31, 0x0a, 0x04,
	0x22, 0x3c, 0x22, 0x49, 0x44, 0x5b, 0x44, 0x4b, 0x08, 0x02, 0x27, 0x43,
	0xb8, 0x14, 0xc1, 0xf1, 0x08, 0x02, 0x26, 0x43, 0x29, 0x44, 0x0a, 0x05,
	0x44, 0x5d, 0x49, 0x5d, 0x60, 0x3e, 0x5a, 0x3b, 0x5b, 0x3f, 0x0a, 0x04,
	0x3c, 0x5a, 0x5a, 0x3c, 0x5a, 0x36, 0x3c, 0x52, 0x0a, 0x04, 0x24, 0x4e,
	0x3c, 0x5a, 0x3c, 0x52, 0x24, 0x48, 0x06, 0x07, 0xaa, 0x3f, 0x42, 0x2e,
	0x24, 0x48, 0x3c, 0x52, 0x5a, 0x36, 0x51, 0x33, 0x51, 0x33, 0x50, 0x34,
	0x4b, 0x33, 0x4d, 0x34, 0x49, 0x32, 0x49, 0x30, 0x48, 0x31, 0x49, 0x30,
	0x06, 0x08, 0xfa, 0xfa, 0x42, 0x50, 0x3e, 0x54, 0x40, 0x55, 0x3f, 0xc7,
	0xeb, 0x41, 0xc8, 0x51, 0x42, 0xc9, 0x4f, 0x42, 0xc8, 0xda, 0x42, 0xca,
	0x41, 0xc0, 0xf1, 0x5d, 0x45, 0xca, 0x81, 0x46, 0xc7, 0xb7, 0x46, 0xc8,
	0xa9, 0x46, 0xc7, 0x42, 0x44, 0x51, 0x45, 0xc6, 0xb9, 0x43, 0xc6, 0x53,
	0x0a, 0x07, 0x3c, 0x5c, 0x40, 0x5c, 0x42, 0x5e, 0x48, 0x5e, 0x4a, 0x5c,
	0x46, 0x5a, 0x45, 0x4b, 0x06, 0x09, 0x9a, 0xf6, 0x03, 0x42, 0x2e, 0x24,
	0x48, 0x4e, 0x3c, 0x5a, 0x5a, 0x3c, 0x36, 0x51, 0x33, 0x51, 0x33, 0x50,
	0x34, 0x4b, 0x33, 0x4d, 0x34, 0x49, 0x32, 0x49, 0x30, 0x48, 0x31, 0x49,
	0x30, 0x18, 0x0a, 0x07, 0x01, 0x06, 0x00, 0x0a, 0x00, 0x01, 0x00, 0x10,
	0x01, 0x17, 0x84, 0x00, 0x04, 0x0a, 0x01, 0x01, 0x01, 0x00, 0x0a, 0x02,
	0x01, 0x02, 0x00, 0x0a, 0x03, 0x01, 0x03, 0x00, 0x0a, 0x04, 0x01, 0x04,
	0x10, 0x01, 0x17, 0x85, 0x20, 0x04, 0x0a, 0x06, 0x01, 0x05, 0x30, 0x24,
	0xb3, 0x99, 0x01, 0x17, 0x82, 0x00, 0x04, 0x0a, 0x05, 0x01, 0x05, 0x30,
	0x20, 0xb2, 0xe6, 0x01, 0x17, 0x82, 0x00, 0x04, 0x0a, 0x09, 0x01, 0x0b,
	0x02, 0x3e, 0x9b, 0x12, 0xb5, 0xf9, 0x99, 0x36, 0x19, 0x10, 0x3e, 0xc0,
	0x21, 0x48, 0xed, 0x4d, 0xc8, 0x5a, 0x02, 0x0a, 0x09, 0x01, 0x0b, 0x02,
	0x3e, 0x9b, 0x12, 0xb5, 0xf9, 0x99, 0x36, 0x19, 0x10, 0x3e, 0xc0, 0x21,
	0x48, 0x4c, 0xd4, 0xc7, 0x9c, 0x11, 0x0a, 0x09, 0x01, 0x0b, 0x02, 0x3e,
	0x9b, 0x12, 0xb5, 0xf9, 0x99, 0x36, 0x19, 0x10, 0x3e, 0xc0, 0x21, 0x47,
	0x5c, 0xe7, 0xc6, 0x2c, 0x1a, 0x0a, 0x09, 0x01, 0x0b, 0x02, 0x3e, 0x9b,
	0x12, 0xb5, 0xf9, 0x99, 0x36, 0x19, 0x10, 0x3e, 0xc0, 0x21, 0x46, 0x1b,
	0xf5, 0xc4, 0x28, 0x4e, 0x0a, 0x08, 0x01, 0x0c, 0x12, 0x3e, 0xc0, 0x21,
	0xb6, 0x19, 0x10, 0x36, 0x19, 0x10, 0x3e, 0xc0, 0x21, 0x45, 0xb6, 0x34,
	0xc4, 0x22, 0x1f, 0x01, 0x17, 0x84, 0x00, 0x04, 0x0a, 0x0a, 0x01, 0x07,
	0x02, 0x3e, 0xc0, 0x21, 0xb6, 0x19, 0x10, 0x36, 0x19, 0x10, 0x3e, 0xc0,
	0x21, 0x45, 0xb6, 0x34, 0xc4, 0x22, 0x1f, 0x0a, 0x0b, 0x01, 0x08, 0x02,
	0x3e, 0xc0, 0x21, 0xb6, 0x19, 0x10, 0x36, 0x19, 0x10, 0x3e, 0xc0, 0x21,
	0x45, 0xb6, 0x34, 0xc4, 0x22, 0x1f, 0x0a, 0x0c, 0x01, 0x09, 0x02, 0x3e,
	0xc0, 0x21, 0xb6, 0x19, 0x10, 0x36, 0x19, 0x10, 0x3e, 0xc0, 0x21, 0x45,
	0xb6, 0x34, 0xc4, 0x22, 0x1f, 0x0a, 0x08, 0x01, 0x0a, 0x12, 0x3e, 0x98,
	0xfd, 0xb5, 0xf6, 0x6c, 0x35, 0xc9, 0x3d, 0x3e, 0x7b, 0x5e, 0x48, 0xf2,
	0x4e, 0xc7, 0xee, 0x3f, 0x01, 0x17, 0x84, 0x22, 0x04, 0x0a, 0x0d, 0x01,
	0x0a, 0x02, 0x3e, 0x98, 0xfd, 0xb5, 0xf6, 0x6c, 0x35, 0xc9, 0x3d, 0x3e,
	0x7b, 0x5e, 0x48, 0xf2, 0x4e, 0xc7, 0xee, 0x3f, 0x0a, 0x08, 0x01, 0x0a,
	0x12, 0x3e, 0x98, 0xfd, 0xb5, 0xf6, 0x6c, 0x35, 0xc9, 0x3d, 0x3e, 0x7b,
	0x5e, 0x48, 0x53, 0xa1, 0xc6, 0xa0, 0xb6, 0x01, 0x17, 0x84, 0x22, 0x04,
	0x0a, 0x0d, 0x01, 0x0a, 0x02, 0x3e, 0x98, 0xfd, 0xb5, 0xf6, 0x6c, 0x35,
	0xc9, 0x3d, 0x3e, 0x7b, 0x5e, 0x48, 0x53, 0xa1, 0xc6, 0xa0, 0xb6, 0x0a,
	0x08, 0x01, 0x0a, 0x12, 0x3e, 0x98, 0xfd, 0xb5, 0xf6, 0x6c, 0x35, 0xc9,
	0x3d, 0x3e, 0x7b, 0x5e, 0x47, 0x69, 0xe9, 0xc4, 0xa6, 0x5a, 0x01, 0x17,
	0x84, 0x22, 0x04, 0x0a, 0x0d, 0x01, 0x0a, 0x02, 0x3e, 0x98, 0xfd, 0xb5,
	0xf6, 0x6c, 0x35, 0xc9, 0x3d, 0x3e, 0x7b, 0x5e, 0x47, 0x69, 0xe9, 0xc4,
	0xa6, 0x5a, 0x0a, 0x08, 0x01, 0x0a, 0x12, 0x3e, 0x98, 0xfd, 0xb5, 0xf6,
	0x6c, 0x35, 0xc9, 0x3d, 0x3e, 0x7b, 0x5e, 0x46, 0x2c, 0x90, 0xb8, 0xd1,
	0xff, 0x01, 0x17, 0x84, 0x22, 0x04, 0x0a, 0x0d, 0x01, 0x0a, 0x02, 0x3e,
	0x98, 0xfd, 0xb5, 0xf6, 0x6c, 0x35, 0xc9, 0x3d, 0x3e, 0x7b, 0x5e, 0x46,
	0x2c, 0x90, 0xb8, 0xd1, 0xff
};


// parameters for the DMA resource
static const uint32 kDMAResourceBufferCount			= 16;
static const uint32 kDMAResourceBounceBufferCount	= 16;

static const char* const kDriverModuleName
	= "drivers/disk/virtual/ram_disk/driver_v1";
static const char* const kControlDeviceModuleName
	= "drivers/disk/virtual/ram_disk/control/device_v1";
static const char* const kRawDeviceModuleName
	= "drivers/disk/virtual/ram_disk/raw/device_v1";

static const char* const kControlDeviceName = RAM_DISK_CONTROL_DEVICE_NAME;
static const char* const kRawDeviceBaseName = RAM_DISK_RAW_DEVICE_BASE_NAME;

static const char* const kFilePathItem = "ram_disk/file_path";
static const char* const kDeviceSizeItem = "ram_disk/device_size";
static const char* const kDeviceIDItem = "ram_disk/id";


struct RawDevice;
typedef DoublyLinkedList<RawDevice> RawDeviceList;

struct device_manager_info* sDeviceManager;

static RawDeviceList sDeviceList;
static mutex sDeviceListLock = MUTEX_INITIALIZER("ram disk device list");
static uint64 sUsedRawDeviceIDs = 0;


static int32	allocate_raw_device_id();
static void		free_raw_device_id(int32 id);


struct Device {
	Device(device_node* node)
		:
		fNode(node)
	{
		mutex_init(&fLock, "ram disk device");
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

	status_t Register(const char* filePath, uint64 deviceSize, int32& _id)
	{
		int32 id = allocate_raw_device_id();
		if (id < 0)
			return B_BUSY;

		device_attr attrs[] = {
			{B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
				{string: "RAM Disk Raw Device"}},
			{kDeviceSizeItem, B_UINT64_TYPE, {ui64: deviceSize}},
			{kDeviceIDItem, B_UINT32_TYPE, {ui32: (uint32)id}},
			{kFilePathItem, B_STRING_TYPE, {string: filePath}},
			{NULL}
		};

		// If filePath is NULL, remove the attribute.
		if (filePath == NULL) {
			size_t count = sizeof(attrs) / sizeof(attrs[0]);
			memset(attrs + count - 2, 0, sizeof(attrs[0]));
		}

		status_t error = sDeviceManager->register_node(
			sDeviceManager->get_parent_node(Node()), kDriverModuleName, attrs,
			NULL, NULL);
		if (error != B_OK) {
			free_raw_device_id(id);
			return error;
		}

		_id = id;
		return B_OK;
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
		fID(-1),
		fUnregistered(false),
		fDeviceSize(0),
		fDeviceName(NULL),
		fFilePath(NULL),
		fCache(NULL),
		fDMAResource(NULL),
		fIOScheduler(NULL)
	{
	}

	virtual ~RawDevice()
	{
		if (fID >= 0) {
			MutexLocker locker(sDeviceListLock);
			sDeviceList.Remove(this);
		}

		free(fDeviceName);
		free(fFilePath);
	}

	int32 ID() const				{ return fID; }
	off_t DeviceSize() const		{ return fDeviceSize; }
	const char* DeviceName() const	{ return fDeviceName; }

	bool IsUnregistered() const		{ return fUnregistered; }

	void SetUnregistered(bool unregistered)
	{
		fUnregistered = unregistered;
	}

	status_t Init(int32 id, const char* filePath, uint64 deviceSize)
	{
		fID = id;
		fFilePath = filePath != NULL ? strdup(filePath) : NULL;
		if (filePath != NULL && fFilePath == NULL)
			return B_NO_MEMORY;

		fDeviceSize = (deviceSize + B_PAGE_SIZE - 1) / B_PAGE_SIZE
			* B_PAGE_SIZE;

		if (fDeviceSize < B_PAGE_SIZE
			|| (uint64)fDeviceSize / B_PAGE_SIZE
				> vm_page_num_pages() * 2 / 3) {
			return B_BAD_VALUE;
		}

		// construct our device path
		KPath path(kRawDeviceBaseName);
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "%" B_PRId32 "/raw", fID);

		status_t error = path.Append(buffer);
		if (error != B_OK)
			return error;

		fDeviceName = path.DetachBuffer();

		// insert into device list
		RawDevice* nextDevice = NULL;
		MutexLocker locker(sDeviceListLock);
		for (RawDeviceList::Iterator it = sDeviceList.GetIterator();
				(nextDevice = it.Next()) != NULL;) {
			if (nextDevice->ID() > fID)
				break;
		}

		sDeviceList.InsertBefore(nextDevice, this);

		return B_OK;
	}

	status_t Prepare()
	{
		status_t error = VMCacheFactory::CreateAnonymousCache(fCache, false, 0,
			0, false, VM_PRIORITY_SYSTEM);
		if (error != B_OK) {
			Unprepare();
			return error;
		}

		fCache->temporary = 1;
		fCache->virtual_end = fDeviceSize;

		error = fCache->Commit(fDeviceSize, VM_PRIORITY_SYSTEM);
		if (error != B_OK) {
			Unprepare();
			return error;
		}

		if (fFilePath != NULL) {
			error = _LoadFile();
			if (error != B_OK) {
				Unprepare();
				return error;
			}
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

		error = fIOScheduler->Init("ram disk device scheduler");
		if (error != B_OK) {
			Unprepare();
			return error;
		}

		fIOScheduler->SetCallback(&_DoIOEntry, this);

		return B_OK;
	}

	void Unprepare()
	{
		delete fIOScheduler;
		fIOScheduler = NULL;

		delete fDMAResource;
		fDMAResource = NULL;

		if (fCache != NULL) {
			fCache->Lock();
			fCache->ReleaseRefAndUnlock();
			fCache = NULL;
		}
	}

	void GetInfo(ram_disk_ioctl_info& _info) const
	{
		_info.id = fID;
		_info.size = fDeviceSize;
		memset(&_info.path, 0, sizeof(_info.path));
		if (fFilePath != NULL)
			strlcpy(_info.path, fFilePath, sizeof(_info.path));
	}

	status_t Flush()
	{
		static const size_t kPageCountPerIteration = 1024;
		static const size_t kMaxGapSize = 15;

		int fd = open(fFilePath, O_WRONLY);
		if (fd < 0)
			return errno;
		FileDescriptorCloser fdCloser(fd);

		vm_page** pages = new(std::nothrow) vm_page*[kPageCountPerIteration];
		ArrayDeleter<vm_page*> pagesDeleter(pages);

		uint8* buffer = (uint8*)malloc(kPageCountPerIteration * B_PAGE_SIZE);
		MemoryDeleter bufferDeleter(buffer);

		if (pages == NULL || buffer == NULL)
			return B_NO_MEMORY;

		// Iterate through all pages of the cache and write those back that have
		// been modified.
		AutoLocker<VMCache> locker(fCache);

		status_t error = B_OK;

		for (off_t offset = 0; offset < fDeviceSize;) {
			// find the first modified page at or after the current offset
			VMCachePagesTree::Iterator it
				= fCache->pages.GetIterator(offset / B_PAGE_SIZE, true, true);
			vm_page* firstModified;
			while ((firstModified = it.Next()) != NULL
				&& !firstModified->modified) {
			}

			if (firstModified == NULL)
				break;

			if (firstModified->busy) {
				fCache->WaitForPageEvents(firstModified, PAGE_EVENT_NOT_BUSY,
					true);
				continue;
			}

			pages[0] = firstModified;
			page_num_t firstPageIndex = firstModified->cache_offset;
			offset = firstPageIndex * B_PAGE_SIZE;

			// Collect more pages until the gap between two modified pages gets
			// too large or we hit the end of our array.
			size_t previousModifiedIndex = 0;
			size_t previousIndex = 0;
			while (vm_page* page = it.Next()) {
				page_num_t index = page->cache_offset - firstPageIndex;
				if (page->busy
					|| index >= kPageCountPerIteration
					|| index - previousModifiedIndex > kMaxGapSize) {
					break;
				}

				pages[index] = page;

				// clear page array gap since the previous page
				if (previousIndex + 1 < index) {
					memset(pages + previousIndex + 1, 0,
						(index - previousIndex - 1) * sizeof(vm_page*));
				}

				previousIndex = index;
				if (page->modified)
					previousModifiedIndex = index;
			}

			// mark all pages we want to write busy
			size_t pagesToWrite = previousModifiedIndex + 1;
			for (size_t i = 0; i < pagesToWrite; i++) {
				if (vm_page* page = pages[i]) {
					DEBUG_PAGE_ACCESS_START(page);
					page->busy = true;
				}
			}

			locker.Unlock();

			// copy the pages to our buffer
			for (size_t i = 0; i < pagesToWrite; i++) {
				if (vm_page* page = pages[i]) {
					error = vm_memcpy_from_physical(buffer + i * B_PAGE_SIZE,
						page->physical_page_number * B_PAGE_SIZE, B_PAGE_SIZE,
						false);
					if (error != B_OK) {
						dprintf("ramdisk: error copying page %" B_PRIu64
							" data: %s\n", (uint64)page->physical_page_number,
							strerror(error));
						break;
					}
				} else
					memset(buffer + i * B_PAGE_SIZE, 0, B_PAGE_SIZE);
			}

			// write the buffer
			if (error == B_OK) {
				ssize_t bytesWritten = pwrite(fd, buffer,
					pagesToWrite * B_PAGE_SIZE, offset);
				if (bytesWritten < 0) {
					dprintf("ramdisk: error writing pages to file: %s\n",
						strerror(bytesWritten));
					error = bytesWritten;
				}
				else if ((size_t)bytesWritten != pagesToWrite * B_PAGE_SIZE) {
					dprintf("ramdisk: error writing pages to file: short "
						"write (%zd/%zu)\n", bytesWritten,
						pagesToWrite * B_PAGE_SIZE);
					error = B_ERROR;
				}
			}

			// mark the pages unbusy, on success also unmodified
			locker.Lock();

			for (size_t i = 0; i < pagesToWrite; i++) {
				if (vm_page* page = pages[i]) {
					if (error == B_OK)
						page->modified = false;
					fCache->MarkPageUnbusy(page);
					DEBUG_PAGE_ACCESS_END(page);
				}
			}

			if (error != B_OK)
				break;

			offset += pagesToWrite * B_PAGE_SIZE;
		}

		return error;
	}

	status_t Trim(fs_trim_data* trimData)
	{
		TRACE("trim_device()\n");

		trimData->trimmed_size = 0;

		const off_t deviceSize = fDeviceSize; // in bytes
		if (deviceSize < 0)
			return B_BAD_VALUE;

		STATIC_ASSERT(sizeof(deviceSize) <= sizeof(uint64));
		ASSERT(deviceSize >= 0);

		// Do not trim past device end
		for (uint32 i = 0; i < trimData->range_count; i++) {
			uint64 offset = trimData->ranges[i].offset;
			uint64& size = trimData->ranges[i].size;

			if (offset >= (uint64)deviceSize)
				return B_BAD_VALUE;
			size = min_c(size, (uint64)deviceSize - offset);
		}

		status_t result = B_OK;
		uint64 trimmedSize = 0;
		for (uint32 i = 0; i < trimData->range_count; i++) {
			uint64 offset = trimData->ranges[i].offset;
			uint64 length = trimData->ranges[i].size;

			// Round up offset and length to multiple of the page size
			// The offset is rounded up, so some space may be left
			// (not trimmed) at the start of the range.
			offset = (offset + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
			// Adjust the length for the possibly skipped range
			length -= offset - trimData->ranges[i].offset;
			// The length is rounded down, so some space at the end may also
			// be left (not trimmed).
			length &= ~(B_PAGE_SIZE - 1);

			if (length == 0)
				continue;

			TRACE("ramdisk: trim %" B_PRIu64 " bytes from %" B_PRIu64 "\n",
				length, offset);

			ASSERT(offset % B_PAGE_SIZE == 0);
			ASSERT(length % B_PAGE_SIZE == 0);

			vm_page** pages = new(std::nothrow) vm_page*[length / B_PAGE_SIZE];
			if (pages == NULL) {
				result = B_NO_MEMORY;
				break;
			}
			ArrayDeleter<vm_page*> pagesDeleter(pages);

			_GetPages((off_t)offset, (off_t)length, false, pages);

			AutoLocker<VMCache> locker(fCache);
			uint64 j;
			for (j = 0; j < length / B_PAGE_SIZE; j++) {
				// If we run out of pages (some may already be trimmed), stop.
				if (pages[j] == NULL)
					break;

				TRACE("free range %" B_PRIu32 ", page %" B_PRIu64 ", offset %"
					B_PRIu64 "\n", i, j, offset);
				if (pages[j]->Cache())
					fCache->RemovePage(pages[j]);
				vm_page_free(NULL, pages[j]);
				trimmedSize += B_PAGE_SIZE;
			}
		}

		trimData->trimmed_size = trimmedSize;

		return result;
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

		vm_page** pages = new(std::nothrow) vm_page*[length / B_PAGE_SIZE];
		if (pages == NULL)
			return B_NO_MEMORY;
		ArrayDeleter<vm_page*> pagesDeleter(pages);

		_GetPages(offset, length, isWrite, pages);

		status_t error = B_OK;
		size_t index = 0;

		while (length > 0) {
			vm_page* page = pages[index];

			if (isWrite)
				page->modified = true;

			error = _CopyData(page, vecs, vecOffset, isWrite);
			if (error != B_OK)
				break;

			offset += B_PAGE_SIZE;
			length -= B_PAGE_SIZE;
			index++;
		}

		_PutPages(operation->Offset(), operation->Length(), pages,
			error == B_OK);

		if (error != B_OK) {
			fIOScheduler->OperationCompleted(operation, error, 0);
			return error;
		}

		fIOScheduler->OperationCompleted(operation, B_OK, operation->Length());
		return B_OK;
	}

	void _GetPages(off_t offset, off_t length, bool isWrite, vm_page** pages)
	{
		// TODO: This method is duplicated in ramfs' DataContainer. Perhaps it
		// should be put into a common location?

		// get the pages, we already have
		AutoLocker<VMCache> locker(fCache);

		size_t pageCount = length / B_PAGE_SIZE;
		size_t index = 0;
		size_t missingPages = 0;

		while (length > 0) {
			vm_page* page = fCache->LookupPage(offset);
			if (page != NULL) {
				if (page->busy) {
					fCache->WaitForPageEvents(page, PAGE_EVENT_NOT_BUSY, true);
					continue;
				}

				DEBUG_PAGE_ACCESS_START(page);
				page->busy = true;
			} else
				missingPages++;

			pages[index++] = page;
			offset += B_PAGE_SIZE;
			length -= B_PAGE_SIZE;
		}

		locker.Unlock();

		// For a write we need to reserve the missing pages.
		if (isWrite && missingPages > 0) {
			vm_page_reservation reservation;
			vm_page_reserve_pages(&reservation, missingPages,
				VM_PRIORITY_SYSTEM);

			for (size_t i = 0; i < pageCount; i++) {
				if (pages[i] != NULL)
					continue;

				pages[i] = vm_page_allocate_page(&reservation,
					PAGE_STATE_WIRED | VM_PAGE_ALLOC_BUSY);

				if (--missingPages == 0)
					break;
			}

			vm_page_unreserve_pages(&reservation);
		}
	}

	void _PutPages(off_t offset, off_t length, vm_page** pages, bool success)
	{
		// TODO: This method is duplicated in ramfs' DataContainer. Perhaps it
		// should be put into a common location?

		AutoLocker<VMCache> locker(fCache);

		// Mark all pages unbusy. On error free the newly allocated pages.
		size_t index = 0;

		while (length > 0) {
			vm_page* page = pages[index++];
			if (page != NULL) {
				if (page->CacheRef() == NULL) {
					if (success) {
						fCache->InsertPage(page, offset);
						fCache->MarkPageUnbusy(page);
						DEBUG_PAGE_ACCESS_END(page);
					} else
						vm_page_free(NULL, page);
				} else {
					fCache->MarkPageUnbusy(page);
					DEBUG_PAGE_ACCESS_END(page);
				}
			}

			offset += B_PAGE_SIZE;
			length -= B_PAGE_SIZE;
		}
	}

	status_t _CopyData(vm_page* page, const generic_io_vec*& vecs,
		generic_size_t& vecOffset, bool toPage)
	{
		// map page to virtual memory
		Thread* thread = thread_get_current_thread();
		uint8* pageData = NULL;
		void* handle;
		if (page != NULL) {
			thread_pin_to_current_cpu(thread);
			addr_t virtualAddress;
			status_t error = vm_get_physical_page_current_cpu(
				page->physical_page_number * B_PAGE_SIZE, &virtualAddress,
				&handle);
			if (error != B_OK) {
				thread_unpin_from_current_cpu(thread);
				return error;
			}

			pageData = (uint8*)virtualAddress;
		}

		status_t error = B_OK;
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

			error = toPage
				? vm_memcpy_from_physical(pageData, vecAddress, toCopy, false)
				: (page != NULL
					? vm_memcpy_to_physical(vecAddress, pageData, toCopy, false)
					: vm_memset_physical(vecAddress, 0, toCopy));
			if (error != B_OK)
				break;

			pageData += toCopy;
			length -= toCopy;
			vecOffset += toCopy;
		}

		if (page != NULL) {
			vm_put_physical_page_current_cpu((addr_t)pageData, handle);
			thread_unpin_from_current_cpu(thread);
		}

		return error;
	}

	status_t _LoadFile()
	{
		static const size_t kPageCountPerIteration = 1024;

		int fd = open(fFilePath, O_RDONLY);
		if (fd < 0)
			return errno;
		FileDescriptorCloser fdCloser(fd);

		vm_page** pages = new(std::nothrow) vm_page*[kPageCountPerIteration];
		ArrayDeleter<vm_page*> pagesDeleter(pages);

		uint8* buffer = (uint8*)malloc(kPageCountPerIteration * B_PAGE_SIZE);
		MemoryDeleter bufferDeleter(buffer);
			// TODO: Ideally we wouldn't use a buffer to read the file content,
			// but read into the pages we allocated directly. Unfortunately
			// there's no API to do that yet.

		if (pages == NULL || buffer == NULL)
			return B_NO_MEMORY;

		status_t error = B_OK;

		page_num_t allocatedPages = 0;
		off_t offset = 0;
		off_t sizeRemaining = fDeviceSize;
		while (sizeRemaining > 0) {
			// Note: fDeviceSize is B_PAGE_SIZE aligned.
			size_t pagesToRead = std::min(kPageCountPerIteration,
				size_t(sizeRemaining / B_PAGE_SIZE));

			// allocate the missing pages
			if (allocatedPages < pagesToRead) {
				vm_page_reservation reservation;
				vm_page_reserve_pages(&reservation,
					pagesToRead - allocatedPages, VM_PRIORITY_SYSTEM);

				while (allocatedPages < pagesToRead) {
					pages[allocatedPages++]
						= vm_page_allocate_page(&reservation, PAGE_STATE_WIRED);
				}

				vm_page_unreserve_pages(&reservation);
			}

			// read from the file
			size_t bytesToRead = pagesToRead * B_PAGE_SIZE;
			ssize_t bytesRead = pread(fd, buffer, bytesToRead, offset);
			if (bytesRead < 0) {
				error = bytesRead;
				break;
			}
			size_t pagesRead = (bytesRead + B_PAGE_SIZE - 1) / B_PAGE_SIZE;
			if (pagesRead < pagesToRead) {
				error = B_ERROR;
				break;
			}

			// clear the last read page, if partial
			if ((size_t)bytesRead < pagesRead * B_PAGE_SIZE) {
				memset(buffer + bytesRead, 0,
					pagesRead * B_PAGE_SIZE - bytesRead);
			}

			// copy data to allocated pages
			for (size_t i = 0; i < pagesRead; i++) {
				vm_page* page = pages[i];
				error = vm_memcpy_to_physical(
					page->physical_page_number * B_PAGE_SIZE,
					buffer + i * B_PAGE_SIZE, B_PAGE_SIZE, false);
				if (error != B_OK)
					break;
			}

			if (error != B_OK)
				break;

			// Add pages to cache. Ignore clear pages, though. Move those to the
			// beginning of the array, so we can reuse them in the next
			// iteration.
			AutoLocker<VMCache> locker(fCache);

			size_t clearPages = 0;
			for (size_t i = 0; i < pagesRead; i++) {
				uint64* pageData = (uint64*)(buffer + i * B_PAGE_SIZE);
				bool isClear = true;
				for (size_t k = 0; isClear && k < B_PAGE_SIZE / 8; k++)
					isClear = pageData[k] == 0;

				if (isClear) {
					pages[clearPages++] = pages[i];
				} else {
					fCache->InsertPage(pages[i], offset + i * B_PAGE_SIZE);
					DEBUG_PAGE_ACCESS_END(pages[i]);
				}
			}

			locker.Unlock();

			// Move any left-over allocated pages to the end of the empty pages
			// and compute the new allocated pages count.
			if (pagesRead < allocatedPages) {
				size_t count = allocatedPages - pagesRead;
				memcpy(pages + clearPages, pages + pagesRead,
					count * sizeof(vm_page*));
				clearPages += count;
			}
			allocatedPages = clearPages;

			offset += pagesRead * B_PAGE_SIZE;
			sizeRemaining -= pagesRead * B_PAGE_SIZE;
		}

		// free left-over allocated pages
		for (size_t i = 0; i < allocatedPages; i++)
			vm_page_free(NULL, pages[i]);

		return error;
	}

private:
	int32			fID;
	bool			fUnregistered;
	off_t			fDeviceSize;
	char*			fDeviceName;
	char*			fFilePath;
	VMCache*		fCache;
	DMAResource*	fDMAResource;
	IOScheduler*	fIOScheduler;
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


static int32
allocate_raw_device_id()
{
	MutexLocker deviceListLocker(sDeviceListLock);
	for (size_t i = 0; i < sizeof(sUsedRawDeviceIDs) * 8; i++) {
		if ((sUsedRawDeviceIDs & ((uint64)1 << i)) == 0) {
			sUsedRawDeviceIDs |= (uint64)1 << i;
			return (int32)i;
		}
	}

	return -1;
}


static void
free_raw_device_id(int32 id)
{
	MutexLocker deviceListLocker(sDeviceListLock);
	sUsedRawDeviceIDs &= ~((uint64)1 << id);
}


static RawDevice*
find_raw_device(int32 id)
{
	for (RawDeviceList::Iterator it = sDeviceList.GetIterator();
			RawDevice* device = it.Next();) {
		if (device->ID() == id)
			return device;
	}

	return NULL;
}


static status_t
ioctl_register(ControlDevice* controlDevice, ram_disk_ioctl_register* request)
{
	KPath path;
	uint64 deviceSize = 0;

	if (request->path[0] != '\0') {
		// check if the path is null-terminated
		if (strnlen(request->path, sizeof(request->path))
				== sizeof(request->path)) {
			return B_BAD_VALUE;
		}

		// get a normalized file path
		status_t error = path.SetTo(request->path, true);
		if (error != B_OK) {
			dprintf("ramdisk: register: Invalid path \"%s\": %s\n",
				request->path, strerror(error));
			return B_BAD_VALUE;
		}

		struct stat st;
		if (lstat(path.Path(), &st) != 0) {
			dprintf("ramdisk: register: Failed to stat \"%s\": %s\n",
				path.Path(), strerror(errno));
			return errno;
		}

		if (!S_ISREG(st.st_mode)) {
			dprintf("ramdisk: register: \"%s\" is not a file!\n", path.Path());
			return B_BAD_VALUE;
		}

		deviceSize = st.st_size;
	} else {
		deviceSize = request->size;
	}

	return controlDevice->Register(path.Length() > 0 ? path.Path() : NULL,
		deviceSize, request->id);
}


static status_t
ioctl_unregister(ControlDevice* controlDevice,
	ram_disk_ioctl_unregister* request)
{
	// find the device in the list and unregister it
	MutexLocker locker(sDeviceListLock);
	RawDevice* device = find_raw_device(request->id);
	if (device == NULL)
		return B_ENTRY_NOT_FOUND;

	// mark unregistered before we unlock
	if (device->IsUnregistered())
		return B_BUSY;
	device->SetUnregistered(true);
	locker.Unlock();

	device_node* node = device->Node();
	status_t error = sDeviceManager->unpublish_device(node,
		device->DeviceName());
	if (error != B_OK) {
		dprintf("ramdisk: unregister: Failed to unpublish device \"%s\": %s\n",
			device->DeviceName(), strerror(error));
		return error;
	}

	error = sDeviceManager->unregister_node(node);
	// Note: B_BUSY is OK. The node will removed as soon as possible.
	if (error != B_OK && error != B_BUSY) {
		dprintf("ramdisk: unregister: Failed to unregister node for device %"
			B_PRId32 ": %s\n", request->id, strerror(error));
		return error;
	}

	return B_OK;
}


static status_t
ioctl_info(RawDevice* device, ram_disk_ioctl_info* request)
{
	device->GetInfo(*request);
	return B_OK;
}


template<typename DeviceType, typename Request>
static status_t
handle_ioctl(DeviceType* device,
	status_t (*handler)(DeviceType*, Request*), void* buffer)
{
	// copy request to the kernel heap
	if (buffer == NULL || !IS_USER_ADDRESS(buffer))
		return B_BAD_ADDRESS;

	Request* request = new(std::nothrow) Request;
	if (request == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<Request> requestDeleter(request);

	if (user_memcpy(request, buffer, sizeof(Request)) != B_OK)
		return B_BAD_ADDRESS;

	// handle the ioctl
	status_t error = handler(device, request);
	if (error != B_OK)
		return error;

	// copy the request back to userland
	if (user_memcpy(buffer, request, sizeof(Request)) != B_OK)
		return B_BAD_ADDRESS;

	return B_OK;
}


//	#pragma mark - driver


static float
ram_disk_driver_supports_device(device_node* parent)
{
	const char* bus = NULL;
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false)
			== B_OK
		&& strcmp(bus, "generic") == 0) {
		return 0.8;
	}

	return -1;
}


static status_t
ram_disk_driver_register_device(device_node* parent)
{
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{string: "RAM Disk Control Device"}},
		{NULL}
	};

	return sDeviceManager->register_node(parent, kDriverModuleName, attrs, NULL,
		NULL);
}


static status_t
ram_disk_driver_init_driver(device_node* node, void** _driverCookie)
{
	uint64 deviceSize;
	if (sDeviceManager->get_attr_uint64(node, kDeviceSizeItem, &deviceSize,
			false) == B_OK) {
		int32 id = -1;
		sDeviceManager->get_attr_uint32(node, kDeviceIDItem, (uint32*)&id,
			false);
		if (id < 0)
			return B_ERROR;

		const char* filePath = NULL;
		sDeviceManager->get_attr_string(node, kFilePathItem, &filePath, false);

		RawDevice* device = new(std::nothrow) RawDevice(node);
		if (device == NULL)
			return B_NO_MEMORY;

		status_t error = device->Init(id, filePath, deviceSize);
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
ram_disk_driver_uninit_driver(void* driverCookie)
{
	Device* device = (Device*)driverCookie;
	if (RawDevice* rawDevice = dynamic_cast<RawDevice*>(device))
		free_raw_device_id(rawDevice->ID());
	delete device;
}


static status_t
ram_disk_driver_register_child_devices(void* driverCookie)
{
	Device* device = (Device*)driverCookie;
	return device->PublishDevice();
}


//	#pragma mark - control device


static status_t
ram_disk_control_device_init_device(void* driverCookie, void** _deviceCookie)
{
	*_deviceCookie = driverCookie;
	return B_OK;
}


static void
ram_disk_control_device_uninit_device(void* deviceCookie)
{
}


static status_t
ram_disk_control_device_open(void* deviceCookie, const char* path, int openMode,
	void** _cookie)
{
	*_cookie = deviceCookie;
	return B_OK;
}


static status_t
ram_disk_control_device_close(void* cookie)
{
	return B_OK;
}


static status_t
ram_disk_control_device_free(void* cookie)
{
	return B_OK;
}


static status_t
ram_disk_control_device_read(void* cookie, off_t position, void* buffer,
	size_t* _length)
{
	return B_BAD_VALUE;
}


static status_t
ram_disk_control_device_write(void* cookie, off_t position, const void* data,
	size_t* _length)
{
	return B_BAD_VALUE;
}


static status_t
ram_disk_control_device_control(void* cookie, uint32 op, void* buffer,
	size_t length)
{
	ControlDevice* device = (ControlDevice*)cookie;

	switch (op) {
		case RAM_DISK_IOCTL_REGISTER:
			return handle_ioctl(device, &ioctl_register, buffer);

		case RAM_DISK_IOCTL_UNREGISTER:
			return handle_ioctl(device, &ioctl_unregister, buffer);
	}

	return B_BAD_VALUE;
}


//	#pragma mark - raw device


static status_t
ram_disk_raw_device_init_device(void* driverCookie, void** _deviceCookie)
{
	RawDevice* device = static_cast<RawDevice*>((Device*)driverCookie);

	status_t error = device->Prepare();
	if (error != B_OK)
		return error;

	*_deviceCookie = device;
	return B_OK;
}


static void
ram_disk_raw_device_uninit_device(void* deviceCookie)
{
	RawDevice* device = (RawDevice*)deviceCookie;
	device->Unprepare();
}


static status_t
ram_disk_raw_device_open(void* deviceCookie, const char* path, int openMode,
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
ram_disk_raw_device_close(void* cookie)
{
	return B_OK;
}


static status_t
ram_disk_raw_device_free(void* _cookie)
{
	RawDeviceCookie* cookie = (RawDeviceCookie*)_cookie;
	delete cookie;
	return B_OK;
}


static status_t
ram_disk_raw_device_read(void* _cookie, off_t pos, void* buffer,
	size_t* _length)
{
	RawDeviceCookie* cookie = (RawDeviceCookie*)_cookie;
	RawDevice* device = cookie->Device();

	size_t length = *_length;

	if (pos >= device->DeviceSize())
		return B_BAD_VALUE;
	if (pos + (off_t)length > device->DeviceSize())
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
ram_disk_raw_device_write(void* _cookie, off_t pos, const void* buffer,
	size_t* _length)
{
	RawDeviceCookie* cookie = (RawDeviceCookie*)_cookie;
	RawDevice* device = cookie->Device();

	size_t length = *_length;

	if (pos >= device->DeviceSize())
		return B_BAD_VALUE;
	if (pos + (off_t)length > device->DeviceSize())
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
ram_disk_raw_device_io(void* _cookie, io_request* request)
{
	RawDeviceCookie* cookie = (RawDeviceCookie*)_cookie;
	RawDevice* device = cookie->Device();

	return device->DoIO(request);
}


static status_t
ram_disk_raw_device_control(void* _cookie, uint32 op, void* buffer,
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

		case B_GET_ICON_NAME:
			return user_strlcpy((char*)buffer, "devices/drive-ramdisk",
				B_FILE_NAME_LENGTH);

		case B_GET_VECTOR_ICON:
		{
			device_icon iconData;
			if (length != sizeof(device_icon))
				return B_BAD_VALUE;
			if (user_memcpy(&iconData, buffer, sizeof(device_icon)) != B_OK)
				return B_BAD_ADDRESS;

			if (iconData.icon_size >= (int32)sizeof(kRamdiskIcon)) {
				if (user_memcpy(iconData.icon_data, kRamdiskIcon,
						sizeof(kRamdiskIcon)) != B_OK)
					return B_BAD_ADDRESS;
			}

			iconData.icon_size = sizeof(kRamdiskIcon);
			return user_memcpy(buffer, &iconData, sizeof(device_icon));
		}

		case B_SET_UNINTERRUPTABLE_IO:
		case B_SET_INTERRUPTABLE_IO:
		case B_FLUSH_DRIVE_CACHE:
			return B_OK;

		case RAM_DISK_IOCTL_FLUSH:
		{
			status_t error = device->Flush();
			if (error != B_OK) {
				dprintf("ramdisk: flush: Failed to flush device: %s\n",
					strerror(error));
				return error;
			}

			return B_OK;
		}

		case B_TRIM_DEVICE:
		{
			// We know the buffer is kernel-side because it has been
			// preprocessed in devfs
			ASSERT(IS_KERNEL_ADDRESS(buffer));
			return device->Trim((fs_trim_data*)buffer);
		}

		case RAM_DISK_IOCTL_INFO:
			return handle_ioctl(device, &ioctl_info, buffer);
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

	ram_disk_driver_supports_device,
	ram_disk_driver_register_device,
	ram_disk_driver_init_driver,
	ram_disk_driver_uninit_driver,
	ram_disk_driver_register_child_devices
};

static const struct device_module_info sChecksumControlDeviceModule = {
	{
		kControlDeviceModuleName,
		0,
		NULL
	},

	ram_disk_control_device_init_device,
	ram_disk_control_device_uninit_device,
	NULL,

	ram_disk_control_device_open,
	ram_disk_control_device_close,
	ram_disk_control_device_free,

	ram_disk_control_device_read,
	ram_disk_control_device_write,
	NULL,	// io

	ram_disk_control_device_control,

	NULL,	// select
	NULL	// deselect
};

static const struct device_module_info sChecksumRawDeviceModule = {
	{
		kRawDeviceModuleName,
		0,
		NULL
	},

	ram_disk_raw_device_init_device,
	ram_disk_raw_device_uninit_device,
	NULL,

	ram_disk_raw_device_open,
	ram_disk_raw_device_close,
	ram_disk_raw_device_free,

	ram_disk_raw_device_read,
	ram_disk_raw_device_write,
	ram_disk_raw_device_io,

	ram_disk_raw_device_control,

	NULL,	// select
	NULL	// deselect
};

const module_info* modules[] = {
	(module_info*)&sChecksumDeviceDriverModule,
	(module_info*)&sChecksumControlDeviceModule,
	(module_info*)&sChecksumRawDeviceModule,
	NULL
};
