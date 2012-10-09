/*
 * Copyright 2012, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2003, Tyler Dauwalder, tyler@dauwalder.net.
 * Distributed under the terms of the MIT License.
 */


#include "Recognition.h"

#include "UdfString.h"
#include "MemoryChunk.h"
#include "Utils.h"

#include <string.h>


//------------------------------------------------------------------------------
// forward declarations
//------------------------------------------------------------------------------

static status_t
walk_volume_recognition_sequence(int device, off_t offset, uint32 blockSize,
	uint32 blockShift);

static status_t
walk_anchor_volume_descriptor_sequences(int device, off_t offset, off_t length,
	uint32 blockSize, uint32 blockShift,
	primary_volume_descriptor &primaryVolumeDescriptor,
	logical_volume_descriptor &logicalVolumeDescriptor,
	partition_descriptor partitionDescriptors[],
	uint8 &partitionDescriptorCount);

static status_t
walk_volume_descriptor_sequence(extent_address descriptorSequence, int device,
	uint32 blockSize, uint32 blockShift,
	primary_volume_descriptor &primaryVolumeDescriptor,
	logical_volume_descriptor &logicalVolumeDescriptor,
	partition_descriptor partitionDescriptors[],
	uint8 &partitionDescriptorCount);

static status_t
walk_integrity_sequence(int device, uint32 blockSize, uint32 blockShift,
	extent_address descriptorSequence, uint32 sequenceNumber = 0);

//------------------------------------------------------------------------------
// externally visible functions
//------------------------------------------------------------------------------


status_t
udf_recognize(int device, off_t offset, off_t length, uint32 blockSize,
	uint32 &blockShift, primary_volume_descriptor &primaryVolumeDescriptor,
	logical_volume_descriptor &logicalVolumeDescriptor,
	partition_descriptor partitionDescriptors[],
	uint8 &partitionDescriptorCount)
{
	TRACE(("udf_recognize: device: = %d, offset = %Ld, length = %Ld, "
		"blockSize = %ld, ", device, offset, length, blockSize));

	// Check the block size
	status_t status = get_block_shift(blockSize, blockShift);
	if (status != B_OK) {
		TRACE_ERROR(("\nudf_recognize: Block size must be a positive power of "
			"two! (blockSize = %ld)\n", blockSize));
		return status;
	}
	TRACE(("blockShift: %ld\n", blockShift));

	// Check for a valid volume recognition sequence
	status = walk_volume_recognition_sequence(device, offset, blockSize,
		blockShift);
	if (status != B_OK) {
		TRACE_ERROR(("udf_recognize: Invalid sequence. status = %ld\n", status));
		return status;
	}
	// Now hunt down a volume descriptor sequence from one of
	// the anchor volume pointers (if there are any).
	status = walk_anchor_volume_descriptor_sequences(device, offset, length,
		blockSize, blockShift, primaryVolumeDescriptor,
		logicalVolumeDescriptor, partitionDescriptors,
		partitionDescriptorCount);
	if (status != B_OK) {
		TRACE_ERROR(("udf_recognize: cannot find volume descriptor. status = %ld\n",
			status));
		return status;
	}
	// Now walk the integrity sequence and make sure the last integrity
	// descriptor is a closed descriptor
	status = walk_integrity_sequence(device, blockSize, blockShift,
		logicalVolumeDescriptor.integrity_sequence_extent());
	if (status != B_OK) {
		TRACE_ERROR(("udf_recognize: last integrity descriptor not closed. "
			"status = %ld\n", status));
		return status;
	}

	return B_OK;
}

//------------------------------------------------------------------------------
// local functions
//------------------------------------------------------------------------------

