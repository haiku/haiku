/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Modified Sep 2001 by Rob Judd <judd@ob-wan.com>
** Distributed under the terms of the NewOS License.
*/

#include <kernel.h>
#include <console.h>
#include <debug.h>
#include <memheap.h>
#include <int.h>
#include <OS.h>
#include <vfs.h>
#include <errors.h>

#include <arch/cpu.h>
#include <arch/int.h>

#include <string.h>
#include <stdio.h>

#include <arch/x86/ide_bus.h>

#include <devfs.h>

#include "ide_private.h"
#include "ide_raw.h"

ide_device   devices[MAX_DEVICES];
sem_id ide_sem;

#define IDE_0_INTERRUPT  14
#define IDE_1_INTERRUPT  15
#define MAX_PARTITIONS   8

typedef struct
{
	ide_device*	dev;			// Pointer to entry in 'devices' table

	// Specs for whole disk or partition
	uint32		block_start;	// Number of first block
	uint32		block_count;	// Number of blocks used
	uint16		block_size;		// Bytes per block
} ide_ident;

//--------------------------------------------------------------------------------
static int ide_open(const char *name, uint32 flags, void **cookie)
{
	ide_ident* ident = (ide_ident*)kmalloc(sizeof(ide_ident));
	/* We hold our 'ident' structure as cookie, as it contains all we need */
	*cookie = ident;

	return NO_ERROR;
}

//--------------------------------------------------------------------------------
static int ide_close(void * _cookie)
{
	return NO_ERROR;
}

//--------------------------------------------------------------------------------
static int ide_freecookie(void *cookie)
{
	kfree(cookie);
	return NO_ERROR;
}

//--------------------------------------------------------------------------------
static int ide_seek(void * cookie, off_t pos, seek_type st)
{
	return ERR_UNIMPLEMENTED;
}

//--------------------------------------------------------------------------------
static int ide_get_geometry(ide_device* device, void *buf, size_t len)
{
	drive_geometry* drive_geometry = buf;

	if (len < sizeof(drive_geometry))
		return ERR_VFS_INSUFFICIENT_BUF;

	drive_geometry->blocks = device->end_block - device->start_block;
	drive_geometry->heads = device->hardware_device.heads;
	drive_geometry->cylinders = device->hardware_device.cyls;
	drive_geometry->sectors = device->hardware_device.sectors;
	drive_geometry->removable = false;
	drive_geometry->bytes_per_sector = device->bytes_per_sector;
	drive_geometry->read_only = false;
	strcpy(drive_geometry->model, device->hardware_device.model);
	strcpy(drive_geometry->serial, device->hardware_device.serial);
	strcpy(drive_geometry->firmware, device->hardware_device.firmware);

	return NO_ERROR;
}


//--------------------------------------------------------------------------------
static int ide_ioctl(void * _cookie, uint32 op, void *buf, size_t len)
{
	ide_ident* cookie = (ide_ident*)_cookie;
	int err = 0;

	acquire_sem(ide_sem);

	switch(op) {
		case DISK_GET_GEOMETRY:
			err = ide_get_geometry(cookie->dev,buf,len);
			break;

		case DISK_USE_DMA:
		case DISK_USE_BUS_MASTERING:
			err = ERR_UNIMPLEMENTED;
			break;

		case DISK_USE_PIO:
			err = NO_ERROR;
			break;

		case DISK_GET_ACCOUSTIC_LEVEL:
			if (len != sizeof(int8)) {
				err = ERR_INVALID_ARGS;
			} else {
				err = ide_get_accoustic(cookie->dev, (int8*)buf);
			}
			break;

		case DISK_SET_ACCOUSTIC_LEVEL:
			if (len != sizeof(int8)) {
				err = ERR_INVALID_ARGS;
			} else {
				err = ide_set_accoustic(cookie->dev,*(int8*)buf);
			}
			break;

		default:
			err = ERR_INVALID_ARGS;
	}

	release_sem(ide_sem);

	return err;
}

//--------------------------------------------------------------------------------
static ssize_t ide_read(void * _cookie, off_t pos, void *buf, size_t *len)
{
	ide_ident*	cookie = (ide_ident*)_cookie;
	uint32		sectors;
	uint32		currentSector;
	uint32		sectorsToRead;
	uint32		block;

	if (cookie == NULL) {
		return ERR_INVALID_ARGS;
	}

	// Make sure noone else is doing IDE
	acquire_sem(ide_sem);

	// Calculate start block and number of blocks to read
	block = pos / cookie->block_size;
	block += cookie->block_start;
	sectors = *len / cookie->block_size;

	// correct len to be the actual # of bytes to read
	*len -= *len % cookie->block_size;

	// If it goes beyond the disk/partition, exit
	if (block + sectors > cookie->block_start + cookie->block_count) {
		release_sem(ide_sem);
		*len = 0;
		/* XXX - should be returning an error */
		return 0;
	}

	// Start reading the sectors
	currentSector = 0;
	while(currentSector < sectors) {
		// Read max. of 255 sectors at a time
		sectorsToRead = (sectors - currentSector) > 255 ? 255 : sectors;

		// If the read fails, exit with I/O error
		if (ide_read_block(cookie->dev, buf, block, sectorsToRead) != 0) {
			release_sem(ide_sem);
			return ERR_IO_ERROR;
		}

		// Move to next block to read
		block += sectorsToRead * cookie->block_size;
		currentSector += sectorsToRead;
	}

	// Give up
	release_sem(ide_sem);

	return 0;
}

