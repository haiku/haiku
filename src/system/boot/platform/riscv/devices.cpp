/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "bios.h"
#include "virtio.h"

#include <KernelExport.h>
#include <boot/platform.h>
#include <boot/partitions.h>
#include <boot/stdio.h>
#include <boot/stage2.h>

#include <AutoDeleter.h>

#include <string.h>
#include <new>

//#define TRACE_DEVICES
#ifdef TRACE_DEVICES
# define TRACE(x...) dprintf(x)
#else
# define TRACE(x...) ;
#endif


void* aligned_malloc(size_t required_bytes, size_t alignment);
void aligned_free(void* p);


class VirtioBlockDevice : public Node
{
	public:
		VirtioBlockDevice(VirtioDevice* blockIo);
		virtual ~VirtioBlockDevice();

		virtual ssize_t ReadAt(void* cookie, off_t pos, void* buffer,
			size_t bufferSize);
		virtual ssize_t WriteAt(void* cookie, off_t pos, const void* buffer,
			size_t bufferSize) { return B_UNSUPPORTED; }
		virtual off_t Size() const {
			return (*(uint32*)(&fBlockIo->Regs()->config[0])
				+ ((uint64)(*(uint32*)(&fBlockIo->Regs()->config[4])) << 32)
				)*kVirtioBlockSectorSize;
		}

		uint32 BlockSize() const { return kVirtioBlockSectorSize; }
		bool ReadOnly() const { return false; }
	private:
		ObjectDeleter<VirtioDevice> fBlockIo;
};


VirtioBlockDevice::VirtioBlockDevice(VirtioDevice* blockIo)
	:
	fBlockIo(blockIo)
{
	dprintf("+VirtioBlockDevice\n");
}


VirtioBlockDevice::~VirtioBlockDevice()
{
	dprintf("-VirtioBlockDevice\n");
}


ssize_t
VirtioBlockDevice::ReadAt(void* cookie, off_t pos, void* buffer,
	size_t bufferSize)
{
	// dprintf("ReadAt(%p, %ld, %p, %ld)\n", cookie, pos, buffer, bufferSize);

	off_t offset = pos % BlockSize();
	pos /= BlockSize();

	uint32 numBlocks = (offset + bufferSize + BlockSize() - 1) / BlockSize();

	ArrayDeleter<char> readBuffer(
		new(std::nothrow) char[numBlocks * BlockSize() + 1]);
	if (!readBuffer.IsSet())
		return B_NO_MEMORY;

	VirtioBlockRequest blkReq;
	blkReq.type = kVirtioBlockTypeIn;
	blkReq.ioprio = 0;
	blkReq.sectorNum = pos;
	IORequest req(ioOpRead, &blkReq, sizeof(blkReq));
	IORequest reply(ioOpWrite, readBuffer.Get(), numBlocks * BlockSize() + 1);
	IORequest* reqs[] = {&req, &reply};
	fBlockIo->ScheduleIO(reqs, 2);
	fBlockIo->WaitIO();

	if (readBuffer[numBlocks * BlockSize()] != kVirtioBlockStatusOk) {
		dprintf("%s: blockIo error reading from device!\n", __func__);
		return B_ERROR;
	}

	memcpy(buffer, readBuffer.Get() + offset, bufferSize);

	return bufferSize;
}


static VirtioBlockDevice*
CreateVirtioBlockDev(int id)
{
	VirtioResources* devRes = ThisVirtioDev(kVirtioDevBlock, id);
	if (devRes == NULL) return NULL;

	ObjectDeleter<VirtioDevice> virtioDev(
		new(std::nothrow) VirtioDevice(*devRes));
	if (!virtioDev.IsSet())
		panic("Can't allocate memory for VirtioDevice!");

	ObjectDeleter<VirtioBlockDevice> device(
		new(std::nothrow) VirtioBlockDevice(virtioDev.Detach()));
	if (!device.IsSet())
		panic("Can't allocate memory for VirtioBlockDevice!");

	return device.Detach();
}


//#pragma mark -

status_t
platform_add_boot_device(struct stage2_args* args, NodeList* devicesList)
{
	for (int i = 0;; i++) {
		ObjectDeleter<VirtioBlockDevice> device(CreateVirtioBlockDev(i));
		if (!device.IsSet()) break;
		dprintf("virtio_block[%d]\n", i);
		devicesList->Insert(device.Detach());
	}
	return devicesList->Count() > 0 ? B_OK : B_ENTRY_NOT_FOUND;
}


status_t
platform_add_block_devices(struct stage2_args* args, NodeList* devicesList)
{
	return B_ENTRY_NOT_FOUND;
}


status_t
platform_get_boot_partitions(struct stage2_args* args, Node* bootDevice,
	NodeList *list, NodeList *partitionList)
{
	NodeIterator iterator = list->GetIterator();
	boot::Partition *partition = NULL;
	while ((partition = (boot::Partition *)iterator.Next()) != NULL) {
		// ToDo: just take the first partition for now
		partitionList->Insert(partition);
		return B_OK;
	}
	return B_ENTRY_NOT_FOUND;
}


status_t
platform_register_boot_device(Node* device, disk_identifier* defaultDiskID)
{
	TRACE("%s: called\n", __func__);
	gBootParams.SetInt32(BOOT_METHOD, BOOT_METHOD_HARD_DISK);
	gBootParams.SetBool(BOOT_VOLUME_BOOTED_FROM_IMAGE, false);
	gBootParams.SetData(BOOT_VOLUME_DISK_IDENTIFIER, B_RAW_TYPE,
		defaultDiskID, sizeof(disk_identifier));

	return B_OK;
}


void
platform_cleanup_devices()
{
}
