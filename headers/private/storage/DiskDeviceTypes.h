//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _DISK_DEVICE_TYPES_H
#define _DISK_DEVICE_TYPES_H

// Device Types

#define B_DEVICE_TYPE_FLOPPY_DISK	"Floppy Disk Media";
#define B_DEVICE_TYPE_HARD_DISK		"Hard Disk Media";
#define B_DEVICE_TYPE_CD			"CD-ROM Media";
#define B_DEVICE_TYPE_DVD			"DVD-ROM Media";

// Partition types

#define B_PARTITION_TYPE_UNFORMATTED	"Unformatted";

#define B_PARTITION_TYPE_MULTISESSION	"Multisession Storage Device";

#define B_PARTITION_TYPE_CD_AUDIO		"Compact Disc Audio Session";

#define B_PARTITION_TYPE_APPLE			"Apple Partition Map";
#define B_PARTITION_TYPE_INTEL			"Intel Partition Map";
#define B_PARTITION_TYPE_INTEL_EXTENDED	"Intel Extended Partition";

#define B_PARTITION_TYPE_BFS			"BFS Filesystem";
#define B_PARTITION_TYPE_EXT2			"EXT2 Filesystem";
#define B_PARTITION_TYPE_EXT3			"EXT3 Filesystem";
#define B_PARTITION_TYPE_FAT12			"FAT12 Filesystem";
#define B_PARTITION_TYPE_FAT32			"FAT32 Filesystem";
#define B_PARTITION_TYPE_ISO9660		"ISO9660 Filesystem";
#define B_PARTITION_TYPE_REISER			"Reiser Filesystem";
#define B_PARTITION_TYPE_UDF			"UDF Filesystem";

#endif // _DISK_DEVICE_TYPES_H
