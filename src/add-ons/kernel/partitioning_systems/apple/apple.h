/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef APPLE_H
#define APPLE_H


#include "SupportDefs.h"
#include "ByteOrder.h"


enum apple_signature {
	kDriverDescriptorSignature		= 'ER',
	kPartitionMapSignature			= 'PM',
	kObsoletePartitionMapSignature	= 'TS',
};


struct apple_driver_descriptor {
	int16	signature;
	int16	block_size;
	int32	block_count;
	int16	type;				// reserved
	int16	id;					// "
	int32	data;				// "
	int16	descriptor_count;
	int32	driver_block;
	int16	driver_size;
	int16	os_type;			// operating system type

	int16 BlockSize() { return B_BENDIAN_TO_HOST_INT16(block_size); }
	int32 BlockCount() { return B_BENDIAN_TO_HOST_INT32(block_count); }

	bool HasValidSignature() { return B_BENDIAN_TO_HOST_INT16(signature) == kDriverDescriptorSignature; }
};

struct apple_partition_map {
	int16	signature;
	int16	_reserved0;
	int32	map_block_count;
	int32	start;				// in blocks
	int32	size;
	char	name[32];
	char	type[32];
	int32	data_start;			// in blocks
	int32	data_size;
	int32	status;
	int32	boot_start;
	int32	boot_size;
	int32	boot_address;
	int32	_reserved1;
	int32	boot_entry;
	int32	_reserved2;
	int32	boot_code_checksum;
	char	processor_type[16];

	int32 StartBlock() { return B_BENDIAN_TO_HOST_INT32(start); }
	int32 BlockCount() { return B_BENDIAN_TO_HOST_INT32(size); }
	uint64 Start(apple_driver_descriptor &descriptor) { return StartBlock() * descriptor.BlockSize(); }
	uint64 Size(apple_driver_descriptor &descriptor) { return BlockCount() * descriptor.BlockSize(); }

	bool HasValidSignature();
};


inline bool 
apple_partition_map::HasValidSignature()
{
	int16 sig = B_BENDIAN_TO_HOST_INT16(signature);

	return sig == kPartitionMapSignature
		|| sig == kObsoletePartitionMapSignature;
}


enum partition_status {
	kPartitionIsValid				= 0x00000001,
    kPartitionIsAllocated			= 0x00000002,
    kPartitionIsInUse				= 0x00000004,
    kPartitionIsBootValid			= 0x00000008,
    kPartitionIsReadable			= 0x00000010,
    kPartitionAUXIsWriteable		= 0x00000020,	// dunno why IsWriteable is here twice...
    kPartitionIsBootPIC				= 0x00000040,
    kPartitionIsWriteable			= 0x00000020,
    kPartitionIsMounted				= 0x40000000,
    kPartitionIsStartup				= 0x80000000,
    kPartitionIsChainCompatible		= 0x00000100,
    kPartitionIsRealDeviceDriver	= 0x00000200,
    kPartitionCanChainToNext		= 0x00000400,
};

#endif	/* APPLE_H */
