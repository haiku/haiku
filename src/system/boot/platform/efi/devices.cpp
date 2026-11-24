/*
 * Copyright 2016-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/partitions.h>
#include <boot/platform.h>
#include <boot/stage2.h>

#include <boot/net/Ethernet.h>
#include <boot/net/NetStack.h>
#include <boot/net/RemoteDisk.h>

#include "Header.h"

#include "efi_platform.h"
#include <efi/protocol/block-io.h>

#include "gpt.h"
#include "gpt_known_guids.h"


//#define TRACE_DEVICES
#ifdef TRACE_DEVICES
#   define TRACE(x...) dprintf("efi/devices: " x)
#else
#   define TRACE(x...) ;
#endif


static efi_guid BlockIoGUID = EFI_BLOCK_IO_PROTOCOL_GUID;

static uint8* sBounceBuffer;
static uint32 sBounceBufferSize;


class EFIDevice : public Node
{
public:
	EFIDevice(efi_block_io_protocol *blockIo);
	virtual ~EFIDevice();

	virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer,
		size_t bufferSize);
	virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer,
		size_t bufferSize) { return B_UNSUPPORTED; }
	virtual off_t Size() const
	{
		return (fBlockIo->Media->LastBlock + 1) * BlockSize();
	}

	uint32 BlockSize() const { return fBlockIo->Media->BlockSize; }
private:
	efi_block_io_protocol*		fBlockIo;
};


EFIDevice::EFIDevice(efi_block_io_protocol *blockIo)
	:
	fBlockIo(blockIo)
{
}


EFIDevice::~EFIDevice()
{
}


ssize_t
EFIDevice::ReadAt(void *cookie, off_t pos, void *_buffer, size_t bufferSize)
{
	TRACE("%s called. pos: %" B_PRIdOFF ", %p, %" B_PRIuSIZE "\n", __func__,
		pos, _buffer, bufferSize);

	off_t offset = pos % BlockSize();
	uint64 lba = pos / BlockSize();

	if (offset == 0 && (bufferSize % BlockSize()) == 0
			&& (fBlockIo->Media->IoAlign <= 1
				|| ((addr_t)_buffer % fBlockIo->Media->IoAlign) == 0)) {
		// No bounce buffer needed.
		efi_status status = fBlockIo->ReadBlocks(fBlockIo,
			fBlockIo->Media->MediaId, lba, bufferSize, _buffer);
		if (status != EFI_SUCCESS) {
			dprintf("%s: BlockIo error reading (unbounced) from device!\n", __func__);
			return B_ERROR;
		}
		return bufferSize;
	}

	if (fBlockIo->Media->IoAlign > PAGESIZE) {
		panic("IoAlign > PAGESIZE unsupported!");
		return B_ERROR;
	}

	if (sBounceBuffer != NULL && sBounceBufferSize < BlockSize()) {
		platform_free_heap_region(sBounceBuffer, sBounceBufferSize);
		sBounceBuffer = NULL;
	}
	if (sBounceBuffer == NULL) {
		sBounceBufferSize = platform_allocate_heap_region(max_c(PAGESIZE, BlockSize()),
			(void**)&sBounceBuffer);
	}

	uint32 numBlocks = (offset + bufferSize + BlockSize() - 1) / BlockSize();
	uint8* buffer = (uint8*)_buffer;
	off_t remaining = bufferSize;
	const size_t blocksPerBuffer = sBounceBufferSize / BlockSize();

	while (remaining > 0) {
		size_t readBlocks = min_c(numBlocks, blocksPerBuffer);
		efi_status status = fBlockIo->ReadBlocks(fBlockIo, fBlockIo->Media->MediaId,
			lba, readBlocks * BlockSize(), sBounceBuffer);
		if (status != EFI_SUCCESS) {
			dprintf("%s: BlockIo error reading (bounced) from device!\n", __func__);
			return B_ERROR;
		}

		size_t toCopy = min_c(sBounceBufferSize - offset, remaining);
		memcpy(buffer, sBounceBuffer + offset, toCopy);

		offset = 0;
		buffer += toCopy;
		remaining -= toCopy;

		lba += readBlocks;
		numBlocks -= readBlocks;
	}

	return bufferSize;
}


static bool
device_contains_partition(EFIDevice *device, boot::Partition *partition)
{
	EFI::Header *header = (EFI::Header*)partition->content_cookie;
	if (header != NULL && header->InitCheck() == B_OK) {
		// check if device is GPT, and contains partition entry
		uint32 blockSize = device->BlockSize();
		gpt_table_header *deviceHeader =
			(gpt_table_header*)malloc(blockSize);
		ssize_t bytesRead = device->ReadAt(NULL, blockSize, deviceHeader,
			blockSize);
		if (bytesRead != (ssize_t)blockSize)
			return false;

		if (memcmp(deviceHeader, &header->TableHeader(),
				sizeof(gpt_table_header)) != 0)
			return false;

		// partition->cookie == int partition entry index
		uint32 index = (uint32)(addr_t)partition->cookie;
		uint32 size = sizeof(gpt_partition_entry) * (index + 1);
		gpt_partition_entry *entries = (gpt_partition_entry*)malloc(size);
		bytesRead = device->ReadAt(NULL,
			deviceHeader->entries_block * blockSize, entries, size);
		if (bytesRead != (ssize_t)size)
			return false;

		if (memcmp(&entries[index], &header->EntryAt(index),
				sizeof(gpt_partition_entry)) != 0)
			return false;

		for (size_t i = 0; i < sizeof(kTypeMap) / sizeof(struct type_map); ++i)
			if (strcmp(kTypeMap[i].type, BFS_NAME) == 0)
				if (kTypeMap[i].guid == header->EntryAt(index).partition_type)
					return true;

		// Our partition has an EFI header, but we couldn't find one, so bail
		return false;
	}

	if ((partition->offset + partition->size) <= device->Size())
			return true;

	return false;
}


status_t
platform_add_boot_device(struct stage2_args *args, NodeList *devicesList)
{
	TRACE("%s: called\n", __func__);

	efi_block_io_protocol *blockIo;
	size_t memSize = 0;

	// If the bootloader was started from the network (net_stack_init will find
	// the ip= parameter in the LoadOptions), add network booting as the first
	// entry if available
	status_t error = net_stack_init();
	if (error != B_OK) {
		TRACE("Can't init network...\n");
	} else {
		TRACE("Network is initialized! Search for remote disk...\n");
		RemoteDisk *remoteDisk = RemoteDisk::FindAnyRemoteDisk();
		if (remoteDisk != NULL)
			devicesList->Add(remoteDisk);
	}

	// Read to zero sized buffer to get memory needed for handles
	if (kBootServices->LocateHandle(ByProtocol, &BlockIoGUID, 0, &memSize, 0)
			!= EFI_BUFFER_TOO_SMALL)
		panic("Cannot read size of block device handles!");

	uint32 noOfHandles = memSize / sizeof(efi_handle);

	efi_handle handles[noOfHandles];
	if (kBootServices->LocateHandle(ByProtocol, &BlockIoGUID, 0, &memSize,
			handles) != EFI_SUCCESS)
		panic("Failed to locate block devices!");

	// All block devices has one for the disk and one per partition
	// There is a special case for a device with one fixed partition
	// But we probably do not care about booting on that kind of device
	// So find all disk block devices and let Haiku do partition scan
	for (uint32 n = 0; n < noOfHandles; n++) {
		if (kBootServices->HandleProtocol(handles[n], &BlockIoGUID,
				(void**)&blockIo) != EFI_SUCCESS)
			panic("Cannot get block device handle!");

		TRACE("%s: %p: present: %s, logical: %s, removeable: %s, "
			"blocksize: %" PRIu32 ", lastblock: %" PRIu64 "\n",
			__func__, blockIo,
			blockIo->Media->MediaPresent ? "true" : "false",
			blockIo->Media->LogicalPartition ? "true" : "false",
			blockIo->Media->RemovableMedia ? "true" : "false",
			blockIo->Media->BlockSize, blockIo->Media->LastBlock);

		if (!blockIo->Media->MediaPresent
			|| blockIo->Media->LogicalPartition
			|| blockIo->Media->BlockSize == 0)
			continue;

		// The qemu flash device with a 256K block sizes sometime show up
		// in edk2. If flash is unconfigured, bad things happen on arm.
		// edk2 bug: https://bugzilla.tianocore.org/show_bug.cgi?id=2856
		// We're not ready for flash devices in efi, so skip anything odd.
		if (blockIo->Media->BlockSize > 8192)
			continue;

		EFIDevice *device = new(std::nothrow)EFIDevice(blockIo);
		if (device == NULL)
			panic("Can't allocate memory for block devices!");
		devicesList->Insert(device);
	}

	return devicesList->Count() > 0 ? B_OK : B_ENTRY_NOT_FOUND;
}


status_t
platform_add_block_devices(struct stage2_args *args, NodeList *devicesList)
{
	TRACE("%s: called\n", __func__);

	//TODO: Currently we add all in platform_add_boot_device
	return devicesList->Count() > 0 ? B_OK : B_ENTRY_NOT_FOUND;
}


status_t
platform_get_boot_partitions(struct stage2_args *args, Node *bootDevice,
		NodeList *partitions, NodeList *bootPartitions)
{
	NodeIterator iterator = partitions->GetIterator();
	boot::Partition *partition = NULL;
	while ((partition = (boot::Partition*)iterator.Next()) != NULL) {
		if (device_contains_partition((EFIDevice*)bootDevice, partition)) {
			bootPartitions->Insert(partition);
		}
	}

	return bootPartitions->Count() > 0 ? B_OK : B_ENTRY_NOT_FOUND;
}


status_t
platform_register_boot_device(Node *device, disk_identifier* defaultDiskID)
{
	TRACE("%s: called\n", __func__);

	// TODO is there no better way to identify network boot?
	char buffer[11];
	device->GetName(buffer, sizeof(buffer));
	TRACE("Booting from %s\n", buffer);

	if (strcmp(buffer, "RemoteDisk") == 0) {
		TRACE("Booting from network\n");
		RemoteDisk* remoteDisk = static_cast<RemoteDisk*>(device);
		// Tell the kernel we're booting from network and forward the network
		// configuration
		auto ethernetInterface = NetStack::Default()->GetEthernetInterface();
		if (gBootParams.SetInt32(BOOT_METHOD, BOOT_METHOD_NET)
			|| gBootParams.AddInt64("client MAC",
				ethernetInterface->MACAddress().ToUInt64()) != B_OK
			|| gBootParams.AddInt32("client IP",
				ethernetInterface->IPAddress()) != B_OK
			|| gBootParams.AddInt32("server IP", remoteDisk->ServerIPAddress())
				!= B_OK
			|| gBootParams.AddInt32("server port", remoteDisk->ServerPort())
				!= B_OK) {
			return B_NO_MEMORY;
		}
	} else {
		// ...HARD_DISK, as we pick partition and have checksum (no need to use _CD)
		gBootParams.SetInt32(BOOT_METHOD, BOOT_METHOD_HARD_DISK);
	}

	gBootParams.SetData(BOOT_VOLUME_DISK_IDENTIFIER, B_RAW_TYPE,
		defaultDiskID, sizeof(disk_identifier));

	return B_OK;
}


void
platform_cleanup_devices()
{
}
