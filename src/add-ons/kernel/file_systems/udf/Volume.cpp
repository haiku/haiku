//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//  Mad props to Axel Dörfler and his BFS implementation, from which
//  this UDF implementation draws much influence (and a little code :-P).
//----------------------------------------------------------------------
#include "Volume.h"

#include "Icb.h"
#include "MemoryChunk.h"

using namespace Udf;

//----------------------------------------------------------------------
// Volume 
//----------------------------------------------------------------------

/*! \brief Creates an unmounted volume with the given id.
*/
Volume::Volume(nspace_id id)
	: fId(id)
	, fDevice(0)
	, fReadOnly(false)
	, fOffset(0)
	, fBlockSize(0)
	, fBlockShift(0)
	, fInitStatus(B_UNINITIALIZED)
#if (!DRIVE_SETUP_ADDON)
	, fRootIcb(NULL)
#endif
{
}

status_t
Volume::Identify(int device, off_t offset, off_t length, uint32 blockSize, char *volumeName)
{
	DEBUG_INIT_ETC(CF_PUBLIC | CF_VOLUME_OPS, "static Volume",
		("device: %d, offset: %Ld, volumeName: %p", device, offset, volumeName));
	if (!volumeName)
		RETURN(B_BAD_VALUE);
		
//	FILE *file = fopen("/boot/home/Desktop/outputIdentify.txt", "w+");

	Volume volume(0);
	status_t err = volume._Init(device, offset, length, blockSize);
//	fprintf(file, "error = 0x%lx, `%s'\n", err, strerror(err));
//	fflush(file);
	if (!err)
		err = volume._Identify();
//	fprintf(file, "error = 0x%lx, `%s'\n", err, strerror(err));
//	fflush(file);
	if (!err) 
		strcpy(volumeName, volume.Name());
//	fprintf(file, "error = 0x%lx, `%s'\n", err, strerror(err));
//	fflush(file);

//	fclose(file);		
	RETURN(err);	
}

/*! \brief Attempts to mount the given device.

	\param volumeStart The block on the given device whereat the volume begins.
	\param volumeLength The block length of the volume on the given device.
*/
status_t
Volume::Mount(const char *deviceName, off_t volumeStart, off_t volumeLength,
              uint32 flags, uint32 blockSize)
{
	DEBUG_INIT_ETC(CF_PUBLIC | CF_VOLUME_OPS, "Volume",
		("deviceName: `%s', offset: %Ld, length %Ld", deviceName, volumeStart, volumeLength));
	if (!deviceName)
		RETURN(B_BAD_VALUE);
	if (_InitStatus() == B_INITIALIZED)
		RETURN(B_BUSY);
			// Already mounted, thank you for asking

	// Open the device read only
	int device = open(deviceName, O_RDONLY);
	if (device < B_OK)
		RETURN(device);

	status_t err = _Init(device, volumeStart, volumeLength, blockSize);
	if (!err) 
		err = _Identify();
	if (!err)
		err = _Mount();

	if (err)
		fInitStatus = B_UNINITIALIZED;
	
	RETURN(err);
}
 
const char*
Volume::Name() const {
	return fName.String();
}

off_t
Volume::MapAddress(udf_extent_address address)
{
	return address.location() * BlockSize();	
}


/*! \brief Maps the given \c udf_long_address to an absolute block address.
*/
status_t
Volume::MapBlock(udf_long_address address, off_t *mappedBlock)
{
	DEBUG_INIT_ETC(CF_PRIVATE | CF_HIGH_VOLUME, "Volume", ("long_address(block: %ld, partition: %d), %p",
		address.block(), address.partition(), mappedBlock));
	status_t err = mappedBlock ? B_OK : B_BAD_VALUE;
	if (!err)
		err = _InitStatus() >= B_IDENTIFIED ? B_OK : B_NO_INIT;
	if (!err) {
		const udf_partition_descriptor* partition = fPartitionMap.Find(address.partition());
		err = partition ? B_OK : B_BAD_ADDRESS;
		if (!err) {
			*mappedBlock = partition->start() + address.block();	
		}
		if (!err) {
			PRINT(("mapped to block %Ld\n", *mappedBlock));
			snooze(5000);
		}
	}
	RETURN(err);
}

