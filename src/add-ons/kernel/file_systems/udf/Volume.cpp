//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//  Mad props to Axel Dörfler and his BFS implementation, from which
//  this UDF implementation draws much influence (and a little code :-P).
//----------------------------------------------------------------------
#include "Volume.h"

#include "Block.h"

using namespace UDF;

//----------------------------------------------------------------------
// Volume 
//----------------------------------------------------------------------

/*! \brief Creates an unmounted volume with the given id.
*/
Volume::Volume(nspace_id id)
	: fID(id)
	, fDevice(0)
	, fReadOnly(false)
	, fStartAddress(0)
	, fBlockSize(0)
	, fInitStatus(B_UNINITIALIZED)
{
}

status_t
Volume::Identify(int device, off_t base)
{
	return B_ERROR;	
}

/*! \brief Attempts to mount the given device.
*/
status_t
Volume::Mount(const char *deviceName, off_t volumeStart, off_t volumeLength,
              uint32 flags, uint32 blockSize)
{
	DEBUG_INIT_ETC(CF_PUBLIC | CF_VOLUME_OPS, "Volume",
		("devName = `%s'", deviceName));
	if (!deviceName)
		RETURN_ERROR(B_BAD_VALUE);
	if (_InitStatus() == B_INITIALIZED)
		RETURN_ERROR(B_BUSY);
			// Already mounted, thank you for asking
		
	fReadOnly = flags & B_READ_ONLY;
	fStartAddress = volumeStart;
	fLength = volumeLength;	
	fBlockSize = blockSize;
	
	// Open the device, trying read only if readwrite fails
	fDevice = open(deviceName, fReadOnly ? O_RDONLY : O_RDWR);
	if (fDevice < B_OK && !fReadOnly) {
		fReadOnly = true;
		fDevice = open(deviceName, O_RDONLY);
	}		
	if (fDevice < B_OK)
		RETURN_ERROR(fDevice);
		
	// If the device is actually a normal file, try to disable the cache
	// for the file in the parent filesystem
	struct stat stat;
	status_t err = fstat(fDevice, &stat) < 0 ? B_ERROR : B_OK;
	if (!err) {
		if (stat.st_mode & S_IFREG && ioctl(fDevice, IOCTL_FILE_UNCACHED_IO, NULL) < 0) {
			// Apparently it's a bad thing if you can't disable the file
			// cache for a non-device disk image you're trying to mount...
			DIE(("Unable to disable cache of underlying file system. "
			     "I hear that's bad. :-(\n"));
		}
		// So far so good. The device is ready to be accessed now.
		fInitStatus = B_DEVICE_INITIALIZED;
	}
	
	// Now identify the volume
	if (!err)
		err = _Identify();

	RETURN(B_ERROR);
}

/*!	\brief Walks through the volume recognition and descriptor sequences,
	gathering volume description info as it goes.

	Note that the 512 avdp location is, technically speaking, only valid on
	unlosed CD-R media in the absense of an avdp at 256. For now I'm not
	bothering with such silly details, and instead am just checking for it
	last.
*/
status_t
Volume::_Identify()
{
	DEBUG_INIT(CF_PRIVATE | CF_VOLUME_OPS, "Volume");
	
	status_t err = _InitStatus() == B_DEVICE_INITIALIZED ? B_OK : B_BAD_VALUE;

	// Check for a valid volume recognition sequence
	if (!err)
		err = _WalkVolumeRecognitionSequence();
	
	// Now hunt down a volume descriptor sequence from one of
	// the anchor volume pointers (if there are any).
	if (!err) {
		const uint8 avds_location_count = 4;
		const off_t avds_locations[avds_location_count] = { 256,
		                                                    Length()-256,
		                                                    Length(),
		                                                    512,
		                                                  };
		bool found_vds = false;
		                                                  
		for (int32 i = 0; i < avds_location_count; i++) {
			off_t block = avds_locations[i];
			off_t address = AddressForRelativeBlock(block);
			Block<anchor_volume_descriptor_pointer> anchor(BlockSize());
			status_t anchorErr = anchor.InitCheck();
			if (!anchorErr) {
				ssize_t bytesRead = read_pos(fDevice, address, anchor.Data(), BlockSize());
				anchorErr = bytesRead == (ssize_t)BlockSize() ? B_OK : B_IO_ERROR;
				if (anchorErr) {
					PRINT(("block %lld: read_pos(pos:%lld, len:%ld) failed with error 0x%lx\n",
					       block, address, BlockSize(), bytesRead));
				}
			}
			if (!anchorErr) {
				anchor.Data()->tag().dump();
				anchorErr = anchor.Data()->tag().init_check(block);
				if (anchorErr) {
					PRINT(("block %lld: invalid anchor\n", block));
				} else {
					PRINT(("block %lld: valid anchor\n", block));
				}
			}
			if (!anchorErr) {
				// Found an avds, so try the main sequence first, then
				// the reserve sequence if the main one fails.
				anchorErr = _WalkVolumeDescriptorSequence(anchor.Data()->main_vds());
				if (anchorErr)
					anchorErr = _WalkVolumeDescriptorSequence(anchor.Data()->reserve_vds());		
					
				
			}
			if (!anchorErr) {
				PRINT(("block %lld: found valid vds\n", avds_locations[i]));
				found_vds = true;
				break;
			} //else {
				// Both failed, so loop around and try another avds
//				PRINT(("block %lld: vds search failed\n", avds_locations[i]));
//			}
		}
		err = found_vds ? B_OK : B_ERROR;		
	}		
	
	RETURN(err);
}

