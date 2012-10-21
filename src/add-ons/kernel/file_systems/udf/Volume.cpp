/*
 * Copyright 2012, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2008, Salvatore Benedetto, salvatore.benedetto@gmail.com
 * Copyright 2003, Tyler Dauwalder, tyler@dauwalder.net.
 * Distributed under the terms of the MIT License.
 */

#include "Volume.h"

#include "Icb.h"
#include "MemoryChunk.h"
#include "MetadataPartition.h"
#include "PhysicalPartition.h"
#include "Recognition.h"

extern fs_volume_ops gUDFVolumeOps;
extern fs_vnode_ops gUDFVnodeOps;

/*! \brief Creates an unmounted volume with the given id. */
Volume::Volume(fs_volume *fsVolume)
	:
	fBlockCache(NULL),
	fBlockShift(0),
	fBlockSize(0),
	fDevice(-1),
	fFSVolume(fsVolume),
	fLength(0),
	fMounted(false),
	fOffset(0),
	fRootIcb(NULL)
{
	for (int i = 0; i < UDF_MAX_PARTITION_MAPS; i++)
		fPartitions[i] = NULL;
}


Volume::~Volume()
{
	_Unset();
}


/*! \brief Attempts to mount the given device.

	\param lenght The length of the device in number of blocks
*/
status_t
Volume::Mount(const char *deviceName, off_t offset, off_t length,
	uint32 blockSize, uint32 flags)
{
	TRACE(("Volume::Mount: deviceName = `%s', offset = %Ld, length = %Ld, "
		"blockSize: %ld, flags: %ld\n", deviceName, offset, length, blockSize,
		flags));
	if (!deviceName)
		return B_BAD_VALUE;
	if (Mounted())
		// Already mounted, thank you for asking
		return B_BUSY;

	// Open the device read only
	int device = open(deviceName, O_RDONLY);
	if (device < B_OK) {
		TRACE_ERROR(("Volume::Mount: failed to open device = %s\n", deviceName));
		return device;
	}

	DEBUG_INIT_ETC("Volume", ("deviceName: %s", deviceName));
	status_t status = B_OK;

	// If the device is actually a normal file, try to disable the cache
	// for the file in the parent filesystem
#if 0 //  _KERNEL_MODE
	struct stat stat;
	error = fstat(device, &stat) < 0 ? B_ERROR : B_OK;
	if (!error) {
		if (stat.st_mode & S_IFREG && ioctl(device, IOCTL_FILE_UNCACHED_IO, NULL) < 0) {
			DIE(("Unable to disable cache of underlying file system.\n"));
		}
	}
#endif

	logical_volume_descriptor logicalVolumeDescriptor;
	partition_descriptor partitionDescriptors[kMaxPartitionDescriptors];
	uint8 partitionDescriptorCount;
	uint32 blockShift;

	// Run through the volume recognition and descriptor sequences to
	// see if we have a potentially valid UDF volume on our hands
	status = udf_recognize(device, offset, length, blockSize, blockShift,
				fPrimaryVolumeDescriptor, logicalVolumeDescriptor,
				partitionDescriptors, partitionDescriptorCount);

	// Set up the block cache
	if (!status) {
		TRACE(("Volume::Mount: partition recognized\n"));
		fBlockCache = block_cache_create(device, length, blockSize, IsReadOnly());
	} else {
		TRACE_ERROR(("Volume::Mount: failed to recognize partition\n"));
		return status;
	}

	int physicalCount = 0;
	int virtualCount = 0;
	int sparableCount = 0;
	int metadataCount = 0;

	// Set up the partitions
	// Set up physical and sparable partitions first
	offset = 0;
	for (uint8 i = 0; i < logicalVolumeDescriptor.partition_map_count()
	     && !status; i++)
	{
		uint8 *maps = logicalVolumeDescriptor.partition_maps();
		partition_map_header *header = (partition_map_header *)(maps + offset);
		TRACE(("Volume::Mount: partition map %d (type %d):\n", i,
			header->type()));
		if (header->type() == 1) {
			TRACE(("Volume::Mount: map type -> physical\n"));
			physical_partition_map* map = (physical_partition_map *)header;
			// Find the corresponding partition descriptor
			partition_descriptor *descriptor = NULL;
			for (uint8 j = 0; j < partitionDescriptorCount; j++) {
				if (map->partition_number() ==
				    partitionDescriptors[j].partition_number()) {
					descriptor = &partitionDescriptors[j];
					break;
				}
			}
			// Create and add the partition
			if (descriptor) {
				PhysicalPartition *partition
					= new(nothrow) PhysicalPartition(map->partition_number(),
						descriptor->start(), descriptor->length());
				status = partition ? B_OK : B_NO_MEMORY;
				if (!status) {
					TRACE(("Volume::Mount: adding PhysicalPartition(number: %d, "
						"start: %ld, length: %ld)\n", map->partition_number(),
						descriptor->start(), descriptor->length()));
					status = _SetPartition(i, partition);
					if (!status)
						physicalCount++;
				}
			} else {
				TRACE_ERROR(("Volume::Mount: no matching partition descriptor found!\n"));
				status = B_ERROR;
			}
		} else if (header->type() == 2) {
			// Figure out what kind of type 2 partition map we have based
			// on the type identifier
			const entity_id &typeId = header->partition_type_id();
			DUMP(typeId);
			DUMP(kSparablePartitionMapId);
			if (typeId.matches(kVirtualPartitionMapId)) {
				TRACE(("map type: virtual\n"));
				virtual_partition_map* map =
					reinterpret_cast<virtual_partition_map*>(header);
				virtualCount++;
				(void)map;	// kill the warning for now
			} else if (typeId.matches(kSparablePartitionMapId)) {
				TRACE(("map type: sparable\n"));
				sparable_partition_map* map =
					reinterpret_cast<sparable_partition_map*>(header);
				sparableCount++;
				(void)map;	// kill the warning for now
			} else if (typeId.matches(kMetadataPartitionMapId)) {
				TRACE(("map type: metadata\n"));
				metadata_partition_map* map =
					reinterpret_cast<metadata_partition_map*>(header);
				
				
				// Find the corresponding partition descriptor
				partition_descriptor *descriptor = NULL;
				for (uint8 j = 0; j < partitionDescriptorCount; j++) {
					if (map->partition_number() ==
						partitionDescriptors[j].partition_number()) {
						descriptor = &partitionDescriptors[j];
						break;
					}
				}
				Partition *parent = _GetPartition(map->partition_number());
				// Create and add the partition
				if (descriptor != NULL && parent != NULL) {
					MetadataPartition *partition
						= new(nothrow) MetadataPartition(this,
							map->partition_number(), *parent,
							map->metadata_file_location(),
							map->metadata_mirror_file_location(),
							map->metadata_bitmap_file_location(),
							map->allocation_unit_size(),
							map->alignment_unit_size(),
							map->flags() & 1);
					status = partition ? partition->InitCheck() : B_NO_MEMORY;
					if (!status) {
						TRACE(("Volume::Mount: adding MetadataPartition()"));
						status = _SetPartition(i, partition);
						if (status == B_OK)
							metadataCount++;
					} else {
						TRACE_ERROR(("Volume::Mount: metadata partition "
							"creation failed! 0x%lx\n", status));
					}
				} else {
					TRACE_ERROR(("Volume::Mount: no matching partition descriptor found!\n"));
					status = B_ERROR;
				}
			} else {
				TRACE(("map type: unrecognized (`%.23s')\n",
				       typeId.identifier()));
				status = B_ERROR;				
			}
		} else {
			TRACE(("Invalid partition type %d found!\n", header->type()));
			status = B_ERROR;
		}			    
		offset += header->length();
	}

	// Do some checking as to what sorts of partitions we've actually found.
	if (!status) {
		status = (physicalCount == 1 && virtualCount == 0
		         && sparableCount == 0)
		        || (physicalCount == 2 && virtualCount == 0
		           && sparableCount == 0)
		        ? B_OK : B_ERROR;
		if (status) {
			TRACE(("Invalid partition layout found:\n"));
			TRACE(("  physical partitions: %d\n", physicalCount));
			TRACE(("  virtual partitions:  %d\n", virtualCount));
			TRACE(("  sparable partitions: %d\n", sparableCount));
			TRACE(("  metadata partitions: %d\n", metadataCount));
		}		                 
	}
	
	// We're now going to start creating Icb's, which will expect
	// certain parts of the volume to be initialized properly. Thus,
	// we initialize those parts here.
	if (!status) {
		fDevice = device;	
		fOffset = offset;
		fLength = length;	
		fBlockSize = blockSize;
		fBlockShift = blockShift;
	}
	TRACE(("Volume::Mount: device = %d, offset = %Ld, length = %Ld, "
		"blockSize = %ld, blockShift = %ld\n", device, offset, length,
		blockSize, blockShift));
	// At this point we've found a valid set of volume descriptors and
	// our partitions are all set up. We now need to investigate the file
	// set descriptor pointed to by the logical volume descriptor.
	if (!status) {
		TRACE(("Volume::Mount: Partition has been set up\n"));
		MemoryChunk chunk(logicalVolumeDescriptor.file_set_address().length());
	
		status = chunk.InitCheck();

		if (!status) {
			off_t address;
			// Read in the file set descriptor
			status = MapBlock(logicalVolumeDescriptor.file_set_address(),
				&address);
			if (!status)
				address <<= blockShift;
			if (!status) {
				ssize_t bytesRead
					= read_pos(device, address, chunk.Data(), blockSize);
				if (bytesRead != ssize_t(blockSize)) {
					status = B_IO_ERROR;
					TRACE_ERROR(("read_pos(pos:%Ld, len:%ld) failed with: 0x%lx\n",
						address, blockSize, bytesRead));
				}
			}
			// See if it's valid, and if so, create the root icb
			if (!status) {
				file_set_descriptor *fileSet =
				 	reinterpret_cast<file_set_descriptor*>(chunk.Data());
				PDUMP(fileSet);
				status = fileSet->tag().id() == TAGID_FILE_SET_DESCRIPTOR
				        ? B_OK : B_ERROR;
				if (!status) 
					status = fileSet->tag().init_check(
					        logicalVolumeDescriptor.file_set_address().block());
				if (!status) {
					PDUMP(fileSet);
					fRootIcb = new(nothrow) Icb(this, fileSet->root_directory_icb());
					if (fRootIcb == NULL || fRootIcb->InitCheck() != B_OK)
						return B_NO_MEMORY;
				}

				TRACE(("Volume::Mount: Root Node id = %Ld\n", fRootIcb->Id()));
				if (!status) {
					status = publish_vnode(fFSVolume, fRootIcb->Id(), fRootIcb,
						&gUDFVnodeOps, fRootIcb->Mode(), 0);
					if (status != B_OK) {
						TRACE_ERROR(("Error creating vnode for root icb! "
						       "status = 0x%lx, `%s'\n", status,
						       strerror(status)));
						// Clean up the icb we created, since _Unset()
						// won't do this for us.
						delete fRootIcb;
						fRootIcb = NULL;
					}
					TRACE(("Volume::Mount: Root vnode published. Id = %Ld\n",
						fRootIcb->Id()));
				}
			}
		}
	}

	// If we've made it this far, we're good to go; set the volume
	// name and then flag that we're mounted. On the other hand, if
	// an error occurred, we need to clean things up.
	if (!status) {
		fName.SetTo(logicalVolumeDescriptor.logical_volume_identifier()); 
		fMounted = true;
	} else {
		_Unset();
	}

	RETURN(status);
}


