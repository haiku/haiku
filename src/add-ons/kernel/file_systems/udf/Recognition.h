//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------
#ifndef _UDF_RECOGNITION_H
#define _UDF_RECOGNITION_H

/*! \file Recognition.h
*/

#include "UdfStructures.h"
#include "UdfDebug.h"

status_t udf_recognize(int device, off_t offset, off_t length,
					   uint32 blockSize, uint32 &blockShift,
                       logical_volume_descriptor &logicalVolumeDescriptor,
                       partition_descriptor partitionDescriptors[],
                       uint8 &partitionDescriptorCount);

#endif	// _UDF_RECOGNITION_H
