/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef KERNEL_BOOT_PARTITIONS_H
#define KERNEL_BOOT_PARTITIONS_H


#include <boot/vfs.h>
#include <disk_device_manager.h>


// DiskDeviceTypes we need/support in the boot loader
#define kPartitionTypeAmiga "Amiga RDB"
#define kPartitionTypeIntel "Intel"
#define kPartitionTypeIntelExtended "Intel Extended"
#define kPartitionTypeApple "Apple"

#define kPartitionTypeBFS "BFS"

struct partition_module_info;
extern partition_module_info gAmigaPartitionModule;
extern partition_module_info gIntelPartitionMapModule;
extern partition_module_info gIntelExtendedPartitionModule;
extern partition_module_info gApplePartitionModule;

#endif	/* KERNEL_BOOT_PARTITIONS_H */
