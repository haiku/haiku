/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 *
 * Disk device type macros for kernel and boot loader. In userland use
 * <DiskDeviceTypes.h>.
 */
#ifndef _SYSTEM_DISK_DEVICE_TYPES_H
#define _SYSTEM_DISK_DEVICE_TYPES_H


// Device Types

#define FLOPPY_DEVICE_NAME		"Floppy Disk Media"
#define HARD_DISK_DEVICE_NAME	"Hard Disk Media"
#define OPTICAL_DEVICE_NAME 	"Optical Media"

// Partition types

#define UNRECOGNIZED_PARTITION_NAME		"Unrecognized"

#define MULTISESSION_PARTITION_NAME		"Multisession Storage Device"
#define AUDIO_SESSION_PARTITION_NAME	"Audio Session"
#define DATA_SESSION_PARTITION_NAME		"Data Session"

#define AMIGA_PARTITION_NAME			"Amiga Partition Map"
#define APPLE_PARTITION_NAME			"Apple Partition Map"
#define EFI_PARTITION_NAME				"GUID Partition Map"
#define INTEL_PARTITION_NAME			"Intel Partition Map"
#define INTEL_EXTENDED_PARTITION_NAME	"Intel Extended Partition"
#define VMDK_PARTITION_NAME				"VMDK Partition"

#define AMIGA_FFS_NAME					"AmigaFFS File System"
#define BFS_NAME						"Be File System"
#define BTRFS_NAME						"Btrfs File System"
#define EXFAT_FS_NAME					"exFAT File System"
#define EXT2_FS_NAME					"EXT2 File System"
#define EXT3_FS_NAME					"EXT3 File System"
#define FAT12_FS_NAME					"FAT12 File System"
#define FAT32_FS_NAME					"FAT32 File System"
#define HFS_NAME						"HFS File System"
#define HFS_PLUS_NAME					"HFS+ File System"
#define ISO9660_FS_NAME					"ISO9660 File System"
#define REISER_FS_NAME					"Reiser File System"
#define UDF_FS_NAME						"UDF File System"


#endif	// _SYSTEM_DISK_DEVICE_TYPES_H
