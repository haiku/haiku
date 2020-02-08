/*
 * Copyright 2008-2010, François Revol, revol@free.fr. All rights reserved.
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
#include "toscalls.h"

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

static const uint16 kParametersSizeVersion1 = sizeof(struct tos_bpb);
static const uint16 kParametersSizeVersion2 = 0x1e;
static const uint16 kParametersSizeVersion3 = 0x42;

static const uint16 kDevicePathSignature = 0xbedd;

//XXX clean this up!
struct drive_parameters {
	struct tos_bpb bpb;
	uint16		parameters_size;
	uint16		flags;
	uint32		cylinders;
	uint32		heads;
	uint32		sectors_per_track;
	uint64		sectors;
	uint16		bytes_per_sector;
	/* edd 2.0 */
	//real_addr	device_table;
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

struct specification_packet {
	uint8	size;
	uint8	media_type;
	uint8	drive_number;
	uint8	controller_index;
	uint32	start_emulation;
	uint16	device_specification;
	uint8	_more_[9];
} _PACKED;

class BlockHandle : public Handle {
	public:
		BlockHandle(int handle);
		virtual ~BlockHandle();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize);

		virtual off_t Size() const { return fSize; };

		uint32 BlockSize() const { return fBlockSize; }

		bool HasParameters() const { return fHasParameters; }
		const drive_parameters &Parameters() const { return fParameters; }

		virtual status_t FillIdentifier();

		disk_identifier &Identifier() { return fIdentifier; }
		uint8 DriveID() const { return fHandle; }
		status_t InitCheck() const { return fSize > 0 ? B_OK : B_ERROR; };


		
		virtual ssize_t ReadBlocks(void *buffer, off_t first, int32 count);

	protected:
		uint64	fSize;
		uint32	fBlockSize;
		bool	fHasParameters;
		drive_parameters fParameters;
		disk_identifier fIdentifier;
};


class FloppyDrive : public BlockHandle {
	public:
		FloppyDrive(int handle);
		virtual ~FloppyDrive();

		status_t FillIdentifier();

		virtual ssize_t ReadBlocks(void *buffer, off_t first, int32 count);

	protected:
		status_t	ReadBPB(struct tos_bpb *bpb);
};


class BIOSDrive : public BlockHandle {
	public:
		BIOSDrive(int handle);
		virtual ~BIOSDrive();

		status_t FillIdentifier();

		virtual ssize_t ReadBlocks(void *buffer, off_t first, int32 count);

	protected:
		status_t	ReadBPB(struct tos_bpb *bpb);
};


class XHDIDrive : public BlockHandle {
	public:
		XHDIDrive(int handle, uint16 major, uint16 minor);
		virtual ~XHDIDrive();

		status_t FillIdentifier();

		virtual ssize_t ReadBlocks(void *buffer, off_t first, int32 count);

	protected:
		uint16 fMajor;
		uint16 fMinor;
};


static bool sBlockDevicesAdded = false;


static status_t
read_bpb(uint8 drive, struct tos_bpb *bpb)
{
	struct tos_bpb *p;
	p = Getbpb(drive);
	memcpy(bpb, p, sizeof(struct tos_bpb));
	/* Getbpb is buggy so we must force a media change */
	//XXX: docs seems to assume we should loop until it works
	Mediach(drive);
	return B_OK;
}

static status_t
get_drive_parameters(uint8 drive, drive_parameters *parameters)
{
	status_t err;
	err = read_bpb(drive, &parameters->bpb);
	TRACE(("get_drive_parameters: get_bpb: 0x%08lx\n", err));
	TRACE(("get_drive_parameters: bpb: %04x %04x %04x %04x %04x %04x %04x %04x %04x \n",
			parameters->bpb.recsiz, parameters->bpb.clsiz,
			parameters->bpb.clsizb, parameters->bpb.rdlen,
			parameters->bpb.fsiz, parameters->bpb.fatrec,
			parameters->bpb.datrec, parameters->bpb.numcl,
			parameters->bpb.bflags));
	
#if 0
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
#endif
	return B_OK;
}


