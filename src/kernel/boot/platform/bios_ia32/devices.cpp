/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include "bios.h"

#include <boot/platform.h>
#include <boot/partitions.h>
#include <boot/stdio.h>

#include <string.h>


// exported from shell.S
extern uint8 gBootDriveID;
extern uint32 gBootPartitionOffset;

// int 0x13 definitions
#define BIOS_GET_DRIVE_PARAMETERS		0x0800
#define BIOS_IS_EXT_PRESENT				0x4100
#define BIOS_EXT_READ					0x4200
#define BIOS_GET_EXT_DRIVE_PARAMETERS	0x4800

struct real_addr {
	uint16	offset;
	uint16	segment;
};

struct disk_address_packet {
	uint8		size;
	uint8		reserved;
	uint16		number_of_blocks;
	uint32		buffer;
	uint64		lba;
	uint64		flat_buffer;
};

struct drive_parameters {
	uint16		size;
	uint16		flags;
	uint32		cylinders;
	uint32		heads;
	uint32		sectors_per_track;
	uint64		sectors;
	uint16		bytes_per_sector;
	/* edd 2.0 */
	real_addr	device_table;
	/* edd 3.0 */
	uint16		device_path_signature;
	uint8		device_path_size;
	uint8		reserved1[3];
	char		host_bus[4];
	char		interface_type[8];
	union {
		struct {
			uint16	base_address;
		} legacy;
		struct {
			uint8	bus;
			uint8	slot;
			uint8	function;
		} pci;
		uint8		reserved[8];
	} interface;
	union {
		struct {
			uint8	slave;
		} ata;
		struct {
			uint8	slave;
			uint8	logical_unit;
		} atapi;
		struct {
			uint8	logical_unit;
		} scsi;
		struct {
			uint8	tbd;
		} usb;
		struct {
			uint64	guid;
		} firewire;
		struct {
			uint64	wwd;
		} fibre;
	} device;
	uint8		reserved2;
	uint8		checksum;
} _PACKED;


class BIOSDrive : public Node {
	public:
		BIOSDrive(uint8 driveID);
		virtual ~BIOSDrive();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize);

		virtual off_t Size() const;

		uint32 BlockSize() const { return fBlockSize; }

	protected:
		uint8	fDriveID;
		bool	fLBA;
		uint64	fSize;
		uint32	fBlockSize;
		drive_parameters fParameters;
};


static status_t
get_ext_drive_parameters(uint8 drive, drive_parameters *targetParameters)
{
	drive_parameters *parameter = (drive_parameters *)kDataSegmentScratch;
	parameter->size = sizeof(drive_parameters);

	struct bios_regs regs;
	regs.eax = BIOS_GET_EXT_DRIVE_PARAMETERS;
	regs.edx = drive;
	regs.esi = (addr_t)parameter - kDataSegmentBase;
	call_bios(0x13, &regs);

	if (regs.flags & CARRY_FLAG)
		return B_ERROR;

	memcpy(targetParameters, parameter, sizeof(drive_parameters));
	return B_OK;
}


static status_t
get_number_of_drives(uint8 *_count)
{
	struct bios_regs regs;
	regs.eax = 0x800;
	regs.edx = 0x80;
	regs.es = 0;
	regs.edi = 0;
	call_bios(0x13, &regs);

	if (regs.flags & CARRY_FLAG)
		return B_ERROR;

	*_count = regs.edx & 0xff;
	return B_OK;
}


#define DUMPED_BLOCK_SIZE 16

void
dumpBlock(const char *buffer, int size, const char *prefix)
{
	int i;
	
	for (i = 0; i < size;) {
		int start = i;

		printf(prefix);
		for (; i < start+DUMPED_BLOCK_SIZE; i++) {
			if (!(i % 4))
				printf(" ");

			if (i >= size)
				printf("  ");
			else
				printf("%02x", *(unsigned char *)(buffer + i));
		}
		printf("  ");

		for (i = start; i < start + DUMPED_BLOCK_SIZE; i++) {
			if (i < size) {
				char c = buffer[i];

				if (c < 30)
					printf(".");
				else
					printf("%c", c);
			} else
				break;
		}
		printf("\n");
	}
}


//	#pragma mark -


