/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/partitions.h>
#include <boot/platform.h>
#include <boot/stage2.h>

#include "efi_platform.h"


class EfiDevice : public Node
{
	public:
		EfiDevice(EFI_BLOCK_IO *blockIo, EFI_DEVICE_PATH *devicePath);
		virtual ~EfiDevice();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer,
			size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer,
			size_t bufferSize) { return B_UNSUPPORTED; }
		virtual off_t Size() const { return fSize; }

		uint32 BlockSize() const { return fBlockSize; }
	private:
		EFI_BLOCK_IO*		fBlockIo;
		EFI_DEVICE_PATH*	fDevicePath;

		uint32				fBlockSize;
		uint64				fSize;
};


EfiDevice::EfiDevice(EFI_BLOCK_IO *blockIo, EFI_DEVICE_PATH *devicePath)
	:
	fBlockIo(blockIo),
	fDevicePath(devicePath)
{
	fBlockSize = fBlockIo->Media->BlockSize;
	fSize = (fBlockIo->Media->LastBlock + 1) * fBlockSize;
}


EfiDevice::~EfiDevice()
{
}


ssize_t
EfiDevice::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	uint32 offset = pos % fBlockSize;
	pos /= fBlockSize;

	uint32 numBlocks = (offset + bufferSize + fBlockSize) / fBlockSize;
	char readBuffer[numBlocks * fBlockSize];

	EFI_STATUS status = fBlockIo->ReadBlocks(fBlockIo, fBlockIo->Media->MediaId,
		pos, sizeof(readBuffer), readBuffer);

	if (status != EFI_SUCCESS)
		return B_ERROR;

	memcpy(buffer, readBuffer + offset, bufferSize);

	return bufferSize;
}


static EFI_DEVICE_PATH*
find_device_path(EFI_DEVICE_PATH *devicePath, uint16 type, uint16 subType)
{
	EFI_DEVICE_PATH *node = devicePath;
	while (!IsDevicePathEnd(node)) {
		if (DevicePathType(node) == type
			&& (subType == 0xFFFF || DevicePathSubType(node) == subType))
			return node;

		node = NextDevicePathNode(node);
	}

	return NULL;
}


static status_t
add_boot_devices(NodeList *devicesList)
{
	EFI_GUID blockIoGuid = BLOCK_IO_PROTOCOL;
	EFI_GUID devicePathGuid = DEVICE_PATH_PROTOCOL;
	EFI_BLOCK_IO *blockIo;
	EFI_DEVICE_PATH *devicePath, *node, *targetDevicePath = NULL;
	EFI_HANDLE *handles = NULL;
	EFI_STATUS status;
	UINTN size = 0;

	status = kBootServices->LocateHandle(ByProtocol, &blockIoGuid, 0, &size, 0);
	if (status != EFI_BUFFER_TOO_SMALL)
		return B_ENTRY_NOT_FOUND;

	handles = (EFI_HANDLE*)malloc(size);
	status = kBootServices->LocateHandle(ByProtocol, &blockIoGuid, 0, &size,
		handles);
	if (status != EFI_SUCCESS) {
		if (handles != NULL)
			free(handles);
		return B_ENTRY_NOT_FOUND;
	}

	for (int n = (size / sizeof(EFI_HANDLE)) - 1; n >= 0; --n) {
		status = kBootServices->HandleProtocol(handles[n], &devicePathGuid,
			(void**)&devicePath);
		if (status != EFI_SUCCESS)
			continue;

		node = devicePath;
		while (!IsDevicePathEnd(NextDevicePathNode(node)))
			node = NextDevicePathNode(node);

		if (DevicePathType(node) == MEDIA_DEVICE_PATH
			&& DevicePathSubType(node) == MEDIA_CDROM_DP) {
			targetDevicePath = find_device_path(devicePath,
				MESSAGING_DEVICE_PATH, 0xFFFF);
			continue;
		}

		if (DevicePathType(node) != MESSAGING_DEVICE_PATH)
			continue;

		status = kBootServices->HandleProtocol(handles[n], &blockIoGuid,
			(void**)&blockIo);
		if (status != EFI_SUCCESS || !blockIo->Media->MediaPresent)
			continue;

		EfiDevice *device = new(std::nothrow)EfiDevice(blockIo, devicePath);
		if (device == NULL)
			continue;

		if (targetDevicePath != NULL
			&& memcmp(targetDevicePath, node, DevicePathNodeLength(node)) == 0)
			devicesList->InsertBefore(devicesList->Head(), device);
		else
			devicesList->Insert(device);

		targetDevicePath = NULL;
	}

	return devicesList->Count() > 0 ? B_OK : B_ENTRY_NOT_FOUND;
}


static off_t
get_next_check_sum_offset(int32 index, off_t maxSize)
{
	if (index < 2)
		return index * 512;

	if (index < 4)
		return (maxSize >> 10) + index * 2048;

	//return ((system_time() + index) % (maxSize >> 9)) * 512;
	return 42 * 512;
}


static uint32
compute_check_sum(Node *device, off_t offset)
{
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
	// TODO: get GUID of partition to boot, and support for SATA/ATA devices
	return add_boot_devices(devicesList);
}


status_t
platform_add_block_devices(struct stage2_args *args, NodeList *devicesList)
{
	// add_boot_devices will add all available devices, so nothing to do here
	return B_OK;
}


status_t
platform_get_boot_partition(struct stage2_args *args, Node *bootDevice,
		NodeList *partitions, boot::Partition **_partition)
{
	NodeIterator it = partitions->GetIterator();
	while (it.HasNext()) {
		boot::Partition *partition = (boot::Partition*)it.Next();
		// we're only looking for CDs atm, so just return first found
		*_partition = partition;
		return B_OK;
	}

	return B_ERROR;
}


status_t
platform_register_boot_device(Node *device)
{
	disk_identifier identifier;

	identifier.bus_type = UNKNOWN_BUS;
	identifier.device_type = UNKNOWN_DEVICE;
	identifier.device.unknown.size = device->Size();

	for (uint32 i = 0; i < NUM_DISK_CHECK_SUMS; ++i) {
		off_t offset = get_next_check_sum_offset(i, device->Size());
		identifier.device.unknown.check_sums[i].offset = offset;
		identifier.device.unknown.check_sums[i].sum = compute_check_sum(device, offset);
	}

	gBootVolume.SetInt32(BOOT_METHOD, BOOT_METHOD_CD);
	gBootVolume.SetBool(BOOT_VOLUME_BOOTED_FROM_IMAGE, true);
	gBootVolume.SetData(BOOT_VOLUME_DISK_IDENTIFIER, B_RAW_TYPE,
		&identifier, sizeof(disk_identifier));

	return B_OK;
}
