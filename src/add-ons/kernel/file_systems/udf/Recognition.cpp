#include "Recognition.h"

#include "CS0String.h"
#include "MemoryChunk.h"

using namespace Udf;

//------------------------------------------------------------------------------
// forward declarations
//------------------------------------------------------------------------------

static status_t
walk_volume_recognition_sequence(int device, off_t offset,
                                 uint32 blockSize,
                                 uint32 blockShift);
static status_t
walk_anchor_volume_descriptor_sequences(int device, off_t offset, off_t length,
                                        uint32 blockSize, uint32 blockShift,
                                        udf_logical_descriptor &logicalVolumeDescriptor,
							            udf_partition_descriptor partitionDescriptors[],
							            uint8 &partitionDescriptorCount);
static status_t
walk_volume_descriptor_sequence(udf_extent_address descriptorSequence,
								int device, uint32 blockSize, uint32 blockShift,
							    udf_logical_descriptor &logicalVolumeDescriptor,
							    udf_partition_descriptor partitionDescriptors[],
							    uint8 &partitionDescriptorCount);

//------------------------------------------------------------------------------
// externally visible functions
//------------------------------------------------------------------------------

status_t
Udf::udf_recognize(int device, off_t offset, off_t length, uint32 blockSize,
                   uint32 &blockShift, udf_logical_descriptor &logicalVolumeDescriptor,
				   udf_partition_descriptor partitionDescriptors[],
				   uint8 &partitionDescriptorCount)
{
	DEBUG_INIT_ETC(CF_PRIVATE, NULL, ("device: %d, offset: %Ld, length: %Ld, "
	               "blockSize: %ld, [...descriptors, etc...]", device, offset,
	               length, blockSize));

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
				blockShift = i;
				PRINT(("blockShift: %ld\n", blockShift));
			}			
		}
	}
	
	// Check for a valid volume recognition sequence
	status_t err = walk_volume_recognition_sequence(device, offset, blockSize, blockShift);
	
	// Now hunt down a volume descriptor sequence from one of
	// the anchor volume pointers (if there are any).
	if (!err) {
		err = walk_anchor_volume_descriptor_sequences(device, offset, length,
		                                              blockSize, blockShift,
		                                              logicalVolumeDescriptor,
		                                              partitionDescriptors,
		                                              partitionDescriptorCount);
	}
	RETURN(err);
}

status_t
Udf::udf_recognize(int device, off_t offset, off_t length, uint32 blockSize,
                   char *volumeName)
{
	DEBUG_INIT_ETC(CF_PRIVATE, NULL, ("device: %d, offset: %Ld, length: %Ld, "
	               "blockSize: %ld, volumeName: %p", device, offset, length,
	               blockSize, volumeName));
	udf_logical_descriptor logicalVolumeDescriptor;
	udf_partition_descriptor partitionDescriptors[Udf::kMaxPartitionDescriptors];
	uint8 partitionDescriptorCount;
	uint32 blockShift;
	status_t error = udf_recognize(device, offset, length, blockSize, blockShift,
	                               logicalVolumeDescriptor, partitionDescriptors,
	                               partitionDescriptorCount);
	if (!error && volumeName) {
		CS0String name(logicalVolumeDescriptor.logical_volume_identifier());
		strcpy(volumeName, name.String());
	}
	RETURN(error);
}

//------------------------------------------------------------------------------
// local functions
//------------------------------------------------------------------------------

