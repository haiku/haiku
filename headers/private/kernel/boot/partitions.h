/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef KERNEL_BOOT_PARTITIONS_H
#define KERNEL_BOOT_PARTITIONS_H


#include <boot/vfs.h>
#include <disk_device_manager.h>


class Partition : public partition_data, Node {
	public:
		Partition(int deviceFD);
		virtual ~Partition();

		virtual ssize_t ReadAt(void *cookie, off_t offset, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t offset, const void *buffer, size_t bufferSize);

	private:
		int		fFD;
};

// DiskDeviceTypes we need/support in the boot loader
#define kPartitionTypeAmiga "Amiga RDB"
#define kPartitionTypeIntel "Intel"
#define kPartitionTypeApple "Apple"

struct partition_module_info;
extern partition_module_info gAmigaPartitionModule;
extern partition_module_info gIntelPartitionModule;
extern partition_module_info gApplePartitionModule;

#endif	/* KERNEL_BOOT_PARTITIONS_H */
