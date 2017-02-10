//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//  Mad props to Axel Dörfler and his BFS implementation, from which
//  this UDF implementation draws much influence (and a little code :-P).
//----------------------------------------------------------------------
#include "Volume.h"

#include "Icb.h"
#include "MemoryChunk.h"
#include "PhysicalPartition.h"
#include "Recognition.h"

using namespace Udf;

/*! \brief Creates an unmounted volume with the given id.
*/
Volume::Volume(nspace_id id)
	: fId(id)
	, fDevice(-1)
	, fMounted(false)
	, fOffset(0)
	, fLength(0)
	, fBlockSize(0)
	, fBlockShift(0)
	, fRootIcb(NULL)
{
	for (int i = 0; i < UDF_MAX_PARTITION_MAPS; i++)
		fPartitions[i] = NULL;
}

Volume::~Volume()
{
	_Unset();
}

/*! \brief Attempts to mount the given device.

	\param volumeStart The block on the given device whereat the volume begins.
	\param volumeLength The block length of the volume on the given device.
*/
status_t
Volume::Mount(const char *deviceName, off_t offset, off_t length,
              uint32 blockSize, uint32 flags)
{
	DEBUG_INIT_ETC("Volume",
	               ("deviceName: `%s', offset: %Ld, length: %Ld, blockSize: %ld, "
                   "flags: %ld", deviceName, offset, length, blockSize, flags));
	if (!deviceName)
		RETURN(B_BAD_VALUE);
	if (Mounted()) {
		// Already mounted, thank you for asking
		RETURN(B_BUSY);
	}

	// Open the device read only
	int device = open(deviceName, O_RDONLY);
	if (device < B_OK)
		RETURN(device);
	
	status_t error = B_OK;
	
	// If the device is actually a normal file, try to disable the cache
	// for the file in the parent filesystem
#if _KERNEL_MODE
	struct stat stat;
	error = fstat(device, &stat) < 0 ? B_ERROR : B_OK;
	if (!error) {
		if (stat.st_mode & S_IFREG && ioctl(device, IOCTL_FILE_UNCACHED_IO, NULL) < 0) {
			DIE(("Unable to disable cache of underlying file system.\n"));
		}
	}
#endif

	logical_volume_descriptor logicalVolumeDescriptor;
	partition_descriptor partitionDescriptors[Udf::kMaxPartitionDescriptors];
	uint8 partitionDescriptorCount;
	uint32 blockShift;

	// Run through the volume recognition and descriptor sequences to
	// see if we have a potentially valid UDF volume on our hands
	error = udf_recognize(device, offset, length, blockSize, blockShift,
	                               logicalVolumeDescriptor, partitionDescriptors,
	                               partitionDescriptorCount);

	// Set up the block cache
	if (!error)
		error = init_cache_for_device(device, length);
		
	int physicalCount = 0;
	int virtualCount = 0;
	int sparableCount = 0;
	int metadataCount = 0;
	
	// Set up the partitions
	if (!error) {
		// Set up physical and sparable partitions first
		int offset = 0;
		for (uint8 i = 0; i < logicalVolumeDescriptor.partition_map_count()
		     && !error; i++)
		{
			uint8 *maps = logicalVolumeDescriptor.partition_maps();
			partition_map_header *header =
				reinterpret_cast<partition_map_header*>(maps+offset);
			PRINT(("partition map %d (type %d):\n", i, header->type()));
			if (header->type() == 1) {
				PRINT(("map type: physical\n"));
				physical_partition_map* map =
					reinterpret_cast<physical_partition_map*>(header);
				// Find the corresponding partition descriptor
				partition_descriptor *descriptor = NULL;
				for (uint8 j = 0; j < partitionDescriptorCount; j++) {
					if (map->partition_number() ==
					    partitionDescriptors[j].partition_number())
					{
						descriptor = &partitionDescriptors[j];
						break;
					}
				}
				// Create and add the partition
				if (descriptor) {
					PhysicalPartition *partition = new(nothrow) PhysicalPartition(
					                               map->partition_number(),
					                               descriptor->start(),
					                               descriptor->length());
					error = partition ? B_OK : B_NO_MEMORY;
					if (!error) {
						PRINT(("Adding PhysicalPartition(number: %d, start: %ld, "
						       "length: %ld)\n", map->partition_number(),
						       descriptor->start(), descriptor->length()));						       
						error = _SetPartition(i, partition);
						if (!error)
							physicalCount++;	
					}
				} else {
					PRINT(("no matching partition descriptor found!\n"));
					error = B_ERROR;
				}
			} else if (header->type() == 2) {
				// Figure out what kind of type 2 partition map we have based
				// on the type identifier
				const entity_id &typeId = header->partition_type_id();
				DUMP(typeId);
				DUMP(kSparablePartitionMapId);
				if (typeId.matches(kVirtualPartitionMapId)) {
					PRINT(("map type: virtual\n"));
					virtual_partition_map* map =
						reinterpret_cast<virtual_partition_map*>(header);
					virtualCount++;
					(void)map;	// kill the warning for now
				} else if (typeId.matches(kSparablePartitionMapId)) {
					PRINT(("map type: sparable\n"));
					sparable_partition_map* map =
						reinterpret_cast<sparable_partition_map*>(header);
					sparableCount++;
					(void)map;	// kill the warning for now
				} else if (typeId.matches(kMetadataPartitionMapId)) {
					PRINT(("map type: metadata\n"));
					metadata_partition_map* map =
						reinterpret_cast<metadata_partition_map*>(header);
					metadataCount++;
					(void)map;	// kill the warning for now
				} else {
					PRINT(("map type: unrecognized (`%.23s')\n",
					       typeId.identifier()));
					error = B_ERROR;				
				}
			} else {
				PRINT(("Invalid partition type %d found!\n", header->type()));
				error = B_ERROR;
			}			    
			offset += header->length();
		}
	}
		
	// Do some checking as to what sorts of partitions we've actually found.
	if (!error) {
		error = (physicalCount == 1 && virtualCount == 0
		         && sparableCount == 0 && metadataCount == 0)
		        || (physicalCount == 2 && virtualCount == 0
		           && sparableCount == 0 && metadataCount == 0)
		        ? B_OK : B_ERROR;
		if (error) {
			PRINT(("Invalid partition layout found:\n"));
			PRINT(("  physical partitions: %d\n", physicalCount));
			PRINT(("  virtual partitions:  %d\n", virtualCount));
			PRINT(("  sparable partitions: %d\n", sparableCount));
			PRINT(("  metadata partitions: %d\n", metadataCount));
		}		                 
	}
	
	// We're now going to start creating Icb's, which will expect
	// certain parts of the volume to be initialized properly. Thus,
	// we initialize those parts here.
	if (!error) {
		fDevice = device;	
		fOffset = offset;
		fLength = length;	
		fBlockSize = blockSize;
		fBlockShift = blockShift;
	}
	
	// At this point we've found a valid set of volume descriptors and
	// our partitions are all set up. We now need to investigate the file
	// set descriptor pointed to by the logical volume descriptor.
	if (!error) {
		MemoryChunk chunk(logicalVolumeDescriptor.file_set_address().length());
	
		error = chunk.InitCheck();

		if (!error) {
			off_t address;
			// Read in the file set descriptor
			error = MapBlock(logicalVolumeDescriptor.file_set_address(),
			 	             &address);
			if (!error)
				address <<= blockShift;
			if (!error) {
				ssize_t bytesRead = read_pos(device, address, chunk.Data(),
				                             blockSize);
				if (bytesRead != ssize_t(blockSize)) {
					error = B_IO_ERROR;
					PRINT(("read_pos(pos:%Ld, len:%ld) failed with: 0x%lx\n",
					       address, blockSize, bytesRead));
				}
			}
			// See if it's valid, and if so, create the root icb
			if (!error) {
				file_set_descriptor *fileSet =
				 	reinterpret_cast<file_set_descriptor*>(chunk.Data());
				PDUMP(fileSet);
				error = fileSet->tag().id() == TAGID_FILE_SET_DESCRIPTOR
				        ? B_OK : B_ERROR;
				if (!error) 
					error = fileSet->tag().init_check(
					        logicalVolumeDescriptor.file_set_address().block());
				if (!error) {
					PDUMP(fileSet);
					fRootIcb = new(nothrow) Icb(this, fileSet->root_directory_icb());
					error = fRootIcb ? fRootIcb->InitCheck() : B_NO_MEMORY;
				}
				if (!error) {
					error = new_vnode(Id(), RootIcb()->Id(), (void*)RootIcb());
					if (error) {
						PRINT(("Error creating vnode for root icb! "
						       "error = 0x%lx, `%s'\n", error,
						       strerror(error)));
						// Clean up the icb we created, since _Unset()
						// won't do this for us.
						delete fRootIcb;
						fRootIcb = NULL;
					}
				}	
			}
		}	
	}
	
	// If we've made it this far, we're good to go; set the volume
	// name and then flag that we're mounted. On the other hand, if
	// an error occurred, we need to clean things up.
	if (!error) {
		fName.SetTo(logicalVolumeDescriptor.logical_volume_identifier()); 
		fMounted = true;
	} else {
		_Unset();
	}

	RETURN(error);
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
	DEBUG_INIT_ETC("Volume",
		           ("partition: %d, block: %ld, mappedBlock: %p",
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
	fId = 0;
	if (fDevice >= 0) {
		remove_cached_device_blocks(fDevice, NO_WRITES);	
		close(fDevice);
	}
	fDevice = -1;
	fMounted = false;
	fOffset = 0;
	fLength = 0;
	fBlockSize = 0;
	fBlockShift = 0;
	fName.SetTo("", 0);
	// delete our partitions
	for (int i = 0; i < UDF_MAX_PARTITION_MAPS; i++)
		_SetPartition(i, NULL);
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
Udf::Partition*
Volume::_GetPartition(uint number)
{
	return (number < UDF_MAX_PARTITION_MAPS)
	       ? fPartitions[number] : NULL;
}