BIOSDrive::BIOSDrive(uint8 driveID)
	:
	fDriveID(driveID)
{
	if (get_ext_drive_parameters(driveID, &fParameters) != B_OK) {
		// ToDo: old style CHS support
		printf("%d requires CHS support - not yet implemented...\n", driveID);
		fBlockSize = 512;
		fSize = 0;
		fLBA = false;
	} else {
		printf("host bus: %4s, interface: %8s\n", fParameters.host_bus, fParameters.interface_type);
		printf("cylinders: %lu, heads: %lu, sectors: %lu, bytes_per_sector: %u\n",
			fParameters.cylinders, fParameters.heads, fParameters.sectors_per_track,
			fParameters.bytes_per_sector);
		printf("total sectors: %Ld\n", fParameters.sectors);

		fBlockSize = fParameters.bytes_per_sector;
		fSize = fParameters.sectors * fBlockSize;
		fLBA = true;
	}
}


BIOSDrive::~BIOSDrive()
{
}


ssize_t 
BIOSDrive::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	//printf("read %lu bytes from %Ld\n", bufferSize, pos);

	int32 totalBytesRead = 0;

	// ToDo: convert query to blocks correctly
	uint32 offset = pos % fBlockSize;
	pos /= fBlockSize;

	uint32 blocksLeft = (bufferSize + offset + fBlockSize - 1) / fBlockSize;

	//printf("   really reads %lu bytes from %Ld (offset = %lu)\n",
	//	blocksLeft * fBlockSize, pos * fBlockSize, offset);

	uint32 scratchSize = 24 * 1024 / fBlockSize;
		// maximum value allowed by Phoenix BIOS is 0x7f

	while (blocksLeft > 0) {
		uint32 blocksRead;

		if (fLBA) {
			struct disk_address_packet *packet = (disk_address_packet *)kDataSegmentScratch;
			memset(packet, 0, sizeof(disk_address_packet));

			blocksRead = blocksLeft;
			if (blocksRead > scratchSize)
				blocksRead = scratchSize;

			packet->size = sizeof(disk_address_packet);
			packet->number_of_blocks = blocksRead;
			packet->buffer = kExtraSegmentScratch;
			packet->lba = pos;

			struct bios_regs regs;
			regs.eax = BIOS_EXT_READ;
			regs.edx = fDriveID;
			regs.esi = (addr_t)packet - kDataSegmentBase;
			call_bios(0x13, &regs);

			if (regs.flags & CARRY_FLAG)
				goto chs_read;

		} else {
	chs_read:
			// ToDo: Left to be done!
			return B_ERROR;
		}

		uint32 bytesRead = fBlockSize * blocksRead;
		// copy no more than bufferSize bytes
		if (bytesRead > bufferSize)
			bytesRead = bufferSize;

		memcpy(buffer, (void *)(kExtraSegmentScratch + offset), bytesRead);
		pos += blocksRead;
		blocksLeft -= blocksRead;
		bufferSize -= bytesRead;
		buffer = (void *)((addr_t)buffer + bytesRead);
		totalBytesRead += bytesRead;
	}

	return totalBytesRead;
}


ssize_t 
BIOSDrive::WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize)
{
	// we don't have to know how to write
	return B_NOT_ALLOWED;
}


off_t 
BIOSDrive::Size() const
{
	return fSize;
}


//	#pragma mark -


status_t 
platform_get_boot_device(struct stage2_args *args, Node **_device)
{
	printf("boot drive ID: %x\n", gBootDriveID);

	Node *drive = new BIOSDrive(gBootDriveID);
	
	printf("drive size: %Ld bytes\n", drive->Size());

	*_device = drive;
	return B_OK;
}


status_t
platform_get_boot_partition(struct stage2_args *args, Node *bootDevice,
	NodeList *list, boot::Partition **_partition)
{
	BIOSDrive *drive = static_cast<BIOSDrive *>(bootDevice);
	off_t offset = (off_t)gBootPartitionOffset * drive->BlockSize();

	printf("boot partition offset: %Ld\n", offset);

	NodeIterator iterator = list->Iterator();
	boot::Partition *partition = NULL;
	while ((partition = (boot::Partition *)iterator.Next()) != NULL) {
		// search for the partition that contains the partition
		// offset as reported by the BFS boot block
		if (offset >= partition->offset
			&& offset < partition->offset + partition->size) {
			*_partition = partition;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
platform_add_block_devices(stage2_args *args, NodeList *devicesList)
{
	uint8 driveCount;
	if (get_number_of_drives(&driveCount) == B_OK)
		printf("number of drives: %d\n", driveCount);

	return B_OK;
}