static
status_t
walk_volume_recognition_sequence(int device, off_t offset, uint32 blockSize,
	uint32 blockShift)
{
	TRACE(("walk_volume_recognition_sequence: device = %d, offset = %Ld, "
		"blockSize = %ld, blockShift = %lu\n", device, offset, blockSize,
		blockShift));

	// vrs starts at block 16. Each volume structure descriptor (vsd)
	// should be one block long. We're expecting to find 0 or more iso9660
	// vsd's followed by some ECMA-167 vsd's.
	MemoryChunk chunk(blockSize);
	if (chunk.InitCheck() != B_OK) {
		TRACE_ERROR(("walk_volume_recognition_sequence: Failed to construct "
			"MemoryChunk\n"));
		return B_ERROR;
	}

	bool foundExtended = false;
	bool foundECMA167 = false;
	bool foundECMA168 = false;
	for (uint32 block = 16; true; block++) {
		off_t address = (offset + block) << blockShift;
		TRACE(("walk_volume_recognition_sequence: block = %ld, "
			"address = %Ld, ", block, address));
		ssize_t bytesRead = read_pos(device, address, chunk.Data(), blockSize);
		if (bytesRead == (ssize_t)blockSize) {
			volume_structure_descriptor_header* descriptor
				= (volume_structure_descriptor_header *)(chunk.Data());
			if (descriptor->id_matches(kVSDID_ISO)) {
				TRACE(("found ISO9660 descriptor\n"));
			} else if (descriptor->id_matches(kVSDID_BEA)) {
				TRACE(("found BEA descriptor\n"));
				foundExtended = true;
			} else if (descriptor->id_matches(kVSDID_TEA)) {
				TRACE(("found TEA descriptor\n"));
				foundExtended = true;
			} else if (descriptor->id_matches(kVSDID_ECMA167_2)) {
				TRACE(("found ECMA-167 rev 2 descriptor\n"));
				foundECMA167 = true;
			} else if (descriptor->id_matches(kVSDID_ECMA167_3)) {
				TRACE(("found ECMA-167 rev 3 descriptor\n"));
				foundECMA167 = true;
			} else if (descriptor->id_matches(kVSDID_BOOT)) {
				TRACE(("found boot descriptor\n"));
			} else if (descriptor->id_matches(kVSDID_ECMA168)) {
				TRACE(("found ECMA-168 descriptor\n"));
				foundECMA168 = true;
			} else {
				TRACE(("found invalid descriptor, id = `%.5s'\n", descriptor->id));
				break;
			}
		} else {
			TRACE_ERROR(("read_pos(pos:%Ld, len:%ld) failed with: 0x%lx\n", address,
				blockSize, bytesRead));
			break;
		}
	}

	// If we find an ECMA-167 descriptor, OR if we find a beginning
	// or terminating extended area descriptor with NO ECMA-168
	// descriptors, we return B_OK to signal that we should go
	// looking for valid anchors.
	return foundECMA167 || (foundExtended && !foundECMA168) ? B_OK : B_ERROR;	
}


static status_t
walk_anchor_volume_descriptor_sequences(int device, off_t offset, off_t length,
	uint32 blockSize, uint32 blockShift, 
	primary_volume_descriptor &primaryVolumeDescriptor,
	logical_volume_descriptor &logicalVolumeDescriptor,
	partition_descriptor partitionDescriptors[],
	uint8 &partitionDescriptorCount)
{
	DEBUG_INIT(NULL);
	const uint8 avds_location_count = 4;
	const off_t avds_locations[avds_location_count]
		= { 256, length-1-256, length-1, 512, };
	bool found_vds = false;		                                                  
	for (int32 i = 0; i < avds_location_count; i++) {
		off_t block = avds_locations[i];
		off_t address = (offset + block) << blockShift;
		MemoryChunk chunk(blockSize);
		anchor_volume_descriptor *anchor = NULL;

		status_t anchorErr = chunk.InitCheck();
		if (!anchorErr) {
			ssize_t bytesRead = read_pos(device, address, chunk.Data(), blockSize);
			anchorErr = bytesRead == (ssize_t)blockSize ? B_OK : B_IO_ERROR;
			if (anchorErr) {
				PRINT(("block %Ld: read_pos(pos:%Ld, len:%ld) failed with error 0x%lx\n",
				       block, address, blockSize, bytesRead));
			}
		}
		if (!anchorErr) {
			anchor = reinterpret_cast<anchor_volume_descriptor*>(chunk.Data());
			anchorErr = anchor->tag().init_check(block + offset);
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
			anchorErr = walk_volume_descriptor_sequence(anchor->main_vds(),
				device, blockSize, blockShift, primaryVolumeDescriptor,
				logicalVolumeDescriptor, partitionDescriptors,
				partitionDescriptorCount);

			if (anchorErr)
				anchorErr = walk_volume_descriptor_sequence(anchor->reserve_vds(),
					device,	blockSize, blockShift, primaryVolumeDescriptor,
					logicalVolumeDescriptor, partitionDescriptors,
					partitionDescriptorCount);				
		}
		if (!anchorErr) {
			PRINT(("block %Ld: found valid vds\n", avds_locations[i]));
			found_vds = true;
			break;
		} else {
			// Both failed, so loop around and try another avds
			PRINT(("block %Ld: vds search failed\n", avds_locations[i]));
		}
	}
	status_t error = found_vds ? B_OK : B_ERROR;
	RETURN(error);
}

