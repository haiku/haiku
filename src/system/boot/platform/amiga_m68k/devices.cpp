/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>
#include <boot/platform.h>
#include <boot/partitions.h>
#include <boot/stdio.h>
#include <boot/stage2.h>

#include <string.h>

#include "Handle.h"
#include "rom_calls.h"

//#define TRACE_DEVICES
#ifdef TRACE_DEVICES
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// exported from shell.S
extern uint8 gBootedFromImage;
extern uint8 gBootDriveAPI; // ATARI_BOOT_DRIVE_API_*
extern uint8 gBootDriveID;
extern uint32 gBootPartitionOffset;

#define SCRATCH_SIZE (2*4096)
static uint8 gScratchBuffer[SCRATCH_SIZE];


//	#pragma mark -


ExecDevice::ExecDevice(struct IORequest *ioRequest)
{
	fIORequest = ioRequest;
	fIOStdReq = (struct IOStdReq *)ioRequest;
}


ExecDevice::ExecDevice(size_t requestSize)
{
	AllocRequest(requestSize);
}


ExecDevice::ExecDevice()
{
	fIORequest = NULL;
	fIOStdReq = NULL;
}


ExecDevice::~ExecDevice()
{
	CloseDevice(fIORequest);
	DeleteIORequest(fIORequest);
}


status_t
ExecDevice::AllocRequest(size_t requestSize)
{
	struct MsgPort *inputPort = CreateMsgPort();
	if (inputPort == NULL)
		panic("CreateMsgPort()");
	
	fIORequest = (struct IORequest *)CreateIORequest(inputPort, requestSize);
	if (fIORequest == NULL)
		panic("CreateIORequest()");
	fIOStdReq = (struct IOStdReq *)fIORequest;
	return B_OK;
}


status_t
ExecDevice::Open(const char *name, unsigned long unit, unsigned long flags)
{
	status_t err = B_OK;

	if (fIORequest == NULL)
		err = AllocRequest(sizeof(struct IOStdReq));
	if (err < B_OK)
		return err;
	err = exec_error(OpenDevice((uint8 *)name, unit, fIORequest, flags));
	if (err < B_OK)
		return err;
	return B_OK;
}


ssize_t
ExecDevice::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	fIOStdReq->io_Command = CMD_READ;
	fIOStdReq->io_Length = bufferSize;
	fIOStdReq->io_Data = buffer;
	fIOStdReq->io_Offset = (uint32)pos;
	status_t err = Do();
	if (err < B_OK)
		return err;
	return (ssize_t)fIOStdReq->io_Actual;
}


ssize_t
ExecDevice::WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize)
{
	fIOStdReq->io_Command = CMD_WRITE;
	fIOStdReq->io_Length = bufferSize;
	fIOStdReq->io_Data = (void *)buffer;
	fIOStdReq->io_Offset = (uint32)pos;
	status_t err = Do();
	if (err < B_OK)
		return err;
	return (ssize_t)fIOStdReq->io_Actual;
}


off_t 
ExecDevice::Size() const
{
	
	// ToDo: fix this!
	return 1024LL * 1024 * 1024 * 1024;
		// 1024 GB
}


status_t
ExecDevice::Do()
{
	status_t err;
	err = exec_error(DoIO(fIORequest));
	return err;
}


status_t
ExecDevice::Clear()
{
	fIOStdReq->io_Command = CMD_CLEAR;
	status_t err = Do();
	if (err < B_OK)
		return err;
	return B_OK;
}


//	#pragma mark -


status_t 
platform_add_boot_device(struct stage2_args *args, NodeList *devicesList)
{
	TRACE(("boot drive ID: %x\n", gBootDriveID));

//TODO
	gBootVolume.SetBool(BOOT_VOLUME_BOOTED_FROM_IMAGE, gBootedFromImage);

	return B_OK;
}


status_t
platform_get_boot_partition(struct stage2_args *args, Node *bootDevice,
	NodeList *list, boot::Partition **_partition)
{

//TODO

	return B_ENTRY_NOT_FOUND;
}


status_t
platform_add_block_devices(stage2_args *args, NodeList *devicesList)
{
//TODO
	return B_ERROR;
}


status_t 
platform_register_boot_device(Node *device)
{
//TODO

	return B_OK;
}


void
platform_cleanup_devices()
{
}