/*! \brief Maps the given \c udf_long_address to an absolute byte address.
*/
status_t
Volume::MapAddress(udf_long_address address, off_t *mappedAddress)
{
	DEBUG_INIT_ETC(CF_PRIVATE | CF_HIGH_VOLUME, "Volume", ("long_address(block: %ld, partition: %d), %p",
		address.block(), address.partition(), mappedAddress));
	status_t err = MapBlock(address, mappedAddress);
	if (!err)
		*mappedAddress = *mappedAddress * BlockSize();
	if (!err) {
		PRINT(("mapped to address %Ld\n", *mappedAddress));
	}
	RETURN_ERROR(err);
}

off_t
Volume::MapAddress(udf_short_address address)
{
	return 0;
}

/*status_t
Volume::_Read(udf_extent_address address, ssize_t length, void *data)
{
	DEBUG_INIT(CF_PRIVATE | CF_HIGH_VOLUME, "Volume");
	off_t mappedAddress = MapAddress(address);
	status_t err = data ? B_OK : B_BAD_VALUE;
	if (!err) {
		ssize_t bytesRead = read_pos(fDevice, mappedAddress, data, BlockSize());
		if (bytesRead != (ssize_t)BlockSize()) {
			err = B_IO_ERROR;
			PRINT(("read_pos(pos:%Ld, len:%ld) failed with: 0x%lx\n", mappedAddress,
			       length, bytesRead));
		}
	}
	RETURN(err);
}
*/

/*template <class AddressType>
status_t
Volume::_Read(AddressType address, ssize_t length, void *data)
{
	DEBUG_INIT(CF_PRIVATE | CF_HIGH_VOLUME, "Volume");
	off_t mappedAddress;
	status_t err = data ? B_OK : B_BAD_VALUE;
	if (!err)
		err = MapAddress(address, &mappedAddress);
	if (!err) {
		ssize_t bytesRead = read_pos(fDevice, mappedAddress, data, BlockSize());
		if (bytesRead != (ssize_t)BlockSize()) {
			err = B_IO_ERROR;
			PRINT(("read_pos(pos:%Ld, len:%ld) failed with: 0x%lx\n", mappedAddress,
			       length, bytesRead));
		}
	}
	RETURN(err);
}
*/
status_t
Volume::_Init(int device, off_t offset, off_t length, int blockSize)
{
	DEBUG_INIT(CF_PRIVATE | CF_HIGH_VOLUME, "Volume");
	if (_InitStatus() == B_INITIALIZED)
		RETURN_ERROR(B_BUSY);

	// Check the block size
	uint32 bitCount = 0;
	for (int i = 0; i < 32; i++) {
		// Zero out all bits except bit i
		uint32 block = blockSize & (uint32(1) << i);
		if (block) {
			if (++bitCount > 1) {
				PRINT(("Block size must be a power of two! (blockSize = %ld)\n", blockSize));
				RETURN(B_BAD_VALUE);
			} else {
				fBlockShift = i;
				PRINT(("BlockShift() = %ld\n", BlockShift()));
			}			
		}
	}

	fDevice = device;	
	fReadOnly = true;
	fOffset = offset;
	fLength = length;	
	fBlockSize = blockSize;
	
	status_t err = B_OK;
	
#if (!DRIVE_SETUP_ADDON)
	// If the device is actually a normal file, try to disable the cache
	// for the file in the parent filesystem
	struct stat stat;
	err = fstat(fDevice, &stat) < 0 ? B_ERROR : B_OK;
	if (!err) {
		if (stat.st_mode & S_IFREG && ioctl(fDevice, IOCTL_FILE_UNCACHED_IO, NULL) < 0) {
			// Apparently it's a bad thing if you can't disable the file
			// cache for a non-device disk image you're trying to mount...
			DIE(("Unable to disable cache of underlying file system. "
			     "I hear that's bad. :-(\n"));
		}
	}
#endif
	
	fInitStatus = err < B_OK ? B_UNINITIALIZED : B_DEVICE_INITIALIZED;
	
	RETURN(err);
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
	if (!err)
		err = _WalkAnchorVolumeDescriptorSequences();
		
	// Set the volume name
	if (!err) {
//		FILE *file = fopen("/boot/home/Desktop/vdoutput.txt", "w+");
//		fprint

		fName.SetTo(fLogicalVD.logical_volume_identifier()); 
	}
		
	fInitStatus = err < B_OK ? B_UNINITIALIZED : B_IDENTIFIED;

	RETURN(err);
}

