//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _DISK_DEVICE_TYPES_H
#define _DISK_DEVICE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

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
extern const char *kPartitionTypeIntel;

extern const char *kPartitionTypeIntelPrimary;
extern const char *kPartitionTypeIntelExtended;
extern const char *kPartitionTypeIntelLogical;

extern const char *kPartitionTypeAmigaFFS;
extern const char *kPartitionTypeBFS;
extern const char *kPartitionTypeEXT2;
extern const char *kPartitionTypeEXT3;
extern const char *kPartitionTypeFAT12;
extern const char *kPartitionTypeFAT32;
extern const char *kPartitionTypeISO9660;
extern const char *kPartitionTypeReiser;
extern const char *kPartitionTypeUDF;

#ifdef __cplusplus
}
#endif

#endif // _DISK_DEVICE_TYPES_H
