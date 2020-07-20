/*
 * Copyright 2016-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/partitions.h>
#include <boot/platform.h>
#include <boot/stage2.h>

#include "efi_platform.h"
#include <efi/protocol/block-io.h>


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
		bool ReadOnly() const { return fBlockIo->Media->ReadOnly; }
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

	uint32 numBlocks = (offset + bufferSize + BlockSize()) / BlockSize();
	char readBuffer[numBlocks * BlockSize()];

	if (fBlockIo->ReadBlocks(fBlockIo, fBlockIo->Media->MediaId,
		pos, sizeof(readBuffer), readBuffer) != EFI_SUCCESS)
		return B_ERROR;

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
			"blocksize: %" B_PRIuSIZE ", lastblock: %" B_PRIu64 "\n",
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
platform_get_boot_partition(struct stage2_args *args, Node *bootDevice,
		NodeList *partitions, boot::Partition **_partition)
{
	TRACE("%s: called\n", __func__);
	*_partition = (boot::Partition*)partitions->GetIterator().Next();
	return *_partition != NULL ? B_OK : B_ENTRY_NOT_FOUND;
}


status_t
platform_register_boot_device(Node *device)
{
	TRACE("%s: called\n", __func__);

	EfiDevice *efiDevice = (EfiDevice *)device;
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

	gBootVolume.SetInt32(BOOT_METHOD, efiDevice->ReadOnly() ? BOOT_METHOD_CD:
		BOOT_METHOD_HARD_DISK);
	gBootVolume.SetBool(BOOT_VOLUME_BOOTED_FROM_IMAGE, efiDevice->ReadOnly());
	gBootVolume.SetData(BOOT_VOLUME_DISK_IDENTIFIER, B_RAW_TYPE,
		&identifier, sizeof(disk_identifier));

	return B_OK;
}


void
platform_cleanup_devices()
{
}
