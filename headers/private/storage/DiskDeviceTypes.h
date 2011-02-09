/*
 * Copyright 2003-2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DISK_DEVICE_TYPES_H
#define _DISK_DEVICE_TYPES_H

// Device Types

extern const char *kDeviceTypeFloppyDisk;
extern const char *kDeviceTypeHardDisk;
extern const char *kDeviceTypeOptical;

// Partition types

extern const char *kPartitionTypeUnrecognized;

extern const char *kPartitionTypeMultisession;
extern const char *kPartitionTypeAudioSession;
extern const char *kPartitionTypeDataSession;

extern const char *kPartitionTypeAmiga;
extern const char *kPartitionTypeApple;
extern const char *kPartitionTypeEFI;
extern const char *kPartitionTypeIntel;
extern const char *kPartitionTypeIntelExtended;
extern const char *kPartitionTypeVMDK;

extern const char *kPartitionTypeAmigaFFS;
extern const char *kPartitionTypeBFS;
extern const char *kPartitionTypeBTRFS;
extern const char *kPartitionTypeEXFAT;
extern const char *kPartitionTypeEXT2;
extern const char *kPartitionTypeEXT3;
extern const char *kPartitionTypeFAT12;
extern const char *kPartitionTypeFAT32;
extern const char *kPartitionTypeHFS;
extern const char *kPartitionTypeHFSPlus;
extern const char *kPartitionTypeISO9660;
extern const char *kPartitionTypeReiser;
extern const char *kPartitionTypeUDF;

#endif // _DISK_DEVICE_TYPES_H
