/*
 * Copyright 2003-2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Akidau
 */


#include <DiskDeviceTypes.h>

#include <disk_device_types.h>


// Device Types

const char* kDeviceTypeFloppyDisk		= FLOPPY_DEVICE_NAME;
const char* kDeviceTypeHardDisk			= HARD_DISK_DEVICE_NAME;
const char* kDeviceTypeOptical			= OPTICAL_DEVICE_NAME;

// Partition types

const char* kPartitionTypeUnrecognized	= UNRECOGNIZED_PARTITION_NAME;

const char* kPartitionTypeMultisession	= MULTISESSION_PARTITION_NAME;
const char* kPartitionTypeAudioSession	= AUDIO_SESSION_PARTITION_NAME;
const char* kPartitionTypeDataSession	= DATA_SESSION_PARTITION_NAME;

const char* kPartitionTypeAmiga			= AMIGA_PARTITION_NAME;
const char* kPartitionTypeApple			= APPLE_PARTITION_NAME;
const char* kPartitionTypeEFI			= EFI_PARTITION_NAME;
const char* kPartitionTypeIntel			= INTEL_PARTITION_NAME;
const char* kPartitionTypeIntelExtended	= INTEL_EXTENDED_PARTITION_NAME;
const char* kPartitionTypeVMDK			= VMDK_PARTITION_NAME;

const char* kPartitionTypeAmigaFFS		= AMIGA_FFS_NAME;
const char* kPartitionTypeBFS			= BFS_NAME;
const char* kPartitionTypeBTRFS			= BTRFS_NAME;
const char* kPartitionTypeEXFAT			= EXFAT_FS_NAME;
const char* kPartitionTypeEXT2			= EXT2_FS_NAME;
const char* kPartitionTypeEXT3			= EXT3_FS_NAME;
const char* kPartitionTypeFAT12			= FAT12_FS_NAME;
const char* kPartitionTypeFAT32			= FAT32_FS_NAME;
const char* kPartitionTypeHFS			= HFS_NAME;
const char* kPartitionTypeHFSPlus		= HFS_PLUS_NAME;
const char* kPartitionTypeISO9660		= ISO9660_FS_NAME;
const char* kPartitionTypeReiser		= REISER_FS_NAME;
const char* kPartitionTypeUDF			= UDF_FS_NAME;
