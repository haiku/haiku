//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _DISK_DEVICE_DEFS_H
#define _DISK_DEVICE_DEFS_H

#include <SupportDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32 partition_id;
typedef int32 disk_system_id;
typedef int32 disk_job_id;

// partition flags
enum {
	B_PARTITION_IS_DEVICE		= 0x01,
	B_PARTITION_MOUNTABLE		= 0x02,
	B_PARTITION_PARTITIONABLE	= 0x04,
	B_PARTITION_READ_ONLY		= 0x08,
	B_PARTITION_MOUNTED			= 0x10,	// needed?
	B_PARTITION_BUSY			= 0x20,
	B_PARTITION_DESCENDANT_BUSY	= 0x40,
};

// disk device flags
enum {
	B_DISK_DEVICE_REMOVABLE		= 0x01,
	B_DISK_DEVICE_HAS_MEDIA		= 0x02,
};

#ifdef __cplusplus
}
#endif

#endif	// _DISK_DEVICE_DEFS_H
