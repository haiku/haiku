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


status_t 
platform_add_boot_device(struct stage2_args *args, NodeList *devicesList)
{
	TRACE(("boot drive ID: %x\n", gBootDriveID));

//TODO
	gKernelArgs.boot_volume.SetBool(BOOT_VOLUME_BOOTED_FROM_IMAGE,
		gBootedFromImage);

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

