//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file fs_devices.h
*/

#ifndef _FS_DEVICE_H
#define _FS_DEVICE_H

#include <Drivers.h>
#include <OS.h>
#include <SupportDefs.h>

// session flags
enum {
	B_DATA_SESSION			= 0x01,	/* data session */
	B_VIRTUAL_SESSION		= 0x02,	/* e.g. hard disk */
};

typedef struct session_info {
	off_t	offset;				/* offset from start of device (in bytes) */ 
	off_t	size;				/* size (in bytes) */ 
	int32	logical_block_size;	/* logical block size (in bytes) */ 
	int32	index;				/* session index */
	uint32	flags;				/* session flags */
} session_info;

// partition flags
enum {
	B_HIDDEN_PARTITION		= 0x01,	/* non-file system partition */
	B_VIRTUAL_PARTITION		= 0x02,	/* e.g. floppy */
	B_EMPTY_PARTITION		= 0x04,	/* empty partition, implies
									   B_HIDDEN_PARTITION */
};

typedef struct extended_partition_info {
	partition_info	info;
	uint32	flags;					/* partition flags */
	char	partition_name[B_FILE_NAME_LENGTH];
	char	partition_type[B_FILE_NAME_LENGTH];
	char	file_system_short_name[B_FILE_NAME_LENGTH];	/* "", if hidden */
	char	file_system_long_name[B_FILE_NAME_LENGTH];	/* or unknown FS */
	char	volume_name[B_FILE_NAME_LENGTH];			/* "", if hidden */
	char	mounted_at[B_FILE_NAME_LENGTH];				/* "", if not mounted */
										//< better B_PATH_NAME_LENGTH?
	uint8	partition_code;
} extended_partition_info;

#ifdef  __cplusplus
extern "C" {
#endif

// getting infos
status_t get_nth_session_info(int deviceFD, int32 index,
							  session_info *sessionInfo);
status_t get_nth_partition_info(int deviceFD, int32 sessionIndex,
								int32 partitionIndex,
								extended_partition_info *partitionInfo);

// partitioning
status_t partition_session(int deviceFD, int32 sessionIndex,
						   const char *identifier, const char *parameters);

// initialization
// TODO: Move to <unistd.h>. It fits better there.
status_t initialize_volume(const char *where, const char *fileSystem, 
						   const char *volumeName, const char *parameters);


#ifdef  __cplusplus
}
#endif

#endif _FS_DEVICE_H
