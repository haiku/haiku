/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "bios.h"

#include <boot/platform.h>
#include <boot/partitions.h>
#include <boot/stdio.h>
#include <boot/stage2.h>

#include <string.h>

//#define TRACE_DEVICES
#ifdef TRACE_DEVICES
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// exported from shell.S
extern uint8 gBootDriveID;
extern uint32 gBootPartitionOffset;

// int 0x13 definitions
#define BIOS_READ						0x0200
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

static const uint16 kParametersSizeVersion1 = 0x1a;
static const uint16 kParametersSizeVersion2 = 0x1e;
static const uint16 kParametersSizeVersion3 = 0x42;

static const uint16 kDevicePathSignature = 0xbedd;

struct drive_parameters {
	uint16		parameters_size;
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

struct device_table {
	uint16	base_address;
	uint16	control_port_address;
	uint8	_reserved1 : 4;
	uint8	is_slave : 1;
	uint8	_reserved2 : 1;
	uint8	lba_enabled : 1;
} _PACKED;

class BIOSDrive : public Node {
	public:
		BIOSDrive(uint8 driveID);
		virtual ~BIOSDrive();

		status_t InitCheck() const;

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize);

		virtual off_t Size() const;

		uint32 BlockSize() const { return fBlockSize; }

		bool HasParameters() const { return fHasParameters; }
		const drive_parameters &Parameters() const { return fParameters; }

	protected:
		uint8	fDriveID;
		bool	fLBA;
		uint64	fSize;
		uint32	fBlockSize;
		bool	fHasParameters;
		drive_parameters fParameters;
};