static
status_t
walk_volume_recognition_sequence(int device, off_t offset, uint32 blockSize, uint32 blockShift)
{
	DEBUG_INIT(CF_PRIVATE, NULL);
	// vrs starts at block 16. Each volume structure descriptor (vsd)
	// should be one block long. We're expecting to find 0 or more iso9660
	// vsd's followed by some ECMA-167 vsd's.
	MemoryChunk chunk(blockSize);
	status_t err = chunk.InitCheck();
	if (!err) {
		bool foundISO = false;
		bool foundExtended = false;
		bool foundECMA167 = false;
		bool foundECMA168 = false;
		bool foundBoot = false;
		for (uint32 block = 16; true; block++) {
	    	PRINT(("block %ld: ", block))
			off_t address = (offset + block) << blockShift;
			ssize_t bytesRead = read_pos(device, address, chunk.Data(), blockSize);
			if (bytesRead == (ssize_t)blockSize)
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
				        blockSize, bytesRead));
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

static
status_t
walk_anchor_volume_descriptor_sequences(int device, off_t offset, off_t length,
                                        uint32 blockSize, uint32 blockShift,
                                        udf_logical_descriptor &logicalVolumeDescriptor,
							            udf_partition_descriptor partitionDescriptors[],
							            uint8 &partitionDescriptorCount)
{
	DEBUG_INIT(CF_PRIVATE, NULL);
	const uint8 avds_location_count = 4;
	const off_t avds_locations[avds_location_count] = {
														256,
	                                                    length-1-256,
	                                                    length-1,
	                                                    512,
	                                                  };
	bool found_vds = false;		                                                  
	for (int32 i = 0; i < avds_location_count; i++) {
		off_t block = avds_locations[i];
		off_t address = (offset + block) << blockShift;
		MemoryChunk chunk(blockSize);
		udf_anchor_descriptor *anchor = NULL;
			
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
			anchor = reinterpret_cast<udf_anchor_descriptor*>(chunk.Data());
			anchorErr = anchor->tag().init_check(block+offset);
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
			anchorErr = walk_volume_descriptor_sequence(anchor->main_vds(), device,
														blockSize, blockShift,
			                                            logicalVolumeDescriptor,
			                                            partitionDescriptors,
			                                            partitionDescriptorCount);
			if (anchorErr)
				anchorErr = walk_volume_descriptor_sequence(anchor->reserve_vds(), device,
															blockSize, blockShift,
			                                            	logicalVolumeDescriptor,
			                                            	partitionDescriptors,
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
	status_t err = found_vds ? B_OK : B_ERROR;
	RETURN(err);
}

static
status_t
walk_volume_descriptor_sequence(udf_extent_address descriptorSequence,
								int device, uint32 blockSize, uint32 blockShift,
							    udf_logical_descriptor &logicalVolumeDescriptor,
							    udf_partition_descriptor partitionDescriptors[],
							    uint8 &partitionDescriptorCount)
{
	DEBUG_INIT_ETC(CF_PRIVATE, NULL, ("descriptorSequence.loc:%ld, descriptorSequence.len:%ld",
	           descriptorSequence.location(), descriptorSequence.length()));
	uint32 count = descriptorSequence.length() >> blockShift;
		
	bool foundLogicalVolumeDescriptor = false;
	uint8 uniquePartitions = 0;
	status_t err = B_OK;
	
	for (uint32 i = 0; i < count; i++)
	{
		off_t block = descriptorSequence.location()+i;
		off_t address = block << blockShift; 
		MemoryChunk chunk(blockSize);
		udf_tag *tag = NULL;

		PRINT(("descriptor #%ld (block %Ld):\n", i, block));

		status_t loopErr = chunk.InitCheck();
		if (!loopErr) {
			ssize_t bytesRead = read_pos(device, address, chunk.Data(), blockSize);
			loopErr = bytesRead == (ssize_t)blockSize ? B_OK : B_IO_ERROR;
			if (loopErr) {
				PRINT(("block %Ld: read_pos(pos:%Ld, len:%ld) failed with error 0x%lx\n",
				       block, address, blockSize, bytesRead));
			}
		}
		if (!loopErr) {
			tag = reinterpret_cast<udf_tag*>(chunk.Data());
			loopErr = tag->init_check(block);
		}
		if (!loopErr) {
			// Now decide what type of descriptor we have
			switch (tag->id()) {
				case TAGID_UNDEFINED:
					break;
					
				case TAGID_PRIMARY_VOLUME_DESCRIPTOR:
				{
					udf_primary_descriptor *primary = reinterpret_cast<udf_primary_descriptor*>(tag);
					PDUMP(primary);				
					(void)primary;	// kill the warning		
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
					(void)imp_use;	// kill the warning		
					break;
				}

				case TAGID_PARTITION_DESCRIPTOR:
				{
					udf_partition_descriptor *partition = reinterpret_cast<udf_partition_descriptor*>(tag);
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
							if (num < Udf::kMaxPartitionDescriptors) {
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
									err = B_BAD_VALUE;
									break;
								}
							}									
						} 
					}
					break;
				}
					
				case TAGID_LOGICAL_VOLUME_DESCRIPTOR:
				{
					udf_logical_descriptor *logical = reinterpret_cast<udf_logical_descriptor*>(tag);
					PDUMP(logical);
					if (foundLogicalVolumeDescriptor) {
						// Keep the vd with the highest vds_number
						if (logicalVolumeDescriptor.vds_number() < logical->vds_number())
							logicalVolumeDescriptor = *logical;
					} else {
						logicalVolumeDescriptor = *logical;
						foundLogicalVolumeDescriptor = true;
						DUMP(logicalVolumeDescriptor);
					}
					break;
				}
					
				case TAGID_UNALLOCATED_SPACE_DESCRIPTOR:
				{
					udf_unallocated_space_descriptor *unallocated = reinterpret_cast<udf_unallocated_space_descriptor*>(tag);
					PDUMP(unallocated);				
					(void)unallocated;	// kill the warning		
					break;
				}
					
				case TAGID_TERMINATING_DESCRIPTOR:
				{
					udf_terminating_descriptor *terminating = reinterpret_cast<udf_terminating_descriptor*>(tag);
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
	
	if (!err) 
		err = foundLogicalVolumeDescriptor ? B_OK : B_ERROR;
	if (!err) 
		err = uniquePartitions >= 1 ? B_OK : B_ERROR;
	if (!err) 
		partitionDescriptorCount = uniquePartitions;	
			
	RETURN(err);
}