static
status_t
walk_volume_descriptor_sequence(extent_address descriptorSequence,
	int device, uint32 blockSize, uint32 blockShift,
	primary_volume_descriptor &primaryVolumeDescriptor,
	logical_volume_descriptor &logicalVolumeDescriptor,
	partition_descriptor partitionDescriptors[],
	uint8 &partitionDescriptorCount)
{
	DEBUG_INIT_ETC(NULL, ("descriptorSequence.loc:%ld, descriptorSequence.len:%ld",
	           descriptorSequence.location(), descriptorSequence.length()));
	uint32 count = descriptorSequence.length() >> blockShift;
		
	bool foundLogicalVolumeDescriptor = false;
	bool foundUnallocatedSpaceDescriptor = false;
	bool foundUdfImplementationUseDescriptor = false;
	uint8 uniquePartitions = 0;
	status_t error = B_OK;
	
	for (uint32 i = 0; i < count; i++)
	{
		off_t block = descriptorSequence.location()+i;
		off_t address = block << blockShift; 
		MemoryChunk chunk(blockSize);
		descriptor_tag  *tag = NULL;

		PRINT(("descriptor #%ld (block %Ld):\n", i, block));

		status_t loopError = chunk.InitCheck();
		if (!loopError) {
			ssize_t bytesRead = read_pos(device, address, chunk.Data(), blockSize);
			loopError = bytesRead == (ssize_t)blockSize ? B_OK : B_IO_ERROR;
			if (loopError) {
				PRINT(("block %Ld: read_pos(pos:%Ld, len:%ld) failed with error 0x%lx\n",
				       block, address, blockSize, bytesRead));
			}
		}
		if (!loopError) {
			tag = reinterpret_cast<descriptor_tag *>(chunk.Data());
			loopError = tag->init_check(block);
		}
		if (!loopError) {
			// Now decide what type of descriptor we have
			switch (tag->id()) {
				case TAGID_UNDEFINED:
					break;

				case TAGID_PRIMARY_VOLUME_DESCRIPTOR:
				{
					primary_volume_descriptor *primary = reinterpret_cast<primary_volume_descriptor*>(tag);
					PDUMP(primary);				
					primaryVolumeDescriptor = *primary;
					break;
				}

				case TAGID_ANCHOR_VOLUME_DESCRIPTOR_POINTER:
					break;
					
				case TAGID_VOLUME_DESCRIPTOR_POINTER:
					break;

				case TAGID_IMPLEMENTATION_USE_VOLUME_DESCRIPTOR:
				{
					implementation_use_descriptor *impUse = reinterpret_cast<implementation_use_descriptor*>(tag);
					PDUMP(impUse);
					// Check for a matching implementation id string (note that the
					// revision version is not checked)
					if (impUse->tag().init_check(block) == B_OK
					    && impUse->implementation_id().matches(kLogicalVolumeInfoId201))
					{
							foundUdfImplementationUseDescriptor = true;
					}
					break;
				}

				case TAGID_PARTITION_DESCRIPTOR:
				{
					partition_descriptor *partition = reinterpret_cast<partition_descriptor*>(tag);
					PDUMP(partition);
					if (partition->tag().init_check(block) == B_OK) {
						// Check for a previously discovered partition descriptor with
						// the same number as this partition. If found, keep the one with
						// the higher vds number.
						bool foundDuplicate = false;
						int num;
						for (num = 0; num < uniquePartitions; num++) {
							if (partitionDescriptors[num].partition_number()
							    == partition->partition_number())
							{
								foundDuplicate = true;
								if (partitionDescriptors[num].vds_number()
								    < partition->vds_number())
								{
									partitionDescriptors[num] = *partition;		
									PRINT(("Replacing previous partition #%d (vds_number: %ld) with "
									       "new partition #%d (vds_number: %ld)\n",
									       partitionDescriptors[num].partition_number(),
									       partitionDescriptors[num].vds_number(),
									       partition->partition_number(),
									       partition->vds_number()));
								}
								break;
							}							    
						}
						// If we didn't find a duplicate, see if we have any open descriptor
						// spaces left.
						if (!foundDuplicate) {
							if (num < kMaxPartitionDescriptors) {
								// At least one more partition descriptor allowed
								partitionDescriptors[num] = *partition;
								uniquePartitions++;
								PRINT(("Adding partition #%d (vds_number: %ld)\n",
								       partition->partition_number(),
								       partition->vds_number()));
							} else {
								// We've found more than kMaxPartitionDescriptor uniquely-
								// numbered partitions. So, search through the partitions
								// we already have again, this time just looking for a
								// partition with a lower vds number. If we find one,
								// replace it with this one. If we don't, scream bloody
								// murder.
								bool foundReplacement = false;
								for (int j = 0; j < uniquePartitions; j++) {
									if (partitionDescriptors[j].vds_number()
									    < partition->vds_number())
									{
										foundReplacement = true;
										partitionDescriptors[j] = *partition;
										PRINT(("Replacing partition #%d (vds_number: %ld) "
										       "with partition #%d (vds_number: %ld)\n",
										       partitionDescriptors[j].partition_number(),
										       partitionDescriptors[j].vds_number(),
										       partition->partition_number(),
										       partition->vds_number()));
										break;
									}
								}
								if (!foundReplacement) {
									PRINT(("Found more than kMaxPartitionDescriptors == %d "
									       "unique partition descriptors!\n",
									       kMaxPartitionDescriptors));
									error = B_BAD_VALUE;
									break;
								}
							}									
						} 
					}
					break;
				}
					
				case TAGID_LOGICAL_VOLUME_DESCRIPTOR:
				{
					logical_volume_descriptor *logical = reinterpret_cast<logical_volume_descriptor*>(tag);
					PDUMP(logical);
					if (foundLogicalVolumeDescriptor) {
						// Keep the vd with the highest vds_number
						if (logicalVolumeDescriptor.vds_number() < logical->vds_number())
							logicalVolumeDescriptor = *logical;
					} else {
						logicalVolumeDescriptor = *logical;
						foundLogicalVolumeDescriptor = true;
					}
					break;
				}
					
				case TAGID_UNALLOCATED_SPACE_DESCRIPTOR:
				{
					unallocated_space_descriptor *unallocated = reinterpret_cast<unallocated_space_descriptor*>(tag);
					PDUMP(unallocated);
					foundUnallocatedSpaceDescriptor = true;		
					(void)unallocated;	// kill the warning		
					break;
				}
					
				case TAGID_TERMINATING_DESCRIPTOR:
				{
					terminating_descriptor *terminating = reinterpret_cast<terminating_descriptor*>(tag);
					PDUMP(terminating);
					(void)terminating;	// kill the warning		
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

	PRINT(("found %d unique partition%s\n", uniquePartitions,
	       (uniquePartitions == 1 ? "" : "s")));
	
	if (!error && !foundUdfImplementationUseDescriptor) {
		INFORM(("WARNING: no valid udf implementation use descriptor found\n"));
	}
	if (!error) 
		error = foundLogicalVolumeDescriptor
		        && foundUnallocatedSpaceDescriptor
		        ? B_OK : B_ERROR;
	if (!error) 
		error = uniquePartitions >= 1 ? B_OK : B_ERROR;
	if (!error) 
		partitionDescriptorCount = uniquePartitions;	
			
	RETURN(error);
}

/*! \brief Walks the integrity sequence in the extent given by \a descriptorSequence.

	\return
	- \c B_OK: Success. the sequence was terminated by a valid, closed
	           integrity descriptor.
	- \c B_ENTRY_NOT_FOUND: The sequence was empty.
	- (other error code): The sequence was non-empty and did not end in a valid,
                          closed integrity descriptor.
*/                        	
static status_t
walk_integrity_sequence(int device, uint32 blockSize, uint32 blockShift,
                        extent_address descriptorSequence, uint32 sequenceNumber)
{
	DEBUG_INIT_ETC(NULL, ("descriptorSequence.loc:%ld, descriptorSequence.len:%ld",
	           descriptorSequence.location(), descriptorSequence.length()));
	uint32 count = descriptorSequence.length() >> blockShift;
		
	bool lastDescriptorWasClosed = false;
	uint16 highestMinimumUDFReadRevision = 0x0000;
	status_t error = count > 0 ? B_OK : B_ENTRY_NOT_FOUND;
	for (uint32 i = 0; error == B_OK && i < count; i++)
	{
		off_t block = descriptorSequence.location()+i;
		off_t address = block << blockShift; 
		MemoryChunk chunk(blockSize);
		descriptor_tag *tag = NULL;

		PRINT(("integrity descriptor #%ld:%ld (block %Ld):\n", sequenceNumber, i, block));

		status_t loopError = chunk.InitCheck();
		if (!loopError) {
			ssize_t bytesRead = read_pos(device, address, chunk.Data(), blockSize);
			loopError = check_size_error(bytesRead, blockSize);
			if (loopError) {
				PRINT(("block %Ld: read_pos(pos:%Ld, len:%ld) failed with error 0x%lx\n",
				       block, address, blockSize, bytesRead));
			}
		}
		if (!loopError) {
			tag = reinterpret_cast<descriptor_tag *>(chunk.Data());
			loopError = tag->init_check(block);
		}
		if (!loopError) {
			// Check the descriptor type and see if it's closed.
			loopError = tag->id() == TAGID_LOGICAL_VOLUME_INTEGRITY_DESCRIPTOR
			            ? B_OK : B_BAD_DATA;
			if (!loopError) {
				logical_volume_integrity_descriptor *descriptor =
					reinterpret_cast<logical_volume_integrity_descriptor*>(chunk.Data());
				PDUMP(descriptor);
				lastDescriptorWasClosed = descriptor->integrity_type() == INTEGRITY_CLOSED;
				if (lastDescriptorWasClosed) {
					uint16 minimumRevision = descriptor->minimum_udf_read_revision();
					if (minimumRevision > highestMinimumUDFReadRevision) {
						highestMinimumUDFReadRevision = minimumRevision;
					} else if (minimumRevision < highestMinimumUDFReadRevision) {
						INFORM(("WARNING: found decreasing minimum udf read revision in integrity "
						        "sequence (last highest: 0x%04x, current: 0x%04x); using higher "
						        "revision.\n", highestMinimumUDFReadRevision, minimumRevision));
					}
				}
						        
				// Check a continuation extent if necessary. Note that this effectively
				// ends our search through this extent
				extent_address &next = descriptor->next_integrity_extent();
				if (next.length() > 0) {
					status_t nextError = walk_integrity_sequence(device, blockSize, blockShift,
					                                             next, sequenceNumber+1);
					if (nextError && nextError != B_ENTRY_NOT_FOUND) {
						// Continuation proved invalid
						error = nextError;
						break;						
					} else {
						// Either the continuation was valid or empty; either way,
						// we're done searching.
						break;
					}  
				}
			} else {
				PDUMP(tag);
			}				
		}
		// If we hit an error on the first item, consider the extent empty,
		// otherwise just break out of the loop and assume part of the
		// extent is unrecorded
		if (loopError) {
			if (i == 0) 
				error = B_ENTRY_NOT_FOUND;
			else
				break;
		}
	}
	if (!error)
		error = lastDescriptorWasClosed ? B_OK : B_BAD_DATA;
	if (error == B_OK
		&& highestMinimumUDFReadRevision > UDF_MAX_READ_REVISION) {
		error = B_ERROR;
		FATAL(("found udf revision 0x%x more than max 0x%x\n",
			highestMinimumUDFReadRevision, UDF_MAX_READ_REVISION));
	}
	RETURN(error);					
}