status_t
Volume::_Mount()
{
	DEBUG_INIT(CF_PRIVATE | CF_VOLUME_OPS, "Volume");
	
	status_t err = _InitStatus() == B_IDENTIFIED ? B_OK : B_BAD_VALUE;

#if (!DRIVE_SETUP_ADDON)
	if (!err)
		err = init_cache_for_device(Device(), Length());

	// At this point we've found a valid set of volume descriptors. We
	// now need to investigate the file set descriptor pointed to by
	// the logical volume descriptor
	if (!err) 
		err = _InitFileSetDescriptor();
	
	if (!err) {
		// Success, create a vnode for the root
		err = new_vnode(Id(), RootIcb()->Id(), (void*)RootIcb());
		if (err) {
			PRINT(("Error create vnode for root icb! error = 0x%lx, `%s'\n",
			       err, strerror(err)));
		}
	}
	
	fInitStatus = err < B_OK ? B_UNINITIALIZED : B_LOGICAL_VOLUME_INITIALIZED;
#endif

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
	MemoryChunk chunk(BlockSize());
	status_t err = chunk.InitCheck();
	if (!err) {
		bool foundISO = false;
		bool foundExtended = false;
		bool foundECMA167 = false;
		bool foundECMA168 = false;
		bool foundBoot = false;
		for (uint32 block = 16; true; block++) {
	    	PRINT(("block %ld: ", block))
			off_t address = AddressForRelativeBlock(block);
			ssize_t bytesRead = read_pos(fDevice, address, chunk.Data(), BlockSize());
			if (bytesRead == (ssize_t)BlockSize())
		    {
		    	udf_volume_structure_descriptor_header* descriptor =
		    	  reinterpret_cast<udf_volume_structure_descriptor_header*>(chunk.Data());
				if (descriptor->id_matches(kVSDID_ISO)) {
					SIMPLE_PRINT(("found ISO9660 descriptor\n"));
					foundISO = true;
				} else if (descriptor->id_matches(kVSDID_BEA)) {
					SIMPLE_PRINT(("found BEA descriptor\n"));
					foundExtended = true;
				} else if (descriptor->id_matches(kVSDID_TEA)) {
					SIMPLE_PRINT(("found TEA descriptor\n"));
					foundExtended = true;
				} else if (descriptor->id_matches(kVSDID_ECMA167_2)) {
					SIMPLE_PRINT(("found ECMA-167 rev 2 descriptor\n"));
					foundECMA167 = true;
				} else if (descriptor->id_matches(kVSDID_ECMA167_3)) {
					SIMPLE_PRINT(("found ECMA-167 rev 3 descriptor\n"));
					foundECMA167 = true;
				} else if (descriptor->id_matches(kVSDID_BOOT)) {
					SIMPLE_PRINT(("found boot descriptor\n"));
					foundBoot = true;
				} else if (descriptor->id_matches(kVSDID_ECMA168)) {
					SIMPLE_PRINT(("found ECMA-168 descriptor\n"));
					foundECMA168 = true;
				} else {
					SIMPLE_PRINT(("found invalid descriptor, id = `%.5s'\n", descriptor->id));
					break;
				}
			} else {
				SIMPLE_PRINT(("read_pos(pos:%Ld, len:%ld) failed with: 0x%lx\n", address,
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
Volume::_WalkAnchorVolumeDescriptorSequences()
{
	DEBUG_INIT(CF_PRIVATE | CF_VOLUME_OPS, "Volume");
		const uint8 avds_location_count = 4;
		const off_t avds_locations[avds_location_count] = {
															256,
		                                                    Length()-256,
		                                                    Length(),
		                                                    512,
		                                                  };
		bool found_vds = false;
		                                                  
		for (int32 i = 0; i < avds_location_count; i++) {
			off_t block = avds_locations[i];
			off_t address = AddressForRelativeBlock(block);
			MemoryChunk chunk(BlockSize());
			udf_anchor_descriptor *anchor = NULL;
			
			status_t anchorErr = chunk.InitCheck();
			if (!anchorErr) {
				ssize_t bytesRead = read_pos(fDevice, address, chunk.Data(), BlockSize());
				anchorErr = bytesRead == (ssize_t)BlockSize() ? B_OK : B_IO_ERROR;
				if (anchorErr) {
					PRINT(("block %Ld: read_pos(pos:%Ld, len:%ld) failed with error 0x%lx\n",
					       block, address, BlockSize(), bytesRead));
				}
			}			
			if (!anchorErr) {
				anchor = reinterpret_cast<udf_anchor_descriptor*>(chunk.Data());
				anchorErr = anchor->tag().init_check(block+Offset());
				if (anchorErr) {
					PRINT(("block %Ld: invalid anchor\n", block));
				} else {
					PRINT(("block %Ld: valid anchor\n", block));
				}
			}
			if (!anchorErr) {
				PRINT(("block %Ld: anchor:\n", block));
				PDUMP(anchor);
				// Found an avds, so try the main sequence first, then
				// the reserve sequence if the main one fails.
				anchorErr = _WalkVolumeDescriptorSequence(anchor->main_vds());
				if (anchorErr)
					anchorErr = _WalkVolumeDescriptorSequence(anchor->reserve_vds());		
					
				
			}
			if (!anchorErr) {
				PRINT(("block %Ld: found valid vds\n", avds_locations[i]));
				found_vds = true;
				break;
			} //else {
				// Both failed, so loop around and try another avds
//				PRINT(("block %Ld: vds search failed\n", avds_locations[i]));
//			}
		}
		status_t err = found_vds ? B_OK : B_ERROR;
		RETURN(err);
}

status_t
Volume::_WalkVolumeDescriptorSequence(udf_extent_address extent)
{
	DEBUG_INIT_ETC(CF_PRIVATE | CF_VOLUME_OPS, "Volume", ("loc:%ld, len:%ld",
	           extent.location(), extent.length()));
	uint32 count = extent.length()/BlockSize();
	
	bool foundLogicalVD = false;
	
	for (uint32 i = 0; i < count; i++)
	{
		off_t block = extent.location()+i;
		off_t address = block << BlockShift(); //AddressForRelativeBlock(block);
		MemoryChunk chunk(BlockSize());
		udf_tag *tag = NULL;

		PRINT(("descriptor #%ld (block %Ld):\n", i, block));

		status_t err = chunk.InitCheck();
		if (!err) {
			ssize_t bytesRead = read_pos(fDevice, address, chunk.Data(), BlockSize());
			err = bytesRead == (ssize_t)BlockSize() ? B_OK : B_IO_ERROR;
			if (err) {
				PRINT(("block %Ld: read_pos(pos:%Ld, len:%ld) failed with error 0x%lx\n",
				       block, address, BlockSize(), bytesRead));
			}
		}
		if (!err) {
			tag = reinterpret_cast<udf_tag*>(chunk.Data());
			err = tag->init_check(block);
		}
		if (!err) {
			// Now decide what type of descriptor we have
			switch (tag->id()) {
				case TAGID_UNDEFINED:
					break;
					
				case TAGID_PRIMARY_VOLUME_DESCRIPTOR:
				{
					udf_primary_descriptor *primary = reinterpret_cast<udf_primary_descriptor*>(tag);
					PDUMP(primary);				
					break;
				}
				
				case TAGID_ANCHOR_VOLUME_DESCRIPTOR_POINTER:
					break;
					
				case TAGID_VOLUME_DESCRIPTOR_POINTER:
					break;

				case TAGID_IMPLEMENTATION_USE_VOLUME_DESCRIPTOR:
				{
					udf_implementation_use_descriptor *imp_use = reinterpret_cast<udf_implementation_use_descriptor*>(tag);
					PDUMP(imp_use);				
					break;
				}

				case TAGID_PARTITION_DESCRIPTOR:
				{
					udf_partition_descriptor *partition = reinterpret_cast<udf_partition_descriptor*>(tag);
					PDUMP(partition);
					if (partition->tag().init_check(block) == B_OK) {
						const udf_partition_descriptor *current = fPartitionMap.Find(partition->partition_number());
						if (!current || current->vds_number() < partition->vds_number()) {
							PRINT(("adding partition #%d with vds_number %ld to partition map\n",
							       partition->partition_number(), partition->vds_number()));
							fPartitionMap.Add(partition);
						}
					}
					break;
				}
					
				case TAGID_LOGICAL_VOLUME_DESCRIPTOR:
				{
					udf_logical_descriptor *logical = reinterpret_cast<udf_logical_descriptor*>(tag);
					PDUMP(logical);
					if (foundLogicalVD) {
						// Keep the vd with the highest vds_number
						if (logical->vds_number() > fLogicalVD.vds_number())
							fLogicalVD = *(logical);
					} else {
						fLogicalVD = *(logical);
						foundLogicalVD = true;
					}
					break;
				}
					
				case TAGID_UNALLOCATED_SPACE_DESCRIPTOR:
				{
					udf_unallocated_space_descriptor *unallocated = reinterpret_cast<udf_unallocated_space_descriptor*>(tag);
					PDUMP(unallocated);				
					break;
				}
					
				case TAGID_TERMINATING_DESCRIPTOR:
				{
					udf_terminating_descriptor *terminating = reinterpret_cast<udf_terminating_descriptor*>(tag);
					PDUMP(terminating);				
					break;
				}
					
				case TAGID_LOGICAL_VOLUME_INTEGRITY_DESCRIPTOR:
					// Not found in this descriptor sequence
					break;

				default:
					break;			
			
			}
		}
	}
	
	status_t err = foundLogicalVD ? B_OK : B_ERROR;
	if (!err) {
		PRINT(("partition map:\n"));
		DUMP(fPartitionMap);
	}
	RETURN(err);
}

status_t
Volume::_InitFileSetDescriptor()
{
	DEBUG_INIT(CF_PRIVATE | CF_VOLUME_OPS, "Volume");
	MemoryChunk chunk(fLogicalVD.file_set_address().length());
	
	status_t err = chunk.InitCheck();

#if (!DRIVE_SETUP_ADDON)
	if (!err) {
//		err = Read(ad, fLogicalVD.file_set_address().length(), chunk.Data());
		err = Read(fLogicalVD.file_set_address(), fLogicalVD.file_set_address().length(), chunk.Data());
		if (!err) {
			udf_file_set_descriptor *fileSet = reinterpret_cast<udf_file_set_descriptor*>(chunk.Data());
			fileSet->tag().init_check(0);
			PDUMP(fileSet);
			fRootIcb = new Icb(this, fileSet->root_directory_icb());
			err = fRootIcb ? fRootIcb->InitCheck() : B_NO_MEMORY;
		}
	}
#endif
	
	RETURN(err);
}

