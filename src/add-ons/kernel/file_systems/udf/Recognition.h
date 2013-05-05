/*
 * Copyright 2012, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2003, Tyler Dauwalder, tyler@dauwalder.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _UDF_RECOGNITION_H
#define _UDF_RECOGNITION_H

/*! \file Recognition.h
*/

#include "UdfStructures.h"
#include "UdfDebug.h"

status_t udf_recognize(int device, off_t offset, off_t length,
									uint32 blockSize, uint32 &blockShift,
									primary_volume_descriptor &primaryVolumeDescriptor,
									logical_volume_descriptor &logicalVolumeDescriptor, 
									partition_descriptor partitionDescriptors[],
									uint8 &partitionDescriptorCount);

#endif	// _UDF_RECOGNITION_H
