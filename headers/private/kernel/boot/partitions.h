/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef KERNEL_BOOT_PARTITIONS_H
#define KERNEL_BOOT_PARTITIONS_H


#include <boot/vfs.h>
#include <disk_device_manager.h>


namespace boot {

class Partition : public Node, public partition_data {
	public:
		Partition(int deviceFD);
		virtual ~Partition();

		virtual ssize_t ReadAt(void *cookie, off_t offset, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t offset, const void *buffer, size_t bufferSize);

		virtual off_t Size() const;
		virtual int32 Type() const;

		Partition *AddChild();

		status_t Mount(Directory **_fileSystem = NULL);
		status_t Scan(bool mountFileSystems);

		Partition *Parent() const { return fParent; }
		bool IsFileSystem() const { return fIsFileSystem; }

	private:
		void SetParent(Partition *parent) { fParent = parent; }

		int			fFD;
		NodeList	fChildren;
		Partition	*fParent;
		bool		fIsFileSystem;
};

}	// namespace boot

// DiskDeviceTypes we need/support in the boot loader
#define kPartitionTypeAmiga		"Amiga RDB"
#define kPartitionTypeIntel		"Intel"
#define kPartitionTypeIntelExtended "Intel Extended"
#define kPartitionTypeApple		"Apple"

#define kPartitionTypeBFS		"BFS Filesystem"
#define kPartitionTypeAmigaFFS	"AmigaFFS Filesystem"
#define kPartitionTypeBFS		"BFS Filesystem"
#define kPartitionTypeEXT2		"EXT2 Filesystem"
#define kPartitionTypeEXT3		"EXT3 Filesystem"
#define kPartitionTypeFAT12		"FAT12 Filesystem"
#define kPartitionTypeFAT32		"FAT32 Filesystem"
#define kPartitionTypeISO9660	"ISO9660 Filesystem"
#define kPartitionTypeReiser	"Reiser Filesystem"
#define kPartitionTypeUDF		"UDF Filesystem"

// structure definitions as used in the boot loader
struct partition_module_info;
extern partition_module_info gAmigaPartitionModule;
extern partition_module_info gIntelPartitionMapModule;
extern partition_module_info gIntelExtendedPartitionModule;
extern partition_module_info gApplePartitionModule;

// the file system module info is not a standard module info;
// their modules are specifically written for the boot loader,
// and hence, don't need to follow the standard module specs.

struct file_system_module_info {
	const char	*pretty_name;
	status_t	(*get_file_system)(boot::Partition *device, Directory **_root);
};

extern file_system_module_info gBFSFileSystemModule;
extern file_system_module_info gAmigaFFSFileSystemModule;

#endif	/* KERNEL_BOOT_PARTITIONS_H */