const char*
Volume::Name() const {
	return fName.Utf8();
}

/*! \brief Maps the given logical block to a physical block.
*/
status_t
Volume::MapBlock(long_address address, off_t *mappedBlock)
{
	TRACE(("Volume::MapBlock: partition = %d, block = %ld, mappedBlock = %p\n",
		address.partition(), address.block(), mappedBlock));
	DEBUG_INIT_ETC("Volume", ("partition = %d, block = %ld, mappedBlock = %p",
		address.partition(), address.block(), mappedBlock));
	status_t error = mappedBlock ? B_OK : B_BAD_VALUE;
	if (!error) {
		Partition *partition = _GetPartition(address.partition());
		error = partition ? B_OK : B_BAD_ADDRESS;
		if (!error) 
			error = partition->MapBlock(address.block(), *mappedBlock);
	}
	RETURN(error);
}

/*! \brief Unsets the volume and deletes any partitions.

	Does *not* delete the root icb object.
*/
void
Volume::_Unset()
{
	DEBUG_INIT("Volume");
	// delete our partitions
	for (int i = 0; i < UDF_MAX_PARTITION_MAPS; i++)
		_SetPartition(i, NULL);
	fFSVolume->id = 0;
	if (fDevice >= 0) {
		block_cache_delete(fBlockCache, true);	
		close(fDevice);
	}
	fBlockCache = NULL;
	fDevice = -1;
	fMounted = false;
	fOffset = 0;
	fLength = 0;
	fBlockSize = 0;
	fBlockShift = 0;
	fName.SetTo("", 0);
}

/*! \brief Sets the partition associated with the given number after
	deleting any previously associated partition.
	
	\param number The partition number (should be the same as the index
	              into the lvd's partition map array).
	\param partition The new partition (may be NULL).
*/
status_t
Volume::_SetPartition(uint number, Partition *partition)
{
	status_t error = number < UDF_MAX_PARTITION_MAPS
	                 ? B_OK : B_BAD_VALUE;
	if (!error) {
		delete fPartitions[number];
		fPartitions[number] = partition;
	}
	return error;
}

/*! \brief Returns the partition associated with the given number, or
	NULL if no such partition exists or the number is invalid.
*/
Partition*
Volume::_GetPartition(uint number)
{
	return (number < UDF_MAX_PARTITION_MAPS)
	       ? fPartitions[number] : NULL;
}