/*! \brief Walks the iso9660/ecma-167 volume recognition sequence, returning
	\c B_OK if the presence of a UDF filesystem on this volume is likely.
	
	\return \c B_OK: An ECMA-167 vsd was found, or at least one extended area
	                 vsd was found and no ECMA-168 vsds were found.
	\return "error code": Only iso9660 vsds were found, an ECMA-168 vsd was
	                      found (but no ECMA-167 vsd), or an error occurred.
*/
status_t
Volume::_WalkVolumeRecognitionSequence()
{
	DEBUG_INIT(CF_PRIVATE | CF_VOLUME_OPS, "Volume");
	// vrs starts at block 16. Each volume structure descriptor (vsd)
	// should be one block long. We're expecting to find 0 or more iso9660
	// vsd's followed by some ECMA-167 vsd's.
	Block<volume_structure_descriptor_header> descriptor(BlockSize());
	status_t err = descriptor.InitCheck();
	if (!err) {
		bool foundISO = false;
		bool foundExtended = false;
		bool foundECMA167 = false;
		bool foundECMA168 = false;
		bool foundBoot = false;
		for (uint32 block = 16; true; block++) {
	    	PRINT(("block %ld: ", block))
			off_t address = AddressForRelativeBlock(block);
			ssize_t bytesRead = read_pos(fDevice, address, descriptor.Data(), BlockSize());
			if (bytesRead == (ssize_t)BlockSize())
		    {
				if (descriptor.Data()->id_matches(kVSDID_ISO)) {
					SIMPLE_PRINT(("found ISO9660 descriptor\n"));
					foundISO = true;
				} else if (descriptor.Data()->id_matches(kVSDID_BEA)) {
					SIMPLE_PRINT(("found BEA descriptor\n"));
					foundExtended = true;
				} else if (descriptor.Data()->id_matches(kVSDID_TEA)) {
					SIMPLE_PRINT(("found TEA descriptor\n"));
					foundExtended = true;
				} else if (descriptor.Data()->id_matches(kVSDID_ECMA167_2)) {
					SIMPLE_PRINT(("found ECMA-167 rev 2 descriptor\n"));
					foundECMA167 = true;
				} else if (descriptor.Data()->id_matches(kVSDID_ECMA167_3)) {
					SIMPLE_PRINT(("found ECMA-167 rev 3 descriptor\n"));
					foundECMA167 = true;
				} else if (descriptor.Data()->id_matches(kVSDID_BOOT)) {
					SIMPLE_PRINT(("found boot descriptor\n"));
					foundBoot = true;
				} else if (descriptor.Data()->id_matches(kVSDID_ECMA168)) {
					SIMPLE_PRINT(("found ECMA-168 descriptor\n"));
					foundECMA168 = true;
				} else {
					SIMPLE_PRINT(("found invalid descriptor, id = `%.5s'\n", descriptor.Data()->id));
					break;
				}
			} else {
				SIMPLE_PRINT(("read_pos(pos:%lld, len:%ld) failed with: 0x%lx\n", address,
				        BlockSize(), bytesRead));
				break;
			}
		}
		
		// If we find an ECMA-167 descriptor, OR if we find a beginning
		// or terminating extended area descriptor with NO ECMA-168
		// descriptors, we return B_OK to signal that we should go
		// looking for valid anchors.
		err = foundECMA167 || (foundExtended && !foundECMA168) ? B_OK : B_ERROR;	
	}
	
	RETURN(err);
}

status_t
Volume::_WalkVolumeDescriptorSequence(extent_address extent)
{
	DEBUG_INIT_ETC(CF_PRIVATE | CF_VOLUME_OPS, "Volume", ("loc:%ld, len:%ld",
	           extent.location(), extent.length()));
	uint32 count = extent.length()/BlockSize();
	for (uint32 i = 0; i < count; i++)
	{
		off_t block = extent.location()+i;
		off_t address = AddressForRelativeBlock(block);
		Block<descriptor_tag> tag(BlockSize());
		status_t err = tag.InitCheck();
		if (!err) {
			ssize_t bytesRead = read_pos(fDevice, address, tag.Data(), BlockSize());
			err = bytesRead == (ssize_t)BlockSize() ? B_OK : B_IO_ERROR;
			if (err) {
				PRINT(("block %lld: read_pos(pos:%lld, len:%ld) failed with error 0x%lx\n",
				       block, address, BlockSize(), bytesRead));
			}
		}
		if (!err) {
			PRINT(("descriptor #%ld (block %lld):\n", i, block));
			if (tag.Data()->id() == 1) {
				primary_vd *vd = (primary_vd*)tag.Data();
				vd->dump();
			} else {
				tag.Data()->dump();
			}
		}
	}
	
	RETURN(B_ERROR);
}

