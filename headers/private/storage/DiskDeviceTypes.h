//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _DISK_DEVICE_TYPES_H
#define _DISK_DEVICE_TYPES_H

// Device Types

const char *kDeviceTypeFloppyDisk	= "Floppy Disk Media";
const char *kDeviceTypeHardDisk		= "Hard Disk Media";
const char *kDeviceTypeOptical		= "Optical Media";

// Partition types

const char *kPartitionTypeUnrecognized	= "Unrecognized";

const char *kPartitionTypeMultisession	= "Multisession Storage Device";

const char *kPartitionTypeCDDA			= "Compact Disc Audio Session";

const char *kPartitionTypeAmiga			= "Amiga Partition Map";
const char *kPartitionTypeApple			= "Apple Partition Map";
const char *kPartitionTypeIntel			= "Intel Partition Map";
const char *kPartitionTypeIntelExtended	= "Intel Extended Partition";

const char *kPartitionTypeAmigaFFS		= "AmigaFFS Filesystem";
const char *kPartitionTypeBFS			= "BFS Filesystem";
const char *kPartitionTypeEXT2			= "EXT2 Filesystem";
const char *kPartitionTypeEXT3			= "EXT3 Filesystem";
const char *kPartitionTypeFAT12			= "FAT12 Filesystem";
const char *kPartitionTypeFAT32			= "FAT32 Filesystem";
const char *kPartitionTypeISO9660		= "ISO9660 Filesystem";
const char *kPartitionTypeReiser		= "Reiser Filesystem";
const char *kPartitionTypeUDF			= "UDF Filesystem";

#endif // _DISK_DEVICE_TYPES_H