#if 0
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
		dprintf("legacy base address %x\n", disk.bus.legacy.base_address);
	} else {
		dprintf("unknown host bus \"%s\"\n", parameters.host_bus);
		return B_BAD_DATA;
	}

	// parse interface

	if (!strncmp(parameters.interface_type, "ATA", 3)) {
		disk.device_type = ATA_DEVICE;
		disk.device.ata.master = !parameters.device.ata.slave;
		dprintf("ATA device, %s\n", disk.device.ata.master ? "master" : "slave");
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
#endif

static off_t
get_next_check_sum_offset(int32 index, off_t maxSize)
{
	// The boot block often contains the disk superblock, and should be
	// unique enough for most cases
	if (index < 2)
		return index * 512;

	// Try some data in the first part of the drive
	if (index < 4)
		return (maxSize >> 10) + index * 2048;

	// Some random value might do
	return ((system_time() + index) % (maxSize >> 9)) * 512;
}


/**	Computes a check sum for the specified block.
 *	The check sum is the sum of all data in that block interpreted as an
 *	array of uint32 values.
 *	Note, this must use the same method as the one used in kernel/fs/vfs_boot.cpp.
 */

static uint32
compute_check_sum(BlockHandle *drive, off_t offset)
{
	char buffer[512];
	ssize_t bytesRead = drive->ReadAt(NULL, offset, buffer, sizeof(buffer));
	if (bytesRead < B_OK)
		return 0;

	if (bytesRead < (ssize_t)sizeof(buffer))
		memset(buffer + bytesRead, 0, sizeof(buffer) - bytesRead);

	uint32 *array = (uint32 *)buffer;
	uint32 sum = 0;

	for (uint32 i = 0; i < (bytesRead + sizeof(uint32) - 1) / sizeof(uint32); i++) {
		sum += array[i];
	}

	return sum;
}


static void
find_unique_check_sums(NodeList *devices)
{
	NodeIterator iterator = devices->GetIterator();
	Node *device;
	int32 index = 0;
	off_t minSize = 0;
	const int32 kMaxTries = 200;

	while (index < kMaxTries) {
		bool clash = false;

		iterator.Rewind();

		while ((device = iterator.Next()) != NULL) {
			BlockHandle *drive = (BlockHandle *)device;
#if 0
			// there is no RTTI in the boot loader...
			BlockHandle *drive = dynamic_cast<BlockHandle *>(device);
			if (drive == NULL)
				continue;
#endif

			// TODO: currently, we assume that the BIOS provided us with unique
			//	disk identifiers... hopefully this is a good idea
			if (drive->Identifier().device_type != UNKNOWN_DEVICE)
				continue;

			if (minSize == 0 || drive->Size() < minSize)
				minSize = drive->Size();

			// check for clashes

			NodeIterator compareIterator = devices->GetIterator();
			while ((device = compareIterator.Next()) != NULL) {
				BlockHandle *compareDrive = (BlockHandle *)device;

				if (compareDrive == drive
					|| compareDrive->Identifier().device_type != UNKNOWN_DEVICE)
					continue;

				if (!memcmp(&drive->Identifier(), &compareDrive->Identifier(),
						sizeof(disk_identifier))) {
					clash = true;
					break;
				}
			}

			if (clash)
				break;
		}

		if (!clash) {
			// our work here is done.
			return;
		}

		// add a new block to the check sums

		off_t offset = get_next_check_sum_offset(index, minSize);
		int32 i = index % NUM_DISK_CHECK_SUMS;
		iterator.Rewind();

		while ((device = iterator.Next()) != NULL) {
			BlockHandle *drive = (BlockHandle *)device;

			disk_identifier& disk = drive->Identifier();
			disk.device.unknown.check_sums[i].offset = offset;
			disk.device.unknown.check_sums[i].sum = compute_check_sum(drive, offset);

			TRACE(("disk %x, offset %Ld, sum %lu\n", drive->DriveID(), offset,
				disk.device.unknown.check_sums[i].sum));
		}

		index++;
	}

	// If we get here, we couldn't find a way to differentiate all disks from each other.
	// It's very likely that one disk is an exact copy of the other, so there is nothing
	// we could do, anyway.

	dprintf("Could not make BIOS drives unique! Might boot from the wrong disk...\n");
}


static status_t
add_block_devices(NodeList *devicesList, bool identifierMissing)
{
	int32 map;
	uint8 driveID;
	uint8 driveCount = 0;
	
	if (sBlockDevicesAdded)
		return B_OK;

	if (init_xhdi() >= B_OK) {
		uint16 major;
		uint16 minor;
		uint32 blocksize;
		uint32 blocksize2;
		uint32 blocks;
		uint32 devflags;
		uint32 lastacc;
		char product[33];
		int32 err;

		map = XHDrvMap();
		dprintf("XDrvMap() 0x%08lx\n", map);
		// sadly XDrvmap() has the same issues as XBIOS, it only lists known partitions.
		// so we just iterate on each major and try to see if there is something.
		for (driveID = 0; driveID < 32; driveID++) {
			uint32 startsect;
			err = XHInqDev(driveID, &major, &minor, &startsect, NULL);
			if (err < 0) {
				;//dprintf("XHInqDev(%d) error %d\n", driveID, err);
			} else {
				dprintf("XHInqDev(%d): (%d,%d):%d\n", driveID, major, minor, startsect);
			}
		}

		product[32] = '\0';
		for (major = 0; major < 256; major++) {
			if (major == 64) // we don't want floppies
				continue;
			if (major > 23 && major != 64) // extensions and non-standard stuff... skip for speed.
				break;

			for (minor = 0; minor < 255; minor++) {
				if (minor && (major < 8 || major >15))
					break; // minor only used for the SCSI LUN AFAIK.
				if (minor > 15) // are more used at all ?
					break;

				product[0] = '\0';
				blocksize = 0;
				blocksize2 = 0;
#if 0
				err = XHLastAccess(major, minor, &lastacc);
				if (err < 0) {
					;//dprintf("XHLastAccess(%d,%d) error %d\n", major, minor, err);
				} else
					dprintf("XHLastAccess(%d,%d): %ld\n", major, minor, lastacc);
//continue;
#endif
				// we can pass NULL pointers but just to play safe we don't.
				err = XHInqTarget(major, minor, &blocksize, &devflags, product);
				if (err < 0) {
					dprintf("XHInqTarget(%d,%d) error %d\n", major, minor, err);
					continue;
				}
				err = XHGetCapacity(major, minor, &blocks, &blocksize2);
				if (err < 0) {
					//dprintf("XHGetCapacity(%d,%d) error %d\n", major, minor, err);
					continue;
				}

				if (blocksize == 0) {
					dprintf("XHDI: blocksize for (%d,%d) is 0!\n", major, minor);
				}
				//dprintf("XHDI: (%d,%d) blocksize1 %ld blocksize2 %ld\n", major, minor, blocksize, blocksize2);

				dprintf("XHDI(%d,%d): blksize %d, blocks %d, flags 0x%08lx, '%s'\n", major, minor, blocksize, blocks, devflags, product);
				driveID = (uint8)major;

				//if (driveID == gBootDriveID)
				//	continue;
//continue;

				BlockHandle *drive = new(nothrow) XHDIDrive(driveID, major, minor);
				if (drive->InitCheck() != B_OK) {
					dprintf("could not add drive (%d,%d)\n", major, minor);
					delete drive;
					continue;
				}

				devicesList->Add(drive);
				driveCount++;

				if (drive->FillIdentifier() != B_OK)
					identifierMissing = true;
			}
		}
	}
	if (!driveCount) { // try to fallback to BIOS XXX: use MetaDOS
		map = Drvmap();
		dprintf("Drvmap(): 0x%08lx\n", map);
		for (driveID = 0; driveID < 32; driveID++) {
			bool present = map & 0x1;
			map >>= 1;
			if (!present)
				continue;

			if (driveID == gBootDriveID)
				continue;

			BlockHandle *drive = new(nothrow) BlockHandle(driveID);
			if (drive->InitCheck() != B_OK) {
				dprintf("could not add drive %u\n", driveID);
				delete drive;
				continue;
			}

			devicesList->Add(drive);
			driveCount++;

			if (drive->FillIdentifier() != B_OK)
				identifierMissing = true;
		}
	}
	dprintf("number of drives: %d\n", driveCount);

	if (identifierMissing) {
		// we cannot distinguish between all drives by identifier, we need
		// compute checksums for them
		find_unique_check_sums(devicesList);
	}

	sBlockDevicesAdded = true;
	return B_OK; 
}


//	#pragma mark -


BlockHandle::BlockHandle(int handle)
	: Handle(handle)
	, fSize(0LL)
	, fBlockSize(0)
	, fHasParameters(false)
	  
{
	TRACE(("BlockHandle::%s(): drive ID %u\n", __FUNCTION__, fHandle));
}


BlockHandle::~BlockHandle()
{
}


ssize_t 
BlockHandle::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	ssize_t ret;
	uint32 offset = pos % fBlockSize;
	pos /= fBlockSize;
	TRACE(("BlockHandle::%s: (%d) %Ld, %d\n", __FUNCTION__, fHandle, pos, bufferSize));

	uint32 blocksLeft = (bufferSize + offset + fBlockSize - 1) / fBlockSize;
	int32 totalBytesRead = 0;

	//TRACE(("BIOS reads %lu bytes from %Ld (offset = %lu)\n",
	//	blocksLeft * fBlockSize, pos * fBlockSize, offset));

	// read partial block
	if (offset) {
		ret = ReadBlocks(gScratchBuffer, pos, 1);
		if (ret < 0)
			return ret;
		totalBytesRead += fBlockSize - offset;
		memcpy(buffer, gScratchBuffer + offset, totalBytesRead);
		
	}

	uint32 scratchSize = SCRATCH_SIZE / fBlockSize;

	while (blocksLeft > 0) {
		uint32 blocksRead = blocksLeft;
		if (blocksRead > scratchSize)
			blocksRead = scratchSize;

		ret = ReadBlocks(gScratchBuffer, pos, blocksRead);
		if (ret < 0)
			return ret;

		uint32 bytesRead = fBlockSize * blocksRead - offset;
		// copy no more than bufferSize bytes
		if (bytesRead > bufferSize)
			bytesRead = bufferSize;

		memcpy(buffer, (void *)(gScratchBuffer + offset), bytesRead);
		pos += blocksRead;
		offset = 0;
		blocksLeft -= blocksRead;
		bufferSize -= bytesRead;
		buffer = (void *)((addr_t)buffer + bytesRead);
		totalBytesRead += bytesRead;
	}

	//TRACE(("BlockHandle::%s: %d bytes read\n", __FUNCTION__, totalBytesRead));
	return totalBytesRead;
}