//--------------------------------------------------------------------------------
static ssize_t ide_write(void * _cookie, off_t pos, const void *buf, size_t *len)
{
	int			block;
	ide_ident*	cookie = _cookie;
	uint32		sectors;
	uint32		currentSector;
	uint32		sectorsToWrite;

	dprintf("ide_write: entry buf %p, pos 0x%Lx, *len %ld\n", buf, pos, *len);
	if(cookie == NULL) {
		return ERR_INVALID_ARGS;
	}

	// Make sure no other I/O is done
	acquire_sem(ide_sem);

	// Get the start pos and block count to write
	block = pos / cookie->block_size + cookie->block_start;
	sectors = *len / cookie->block_size;

	// If we're writing more than the disk/partition size
	if (block + sectors > cookie->block_start + cookie->block_count) {
		// exit without writing
		release_sem(ide_sem);
		return 0;
	}

	// Loop over sectors to write
	currentSector = 0;
	while(currentSector < sectors) {
		// Write a max of 255 sectors at a time
		sectorsToWrite = (sectors - currentSector) > 255 ? 255 : sectors;

		// Write them
		if (ide_write_block(cookie->dev, buf, block, sectorsToWrite) != 0) {
			//	  dprintf("ide_write: ide_block returned %d\n", rc);
			*len = currentSector * cookie->block_size;
			release_sem(ide_sem);
			return ERR_IO_ERROR;
		}

		block += sectorsToWrite * cookie->block_size;
		currentSector += sectorsToWrite;
	}

	release_sem(ide_sem);

	return 0;
}

//--------------------------------------------------------------------------------
static int ide_canpage(void * ident)
{
	return false;
}

//--------------------------------------------------------------------------------
static ssize_t ide_readpage(void * ident, iovecs *vecs, off_t pos)
{
	return ERR_UNIMPLEMENTED;
}

//--------------------------------------------------------------------------------
static ssize_t ide_writepage(void * ident, iovecs *vecs, off_t pos)
{
	return ERR_UNIMPLEMENTED;
}

//--------------------------------------------------------------------------------
device_hooks ide_hooks = {
	ide_open,
	ide_close,
	ide_freecookie,
	ide_ioctl,
	ide_read,
	ide_write,
	NULL,
	NULL,
//	NULL,
//	NULL
};

//--------------------------------------------------------------------------------
static int ide_interrupt_handler(void* data)
{
  dprintf("in ide interrupt handler\n");
  return  INT_RESCHEDULE;
}

//--------------------------------------------------------------------------------
static ide_ident* ide_create_device_ident(ide_device* dev, int16 partition)
{
	ide_ident* ident = kmalloc(sizeof(ide_ident));
	if (ident != NULL) {
		ident->dev = dev;
		ident->block_size = dev->bytes_per_sector;

		if (partition >= 0 && partition < MAX_PARTITIONS) {
			ident->block_start = dev->partitions[partition].starting_block;
			ident->block_count = dev->partitions[partition].sector_count;
		}
		else
		{
			ident->block_start = 0;
			ident->block_count = dev->sector_count;
		}
	}

	return ident;
}

//--------------------------------------------------------------------------------
static bool ide_attach_device(int bus, int device)
{
	ide_device*	ide = &devices[(bus*2) + device];
	ide_ident*	ident = NULL;
	char		devpath[256];
	int			part;

	ide->bus = bus;
	ide->device = device;

	if (ide_identify_device(bus, device)) {
		ide->magic = IDE_MAGIC_COOKIE;
		ide->magic2 = IDE_MAGIC_COOKIE2;

		if (ide_get_partitions(ide)) {
			for (part=0; part < NUM_PARTITIONS*2; part++) {
				if (ide->partitions[part].partition_type != 0 &&
					(ident=ide_create_device_ident(ide, part)) != NULL) {
					sprintf(devpath, "disk/ide/ata/%d/%d/%d", bus, device, part);
					devfs_publish_device(devpath, ident, &ide_hooks);
				}
			}

			dprintf("ide_attach_device(bus=%d,dev=%d) rc=true\n", bus, device);

			return true;
		}
	}

	dprintf("ide_attach_device(bus=%d,dev=%d) rc=false\n", bus, device);

	return false;
}

//--------------------------------------------------------------------------------
static bool ide_attach_buses(unsigned int bus)
{
//	char devpath[256];
	int found = 0;
	int dev;

	for(dev=0; dev < 2; dev++)
		if (ide_attach_device(bus, dev))
			found++;

	return(found > 0 ? true : false);
}

//--------------------------------------------------------------------------------
// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//--------------------------------------------------------------------------------
// Don't try and attach more then one bus to the driver, as ide_raw_init
// overwrites globals and you end up writing blocks which were meant
// for bus #1 to bus #2 :(
//
// !!!!!! So, only use the one or the other, _never_ both !!!!!!
//--------------------------------------------------------------------------------
int ide_bus_init(kernel_args *ka)
{
	// Create our top-level semaphore
	ide_sem = create_sem(1, "ide_sem");
	if (ide_sem < 0) {
		// We failed, so tell caller
		return ide_sem;
	}

	// attach ide bus #1
	int_set_io_interrupt_handler(0x20 + IDE_0_INTERRUPT, &ide_interrupt_handler, NULL);
	ide_raw_init(0x1f0, 0x3f0);
	ide_attach_buses(0);

	// attach ide bus #2
//	int_set_io_interrupt_handler(0x20 + IDE_1_INTERRUPT, &ide_interrupt_handler, NULL);
//	ide_raw_init(0x170, 0x370);
//	ide_attach_buses(1);

	return 0;
}
