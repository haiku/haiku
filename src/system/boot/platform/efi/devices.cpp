/*
 * Copyright 2016-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/partitions.h>
#include <boot/platform.h>
#include <boot/stage2.h>

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


class EfiDevice : public Node
{
	public:
		EfiDevice(efi_block_io_protocol *blockIo);
		virtual ~EfiDevice();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer,
			size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer,
			size_t bufferSize) { return B_UNSUPPORTED; }
		virtual off_t Size() const {
			return (fBlockIo->Media->LastBlock + 1) * BlockSize(); }

		uint32 BlockSize() const { return fBlockIo->Media->BlockSize; }
	private:
		efi_block_io_protocol*		fBlockIo;
};


EfiDevice::EfiDevice(efi_block_io_protocol *blockIo)
	:
	fBlockIo(blockIo)
{
}


EfiDevice::~EfiDevice()
{
}


ssize_t
EfiDevice::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	TRACE("%s called. pos: %" B_PRIdOFF ", %p, %" B_PRIuSIZE "\n", __func__,
		pos, buffer, bufferSize);

	off_t offset = pos % BlockSize();
	pos /= BlockSize();

	uint32 numBlocks = (offset + bufferSize + BlockSize() - 1) / BlockSize();

	// TODO: We really should implement memalign and align all requests to
	// fBlockIo->Media->IoAlign. This static alignment is large enough though
	// to catch most required alignments.
	char readBuffer[numBlocks * BlockSize()]
		__attribute__((aligned(2048)));

	if (fBlockIo->ReadBlocks(fBlockIo, fBlockIo->Media->MediaId,
		pos, sizeof(readBuffer), readBuffer) != EFI_SUCCESS) {
		dprintf("%s: blockIo error reading from device!\n", __func__);
		return B_ERROR;
	}

	memcpy(buffer, readBuffer + offset, bufferSize);

	return bufferSize;
}


static off_t
get_next_check_sum_offset(int32 index, off_t maxSize)
{
	TRACE("%s: called\n", __func__);

	if (index < 2)
		return index * 512;

	if (index < 4)
		return (maxSize >> 10) + index * 2048;

	return ((system_time() + index) % (maxSize >> 9)) * 512;
}


static uint32
compute_check_sum(Node *device, off_t offset)
{
	TRACE("%s: called\n", __func__);

	char buffer[512];
	ssize_t bytesRead = device->ReadAt(NULL, offset, buffer, sizeof(buffer));
	if (bytesRead < B_OK)
		return 0;

	if (bytesRead < (ssize_t)sizeof(buffer))
		memset(buffer + bytesRead, 0, sizeof(buffer) - bytesRead);

	uint32 *array = (uint32*)buffer;
	uint32 sum = 0;

	for (uint32 i = 0; i < (bytesRead + sizeof(uint32) - 1) / sizeof(uint32); i++)
		sum += array[i];

	return sum;
}


static bool
device_contains_partition(EfiDevice *device, boot::Partition *partition)
{
	EFI::Header *header = (EFI::Header*)partition->content_cookie;
	if (header != NULL && header->InitCheck() == B_OK) {
		// check if device is GPT, and contains partition entry
		uint32 blockSize = device->BlockSize();
		gpt_table_header *deviceHeader =
			(gpt_table_header*)malloc(blockSize);
		ssize_t bytesRead = device->ReadAt(NULL, blockSize, deviceHeader,
			blockSize);
		if (bytesRead != blockSize)
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
		if (bytesRead != size)
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
			"blocksize: %" B_PRIu32 ", lastblock: %" B_PRIu64 "\n",
			__func__, blockIo,
			blockIo->Media->MediaPresent ? "true" : "false",
			blockIo->Media->LogicalPartition ? "true" : "false",
			blockIo->Media->RemovableMedia ? "true" : "false",
			blockIo->Media->BlockSize, blockIo->Media->LastBlock);

		if (!blockIo->Media->MediaPresent || blockIo->Media->LogicalPartition)
			continue;

		// The qemu flash device with a 256K block sizes sometime show up
		// in edk2. If flash is unconfigured, bad things happen on arm.
		// edk2 bug: https://bugzilla.tianocore.org/show_bug.cgi?id=2856
		// We're not ready for flash devices in efi, so skip anything odd.
		if (blockIo->Media->BlockSize > 8192)
			continue;

		EfiDevice *device = new(std::nothrow)EfiDevice(blockIo);
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
	return B_ENTRY_NOT_FOUND;
}


status_t
platform_get_boot_partitions(struct stage2_args *args, Node *bootDevice,
		NodeList *partitions, NodeList *bootPartitions)
{
	NodeIterator iterator = partitions->GetIterator();
	boot::Partition *partition = NULL;
	while ((partition = (boot::Partition*)iterator.Next()) != NULL) {
		if (device_contains_partition((EfiDevice*)bootDevice, partition)) {
			iterator.Remove();
			bootPartitions->Insert(partition);
		}
	}

	return bootPartitions->Count() > 0 ? B_OK : B_ENTRY_NOT_FOUND;
}


status_t
platform_register_boot_device(Node *device)
{
	TRACE("%s: called\n", __func__);

	disk_identifier identifier;

	identifier.bus_type = UNKNOWN_BUS;
	identifier.device_type = UNKNOWN_DEVICE;
	identifier.device.unknown.size = device->Size();

	for (uint32 i = 0; i < NUM_DISK_CHECK_SUMS; ++i) {
		off_t offset = get_next_check_sum_offset(i, device->Size());
		identifier.device.unknown.check_sums[i].offset = offset;
		identifier.device.unknown.check_sums[i].sum = compute_check_sum(device,
			offset);
	}

	// ...HARD_DISK, as we pick partition and have checksum (no need to use _CD)
	gBootVolume.SetInt32(BOOT_METHOD, BOOT_METHOD_HARD_DISK);
	gBootVolume.SetData(BOOT_VOLUME_DISK_IDENTIFIER, B_RAW_TYPE,
		&identifier, sizeof(disk_identifier));

	return B_OK;
}


void
platform_cleanup_devices()
{
}