ssize_t 
BlockHandle::WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize)
{
	// we don't have to know how to write
	return B_NOT_ALLOWED;
}


ssize_t
BlockHandle::ReadBlocks(void *buffer, off_t first, int32 count)
{
	return B_NOT_ALLOWED;
}


status_t
BlockHandle::FillIdentifier()
{
	return B_NOT_ALLOWED;
}

//	#pragma mark -

/*
 * BIOS based disk access.
 * Only for fallback from missing XHDI.
 * XXX: This is broken!
 * XXX: check for physical drives in PUN_INFO
 * XXX: at least try to use MetaDOS calls instead.
 */


FloppyDrive::FloppyDrive(int handle)
	: BlockHandle(handle)
{
	TRACE(("FloppyDrive::%s(%d)\n", __FUNCTION__, fHandle));

	/* first check if the drive exists */
	/* note floppy B can be reported present anyway... */
	uint32 map = Drvmap();
	if (!(map & (1 << fHandle))) {
		fSize = 0LL;
		return;
	}
	//XXX: check size

	if (get_drive_parameters(fHandle, &fParameters) != B_OK) {
		dprintf("getting drive parameters for: %u failed!\n", fHandle);
		return;
	}
	// XXX: probe! this is a std 1.44kB floppy
	fBlockSize = 512;
	fParameters.sectors = 1440 * 1024 / 512;
	fParameters.sectors_per_track = 18;
	fParameters.heads = 2;
	fSize = fParameters.sectors * fBlockSize;
	fHasParameters = false;
/*
	parameters->sectors_per_track = 9;
	parameters->cylinders = (1440/2) / (9*2);
	parameters->heads = 2;
	parameters->sectors = parameters->cylinders * parameters->heads
	* parameters->sectors_per_track;
	*/
#if 0
	if (get_ext_drive_parameters(driveID, &fParameters) != B_OK) {
		// old style CHS support

		if (get_drive_parameters(driveID, &fParameters) != B_OK) {
			dprintf("getting drive parameters for: %u failed!\n", fDriveID);
			return;
		}

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
#endif
}


FloppyDrive::~FloppyDrive()
{
}


status_t
FloppyDrive::FillIdentifier()
{
	TRACE(("FloppyDrive::%s: (%d)\n", __FUNCTION__, fHandle));
#if 0
	if (HasParameters()) {
		// try all drive_parameters versions, beginning from the most informative

#if 0
		if (fill_disk_identifier_v3(fIdentifier, fParameters) == B_OK)
			return B_OK;

		if (fill_disk_identifier_v2(fIdentifier, fParameters) == B_OK)
			return B_OK;
#else
		// TODO: the above version is the correct one - it's currently
		//		disabled, as the kernel boot code only supports the
		//		UNKNOWN_BUS/UNKNOWN_DEVICE way to find the correct boot
		//		device.
		if (fill_disk_identifier_v3(fIdentifier, fParameters) != B_OK)
			fill_disk_identifier_v2(fIdentifier, fParameters);

#endif

		// no interesting information, we have to fall back to the default
		// unknown interface/device type identifier
	}

	fIdentifier.bus_type = UNKNOWN_BUS;
	fIdentifier.device_type = UNKNOWN_DEVICE;
	fIdentifier.device.unknown.size = Size();

	for (int32 i = 0; i < NUM_DISK_CHECK_SUMS; i++) {
		fIdentifier.device.unknown.check_sums[i].offset = -1;
		fIdentifier.device.unknown.check_sums[i].sum = 0;
	}
#endif

	return B_ERROR;
}


static void
hexdump(uint8 *buf, uint32 offset)
{
// hexdump
	
	for (int i = 0; i < 512; i++) {
		if ((i % 16) == 0)
			TRACE(("%08lx ", offset+i));
		if ((i % 16) == 8)
		TRACE((" "));
		TRACE((" %02x", buf[i]));

		if ((i % 16) == 15)
			TRACE(("\n"));
	}
}


ssize_t
FloppyDrive::ReadBlocks(void *buffer, off_t first, int32 count)
{
	int sectorsPerBlocks = (fBlockSize / 512);
	int sectorsPerTrack = fParameters.sectors_per_track;
	int heads = fParameters.heads;
	int32 ret;
	//TRACE(("FloppyDrive::%s(%Ld,%ld) (%d)\n", __FUNCTION__, first, count, fHandle));
	// force single sector reads to avoid crossing track boundaries
	for (int i = 0; i < count; i++) {
		uint8 *buf = (uint8 *)buffer;
		buf += i * fBlockSize;
		
		int16 track, side, sect;
		sect = (first + i) * sectorsPerBlocks;
		track = sect / (sectorsPerTrack * heads);
		side = (sect / sectorsPerTrack) % heads;
		sect %= sectorsPerTrack;
		sect++; // 1-based
	
		
		/*
		  TRACE(("FloppyDrive::%s: THS: %d %d %d\n",
		  __FUNCTION__, track, side, sect));
		*/
		ret = Floprd(buf, 0L, fHandle, sect, track, side, 1);
		if (ret < 0)
			return toserror(ret);
		//if (first >= 1440)
		//hexdump(buf, (uint32)(first+i)*512);
	}

	return count;
}


//	#pragma mark -

/*
 * BIOS based disk access.
 * Only for fallback from missing XHDI.
 * XXX: This is broken!
 * XXX: check for physical drives in PUN_INFO
 * XXX: at least try to use MetaDOS calls instead.
 */


BIOSDrive::BIOSDrive(int handle)
	: BlockHandle(handle)
{
	TRACE(("BIOSDrive::%s(%d)\n", __FUNCTION__, fHandle));

	/* first check if the drive exists */
	/* note floppy B can be reported present anyway... */
	uint32 map = Drvmap();
	if (!(map & (1 << fHandle))) {
		fSize = 0LL;
		return;
	}
	//XXX: check size

	if (get_drive_parameters(fHandle, &fParameters) != B_OK) {
		dprintf("getting drive parameters for: %u failed!\n", fHandle);
		return;
	}
	fBlockSize = 512;
	fSize = fParameters.sectors * fBlockSize;
	fHasParameters = false;

#if 0
	if (get_ext_drive_parameters(driveID, &fParameters) != B_OK) {
		// old style CHS support

		if (get_drive_parameters(driveID, &fParameters) != B_OK) {
			dprintf("getting drive parameters for: %u failed!\n", fDriveID);
			return;
		}

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
#endif
}


BIOSDrive::~BIOSDrive()
{
}


status_t
BIOSDrive::FillIdentifier()
{
	TRACE(("BIOSDrive::%s: (%d)\n", __FUNCTION__, fHandle));
#if 0
	if (HasParameters()) {
		// try all drive_parameters versions, beginning from the most informative

#if 0
		if (fill_disk_identifier_v3(fIdentifier, fParameters) == B_OK)
			return B_OK;

		if (fill_disk_identifier_v2(fIdentifier, fParameters) == B_OK)
			return B_OK;
#else
		// TODO: the above version is the correct one - it's currently
		//		disabled, as the kernel boot code only supports the
		//		UNKNOWN_BUS/UNKNOWN_DEVICE way to find the correct boot
		//		device.
		if (fill_disk_identifier_v3(fIdentifier, fParameters) != B_OK)
			fill_disk_identifier_v2(fIdentifier, fParameters);

#endif

		// no interesting information, we have to fall back to the default
		// unknown interface/device type identifier
	}

	fIdentifier.bus_type = UNKNOWN_BUS;
	fIdentifier.device_type = UNKNOWN_DEVICE;
	fIdentifier.device.unknown.size = Size();

	for (int32 i = 0; i < NUM_DISK_CHECK_SUMS; i++) {
		fIdentifier.device.unknown.check_sums[i].offset = -1;
		fIdentifier.device.unknown.check_sums[i].sum = 0;
	}
#endif

	return B_ERROR;
}


ssize_t
BIOSDrive::ReadBlocks(void *buffer, off_t first, int32 count)
{
	int sectorsPerBlocks = (fBlockSize / 256);
	int32 ret;
	TRACE(("BIOSDrive::%s(%Ld,%ld) (%d)\n", __FUNCTION__, first, count, fHandle));
	// XXX: check for AHDI 3.0 before using long recno!!!
	ret = Rwabs(RW_READ | RW_NOTRANSLATE, buffer, sectorsPerBlocks, -1, fHandle, first * sectorsPerBlocks);
	if (ret < 0)
		return toserror(ret);
	return ret;
}


//	#pragma mark -

/*
 * XHDI based devices
 */


XHDIDrive::XHDIDrive(int handle, uint16 major, uint16 minor)
	: BlockHandle(handle)
{
	/* first check if the drive exists */
	int32 err;
	uint32 devflags;
	uint32 blocks;
	uint32 blocksize;
	char product[33];

	fMajor = major;
	fMinor = minor;
	TRACE(("XHDIDrive::%s(%d, %d, %d)\n", __FUNCTION__, handle, fMajor, fMinor));

	product[32] = '\0';
	err = XHInqTarget(major, minor, &fBlockSize, &devflags, product);
	if (err < 0)
		return;
	//XXX: check size
	err = XHGetCapacity(major, minor, &blocks, &blocksize);
	if (err < 0)
		return;
	
	if (fBlockSize == 0)
		fBlockSize = 512;
	
	fSize = blocks * fBlockSize;
	fHasParameters = false;
#if 0
	if (get_drive_parameters(fHandle, &fParameters) != B_OK) {
		dprintf("getting drive parameters for: %u failed!\n", fHandle);
		return;
	}
	fBlockSize = 512;
	fSize = fParameters.sectors * fBlockSize;
	fHasParameters = false;
#endif
#if 0
	if (get_ext_drive_parameters(driveID, &fParameters) != B_OK) {
		// old style CHS support

		if (get_drive_parameters(driveID, &fParameters) != B_OK) {
			dprintf("getting drive parameters for: %u failed!\n", fDriveID);
			return;
		}

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
#endif
}


XHDIDrive::~XHDIDrive()
{
}


status_t
XHDIDrive::FillIdentifier()
{
	TRACE(("XHDIDrive::%s: (%d,%d)\n", __FUNCTION__, fMajor, fMinor));

	fIdentifier.bus_type = UNKNOWN_BUS;
	fIdentifier.device_type = UNKNOWN_DEVICE;
	fIdentifier.device.unknown.size = Size();
#if 0
	// cf. http://toshyp.atari.org/010008.htm#XHDI-Terminologie
	if (fMajor >= 8 && fMajor <= 15) { // scsi
		fIdentifier.device_type = SCSI_DEVICE;
		fIdentifier.device.scsi.logical_unit = fMinor;
		//XXX: where am I supposed to put the ID ???
	}
#endif

	for (int32 i = 0; i < NUM_DISK_CHECK_SUMS; i++) {
		fIdentifier.device.unknown.check_sums[i].offset = -1;
		fIdentifier.device.unknown.check_sums[i].sum = 0;
	}

	return B_ERROR;
}


ssize_t
XHDIDrive::ReadBlocks(void *buffer, off_t first, int32 count)
{
	int sectorsPerBlock = (fBlockSize / 256);
	int32 ret;
	uint16 flags = RW_READ;
	TRACE(("XHDIDrive::%s(%Ld, %d) (%d,%d)\n", __FUNCTION__, first, count, fMajor, fMinor));
	ret = XHReadWrite(fMajor, fMinor, flags, (uint32)first, (uint16)count, buffer);
	if (ret < 0)
		return xhdierror(ret);
	//TRACE(("XHReadWrite: %ld\n", ret));
	/*
	uint8 *b = (uint8 *)buffer;
	int i = 0;
	for (i = 0; i < 512; i+=16) {
		TRACE(("[%8Ld+%3ld] %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", 
			first, i, b[i], b[i+1], b[i+2], b[i+3], b[i+4], b[i+5], b[i+6], b[i+7], 
			b[i+8], b[i+9], b[i+10], b[i+11], b[i+12], b[i+13], b[i+14], b[i+15]));
		//break;
	}
	*/
	return ret;
}




//	#pragma mark -


status_t 
platform_add_boot_device(struct stage2_args *args, NodeList *devicesList)
{
	TRACE(("boot drive ID: %x API: %d\n", gBootDriveID, gBootDriveAPI));
	init_xhdi();

	//XXX: FIXME
	//BlockHandle *drive = new(nothrow) BlockHandle(gBootDriveID);
	BlockHandle *drive;
	switch (gBootDriveAPI) {
		case ATARI_BOOT_DRVAPI_FLOPPY:
			drive = new(nothrow) FloppyDrive(gBootDriveID);
			break;
			/*
		case ATARI_BOOT_DRVAPI_XBIOS:
			drive = new(nothrow) DMADrive(gBootDriveID);
			break;
			*/
		case ATARI_BOOT_DRVAPI_XHDI:
			drive = new(nothrow) XHDIDrive(gBootDriveID, gBootDriveID, 0);
			break;
		case ATARI_BOOT_DRVAPI_UNKNOWN:
		{
			// we don't know yet, try to ask ARAnyM via NatFeat
			int id = nat_feat_get_bootdrive();
			if (id > -1) {
				gBootDriveID = id;
				dprintf("nat_fead_get_bootdrive() = %d\n", id);
				// XXX: which API does it refer to ??? id = letter - 'a'
				drive = new(nothrow) XHDIDrive(gBootDriveID, gBootDriveID, 0);
				break;
			}
		}
		default:
			dprintf("unknown boot drive API %d\n", gBootDriveAPI);
			return B_ERROR;
			drive = new(nothrow) BIOSDrive(gBootDriveID);
	}

	if (drive == NULL || drive->InitCheck() != B_OK) {
		dprintf("no boot drive!\n");
		return B_ERROR;
	}

	devicesList->Add(drive);

	if (drive->FillIdentifier() != B_OK) {
		// We need to add all block devices to give the kernel the possibility
		// to find the right boot volume
		add_block_devices(devicesList, true);
	}

	TRACE(("boot drive size: %Ld bytes\n", drive->Size()));
	gBootVolume.SetBool(BOOT_VOLUME_BOOTED_FROM_IMAGE, gBootedFromImage);

	return B_OK;
}


status_t
platform_get_boot_partition(struct stage2_args *args, Node *bootDevice,
	NodeList *list, boot::Partition **_partition)
{
	BlockHandle *drive = static_cast<BlockHandle *>(bootDevice);
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
	init_xhdi();
	return add_block_devices(devicesList, false);
}


status_t 
platform_register_boot_device(Node *device)
{
	BlockHandle *drive = (BlockHandle *)device;

#if 0
	check_cd_boot(drive);
#endif

	gBootVolume.SetInt64("boot drive number", drive->DriveID());
	gBootVolume.SetData(BOOT_VOLUME_DISK_IDENTIFIER, B_RAW_TYPE,
		&drive->Identifier(), sizeof(disk_identifier));

	return B_OK;
}


void
platform_cleanup_devices()
{
}
