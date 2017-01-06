/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/partitions.h>
#include <boot/platform.h>
#include <boot/stage2.h>

#include "efi_platform.h"


static EFI_GUID BlockIoGUID = BLOCK_IO_PROTOCOL;
static EFI_GUID LoadedImageGUID = LOADED_IMAGE_PROTOCOL;
static EFI_GUID DevicePathGUID = DEVICE_PATH_PROTOCOL;


class EfiDevice : public Node
{
	public:
		EfiDevice(EFI_BLOCK_IO *blockIo, EFI_DEVICE_PATH *devicePath);
		virtual ~EfiDevice();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer,
			size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer,
			size_t bufferSize) { return B_UNSUPPORTED; }
		virtual off_t Size() const {
			return (fBlockIo->Media->LastBlock + 1) * BlockSize(); }

		uint32 BlockSize() const { return fBlockIo->Media->BlockSize; }
		//TODO: Check for ATAPI messaging in device path?
		bool IsCD() { return fBlockIo->Media->ReadOnly;}
	private:
		EFI_BLOCK_IO*		fBlockIo;
		EFI_DEVICE_PATH*	fDevicePath;
};


EfiDevice::EfiDevice(EFI_BLOCK_IO *blockIo, EFI_DEVICE_PATH *devicePath)
	:
	fBlockIo(blockIo),
	fDevicePath(devicePath)
{
}


EfiDevice::~EfiDevice()
{
}


ssize_t
EfiDevice::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	uint32 offset = pos % BlockSize();
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
	if (index < 2)
		return index * 512;

	if (index < 4)
		return (maxSize >> 10) + index * 2048;

	return ((system_time() + index) % (maxSize >> 9)) * 512;
	//return 42 * 512;
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
	EFI_LOADED_IMAGE *loadedImage;
	if  (kBootServices->HandleProtocol(kImage, &LoadedImageGUID,
			(void**)&loadedImage) != EFI_SUCCESS)
		return B_ERROR;

	EFI_DEVICE_PATH *devicePath, *node;
	if (kBootServices->HandleProtocol(loadedImage->DeviceHandle,
			&DevicePathGUID, (void **)&devicePath) != EFI_SUCCESS)
		panic("Failed to lookup boot device path!");

	for (node = devicePath;DevicePathType(node) != MESSAGING_DEVICE_PATH;
			node = NextDevicePathNode(node)) {
		if (IsDevicePathEnd(node))
			panic("Could not find disk of EFI partition!");
	}

	SetDevicePathEndNode(NextDevicePathNode(node));

	EFI_HANDLE handle;
	if (kBootServices->LocateDevicePath(&BlockIoGUID, &devicePath, &handle)
			!= EFI_SUCCESS)
		panic("Cannot get boot device handle!");

	EFI_BLOCK_IO *blockIo;
	if (kBootServices->HandleProtocol(handle, &BlockIoGUID, (void**)&blockIo)
			!= EFI_SUCCESS)
		panic("Cannot get boot block io protocol!");

	if (!blockIo->Media->MediaPresent)
		panic("Boot disk has no media present!");


	EfiDevice *device = new(std::nothrow)EfiDevice(blockIo, devicePath);
	if (device == NULL)
		panic("Can't allocate memory for boot device!");

	devicesList->Insert(device);
	return B_OK;
}


status_t
platform_add_block_devices(struct stage2_args *args, NodeList *devicesList)
{
	EFI_BLOCK_IO *blockIo;
	UINTN memSize = 0;

	// Read to zero sized buffer to get memory needed for handles
	if (kBootServices->LocateHandle(ByProtocol, &BlockIoGUID, 0, &memSize, 0)
			!= EFI_BUFFER_TOO_SMALL)
		panic("Cannot read size of block device handles!");

	uint32 noOfHandles = memSize / sizeof(EFI_HANDLE);

	EFI_HANDLE handles[noOfHandles];
	if (kBootServices->LocateHandle(ByProtocol, &BlockIoGUID, 0, &memSize,
			handles) != EFI_SUCCESS)
		panic("Failed to locate block devices!");

	for (uint32 n = 0; n < noOfHandles; n++) {
		if (kBootServices->HandleProtocol(handles[n], &BlockIoGUID,
				(void**)&blockIo) != EFI_SUCCESS)
			panic("Cannot get block device handle!");

		// Skip partition block devices, Haiku does partition scan
		if (!blockIo->Media->MediaPresent || blockIo->Media->LogicalPartition)
			continue;

		EFI_DEVICE_PATH *devicePath;
		// Atm devicePath isn't necessary so result isn't checked
		kBootServices->HandleProtocol(handles[n], &DevicePathGUID,
			(void**)&devicePath);

		EfiDevice *device = new(std::nothrow)EfiDevice(blockIo, devicePath);
		if (device == NULL)
			panic("Can't allocate memory for block devices!");
		devicesList->Insert(device);
	}
	return devicesList->Count() > 0 ? B_OK : B_ENTRY_NOT_FOUND;
}


status_t
platform_get_boot_partition(struct stage2_args *args, Node *bootDevice,
		NodeList *partitions, boot::Partition **_partition)
{
	NodeIterator iterator = partitions->GetIterator();
	boot::Partition *partition = NULL;
	while ((partition = (boot::Partition *)iterator.Next()) != NULL) {
		//Currently we pick first partition. If not found menu lets user select.
		*_partition = partition;
		return B_OK;
	}
	return B_ENTRY_NOT_FOUND;
}


status_t
platform_register_boot_device(Node *device)
{
	EfiDevice *efiDevice = (EfiDevice *)device;
	disk_identifier identifier;

	// TODO: Setup using device path
	identifier.bus_type = UNKNOWN_BUS;
	identifier.device_type = UNKNOWN_DEVICE;
	identifier.device.unknown.size = device->Size();

	for (uint32 i = 0; i < NUM_DISK_CHECK_SUMS; ++i) {
		off_t offset = get_next_check_sum_offset(i, device->Size());
		identifier.device.unknown.check_sums[i].offset = offset;
		identifier.device.unknown.check_sums[i].sum = compute_check_sum(device, offset);
	}

	gBootVolume.SetInt32(BOOT_METHOD, efiDevice->IsCD() ? BOOT_METHOD_CD:
		BOOT_METHOD_HARD_DISK);
	gBootVolume.SetBool(BOOT_VOLUME_BOOTED_FROM_IMAGE, true);
	gBootVolume.SetData(BOOT_VOLUME_DISK_IDENTIFIER, B_RAW_TYPE,
		&identifier, sizeof(disk_identifier));

	return B_OK;
}