static status_t
get_ext_drive_parameters(uint8 drive, drive_parameters *targetParameters)
{
	drive_parameters *parameter = (drive_parameters *)kDataSegmentScratch;
	parameter->parameters_size = sizeof(drive_parameters);

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
get_drive_parameters(uint8 drive, drive_parameters *parameters)
{
	struct bios_regs regs;
	regs.eax = BIOS_GET_DRIVE_PARAMETERS;
	regs.edx = drive;
	regs.es = 0;
	regs.edi = 0;	// guard against faulty BIOS, see Ralf Brown's interrupt list
	call_bios(0x13, &regs);

	if ((regs.flags & CARRY_FLAG) != 0 || (regs.ecx & 0x3f) == 0)
		return B_ERROR;

	// fill drive_parameters structure with useful values
	parameters->parameters_size = kParametersSizeVersion1;
	parameters->flags = 0;
	parameters->cylinders = (((regs.ecx & 0xc0) << 2) | ((regs.ecx >> 8) & 0xff)) + 1;
	parameters->heads = ((regs.edx >> 8) & 0xff) + 1;
		// heads and cylinders start counting from 0
	parameters->sectors_per_track = regs.ecx & 0x3f;
	parameters->sectors = parameters->cylinders * parameters->heads
		* parameters->sectors_per_track;
	parameters->bytes_per_sector = 512;

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


/** parse EDD 3.0 drive path information */

static status_t
fill_disk_identifier_v3(disk_identifier &disk, const drive_parameters &parameters)
{
	if (parameters.parameters_size < kParametersSizeVersion3
		|| parameters.device_path_signature != kDevicePathSignature)
		return B_BAD_TYPE;

	// parse host bus

	if (!strncmp(parameters.host_bus, "PCI", 3)) {
		disk.bus_type = PCI_BUS;

		disk.bus.pci.bus = parameters.interface.pci.bus;
		disk.bus.pci.slot = parameters.interface.pci.slot;
		disk.bus.pci.function = parameters.interface.pci.function;
	} else if (!strncmp(parameters.host_bus, "ISA", 3)) {
		disk.bus_type = LEGACY_BUS;

		disk.bus.legacy.base_address = parameters.interface.legacy.base_address;
	} else {
		dprintf("unknown host bus \"%s\"\n", parameters.host_bus);
		return B_BAD_DATA;
	}

	// parse interface

	if (!strncmp(parameters.interface_type, "ATA", 3)) {
		disk.device_type = ATA_DEVICE;
		disk.device.ata.master = !parameters.device.ata.slave;
	} else if (!strncmp(parameters.interface_type, "ATAPI", 3)) {
		disk.device_type = ATAPI_DEVICE;
		disk.device.atapi.master = !parameters.device.ata.slave;
		disk.device.atapi.logical_unit = parameters.device.atapi.logical_unit;
	} else if (!strncmp(parameters.interface_type, "SCSI", 3)) {
		disk.device_type = SCSI_DEVICE;
		disk.device.scsi.logical_unit = parameters.device.scsi.logical_unit;
	} else if (!strncmp(parameters.interface_type, "USB", 3)) {
		disk.device_type = USB_DEVICE;
		disk.device.usb.tbd = parameters.device.usb.tbd;
	} else if (!strncmp(parameters.interface_type, "1394", 3)) {
		disk.device_type = FIREWIRE_DEVICE;
		disk.device.firewire.guid = parameters.device.firewire.guid;
	} else if (!strncmp(parameters.interface_type, "FIBRE", 3)) {
		disk.device_type = FIBRE_DEVICE;
		disk.device.fibre.wwd = parameters.device.fibre.wwd;
	} else {
		dprintf("unknown interface type \"%s\"\n", parameters.interface_type);
		return B_BAD_DATA;
	}

	return B_OK;
}


/** EDD 2.0 drive table information */

static status_t
fill_disk_identifier_v2(disk_identifier &disk, const drive_parameters &parameters)
{
	if (parameters.device_table.segment == 0xffff
		&& parameters.device_table.offset == 0xffff)
		return B_BAD_TYPE;

	device_table *table = (device_table *)LINEAR_ADDRESS(parameters.device_table.segment,
		parameters.device_table.offset);

	disk.bus_type = LEGACY_BUS;
	disk.bus.legacy.base_address = table->base_address;

	disk.device_type = ATA_DEVICE;
	disk.device.ata.master = !table->is_slave;

	return B_OK;
}


#define DUMPED_BLOCK_SIZE 16

void
dumpBlock(const char *buffer, int size, const char *prefix)
{
	int i;
	
	for (i = 0; i < size;) {
		int start = i;

		dprintf(prefix);
		for (; i < start+DUMPED_BLOCK_SIZE; i++) {
			if (!(i % 4))
				dprintf(" ");

			if (i >= size)
				dprintf("  ");
			else
				dprintf("%02x", *(unsigned char *)(buffer + i));
		}
		dprintf("  ");

		for (i = start; i < start + DUMPED_BLOCK_SIZE; i++) {
			if (i < size) {
				char c = buffer[i];

				if (c < 30)
					dprintf(".");
				else
					dprintf("%c", c);
			} else
				break;
		}
		dprintf("\n");
	}
}


//	#pragma mark -


BIOSDrive::BIOSDrive(uint8 driveID)
	:
	fDriveID(driveID)
{
	TRACE(("drive ID %u\n", driveID));

	if (get_ext_drive_parameters(driveID, &fParameters) != B_OK) {
		// old style CHS support

		if (get_drive_parameters(driveID, &fParameters) != B_OK)
			return;

		TRACE(("  cylinders: %lu, heads: %lu, sectors: %lu, bytes_per_sector: %u\n",
			fParameters.cylinders, fParameters.heads, fParameters.sectors_per_track,
			fParameters.bytes_per_sector));
		TRACE(("  total sectors: %Ld\n", fParameters.sectors));

		fBlockSize = 512;
		fSize = fParameters.sectors * fBlockSize;
		fLBA = false;
		fHasParameters = false;
	} else {
		TRACE(("size: %x\n", fParameters.parameters_size));
		TRACE(("drive_path_signature: %x\n", fParameters.device_path_signature));
		TRACE(("host bus: \"%s\", interface: \"%s\"\n", fParameters.host_bus,
			fParameters.interface_type));
		TRACE(("cylinders: %lu, heads: %lu, sectors: %lu, bytes_per_sector: %u\n",
			fParameters.cylinders, fParameters.heads, fParameters.sectors_per_track,
			fParameters.bytes_per_sector));
		TRACE(("total sectors: %Ld\n", fParameters.sectors));

		fBlockSize = fParameters.bytes_per_sector;
		fSize = fParameters.sectors * fBlockSize;
		fLBA = true;
		fHasParameters = true;
	}
}


BIOSDrive::~BIOSDrive()
{
}


status_t 
BIOSDrive::InitCheck() const
{
	return fSize > 0 ? B_OK : B_ERROR;
}


ssize_t 
BIOSDrive::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	uint32 offset = pos % fBlockSize;
	pos /= fBlockSize;

	uint32 blocksLeft = (bufferSize + offset + fBlockSize - 1) / fBlockSize;
	int32 totalBytesRead = 0;

	TRACE(("BIOS reads %lu bytes from %Ld (offset = %lu), drive %ld\n",
		blocksLeft * fBlockSize, pos * fBlockSize, offset, fDriveID));

	uint32 scratchSize = 24 * 1024 / fBlockSize;
		// maximum value allowed by Phoenix BIOS is 0x7f

	while (blocksLeft > 0) {
		uint32 blocksRead = blocksLeft;
		if (blocksRead > scratchSize)
			blocksRead = scratchSize;

		if (fLBA) {
			struct disk_address_packet *packet = (disk_address_packet *)kDataSegmentScratch;
			memset(packet, 0, sizeof(disk_address_packet));

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
			// Old style CHS read routine

			// We can only read up to 64 kB this way, but since scratchSize
			// is actually lower than this value, we don't have to take care
			// of this here.

			uint32 sector = pos % fParameters.sectors_per_track + 1;
				// sectors start countint at 1 (unlike head and cylinder)
			uint32 head = pos / fParameters.sectors_per_track;
			uint32 cylinder = head / fParameters.heads;
			head %= fParameters.heads;

			if (cylinder >= fParameters.cylinders)
				return B_BAD_VALUE;

			struct bios_regs regs;
			regs.eax = BIOS_READ | blocksRead;
			regs.edx = fDriveID | (head << 8);
			regs.ecx = sector | ((cylinder >> 2) & 0xc0) | ((cylinder & 0xff) << 8);
			regs.es = 0;
			regs.ebx = kExtraSegmentScratch;
			call_bios(0x13, &regs);

			if (regs.flags & CARRY_FLAG)
				return B_ERROR;
		}

		uint32 bytesRead = fBlockSize * blocksRead - offset;
		// copy no more than bufferSize bytes
		if (bytesRead > bufferSize)
			bytesRead = bufferSize;

		memcpy(buffer, (void *)(kExtraSegmentScratch + offset), bytesRead);
		pos += blocksRead;
		offset = 0;
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
	TRACE(("boot drive ID: %x\n", gBootDriveID));

	BIOSDrive *drive = new BIOSDrive(gBootDriveID);
	if (drive->InitCheck() != B_OK) {
		dprintf("no boot drive!\n");
		return B_ERROR;
	}

	TRACE(("drive size: %Ld bytes\n", drive->Size()));

	*_device = drive;
	return B_OK;
}


status_t
platform_get_boot_partition(struct stage2_args *args, Node *bootDevice,
	NodeList *list, boot::Partition **_partition)
{
	BIOSDrive *drive = static_cast<BIOSDrive *>(bootDevice);
	off_t offset = (off_t)gBootPartitionOffset * drive->BlockSize();

	dprintf("boot partition offset: %Ld\n", offset);

	NodeIterator iterator = list->GetIterator();
	boot::Partition *partition = NULL;
	while ((partition = (boot::Partition *)iterator.Next()) != NULL) {
		TRACE(("partition offset = %Ld, size = %Ld\n", partition->offset, partition->size));
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
		dprintf("number of drives: %d\n", driveCount);

	// ToDo: add other devices
	return B_OK; 
}


status_t 
platform_register_boot_device(Node *device)
{
	disk_identifier &disk = gKernelArgs.boot_disk.identifier;
	BIOSDrive *drive = (BIOSDrive *)device;

	gKernelArgs.platform_args.boot_drive_number = gBootDriveID;

	if (drive->HasParameters()) {
		const drive_parameters &parameters = drive->Parameters();

		// try all drive_parameters versions, beginning from the most informative

		if (fill_disk_identifier_v3(disk, parameters) == B_OK)
			return B_OK;

		if (fill_disk_identifier_v2(disk, parameters) == B_OK)
			return B_OK;

		// no interesting information, we have to fall back to the default
		// unknown interface/device type identifier
	}

	// ToDo: register all other BIOS drives as well!

	// not yet implemented correctly!!!

	dprintf("legacy drive identification\n");

	disk.bus_type = UNKNOWN_BUS;
	disk.device_type = UNKNOWN_DEVICE;
	disk.device.unknown.size = drive->Size();

	return B_OK;
}


