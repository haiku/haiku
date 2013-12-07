/*
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_PARTITIONS_H
#define KERNEL_BOOT_PARTITIONS_H


#include <boot/vfs.h>
#include <disk_device_manager.h>


struct file_system_module_info;

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

		status_t Mount(Directory **_fileSystem = NULL, bool isBootDevice = false);
		status_t Scan(bool mountFileSystems, bool isBootDevice = false);

		static Partition *Lookup(partition_id id, NodeList *list = NULL);

		void SetParent(Partition *parent);
		Partition *Parent() const;

		bool IsFileSystem() const { return fIsFileSystem; }
		bool IsPartitioningSystem() const { return fIsPartitioningSystem; }
		const char *ModuleName() const { return fModuleName; }

		int FD() const { return fFD; }

	private:
		status_t _Mount(file_system_module_info *module, Directory **_fileSystem);

		int			fFD;
		NodeList	fChildren;
		Partition	*fParent;
		bool		fIsFileSystem, fIsPartitioningSystem;
		const char	*fModuleName;
};

}	// namespace boot

// DiskDeviceTypes we need/support in the boot loader
#define kPartitionTypeAmiga		"Amiga RDB"
#define kPartitionTypeIntel		"Intel Partition Map"
#define kPartitionTypeIntelExtended "Intel Extended Partition"
	// Note: The naming of these two at least must be consistent with
	// DiskDeviceTypes.cpp.
#define kPartitionTypeApple		"Apple"

#define kPartitionTypeBFS		"BFS Filesystem"
#define kPartitionTypeAmigaFFS	"AmigaFFS Filesystem"
#define kPartitionTypeBTRFS		"BTRFS Filesystem"
#define kPartitionTypeEXFAT		"exFAT Filesystem"
#define kPartitionTypeEXT2		"EXT2 Filesystem"
#define kPartitionTypeEXT3		"EXT3 Filesystem"
#define kPartitionTypeFAT12		"FAT12 Filesystem"
#define kPartitionTypeFAT32		"FAT32 Filesystem"
#define kPartitionTypeHFS		"HFS Filesystem"
#define kPartitionTypeHFSPlus	"HFS+ Filesystem"
#define kPartitionTypeISO9660	"ISO9660 Filesystem"
#define kPartitionTypeReiser	"Reiser Filesystem"
#define kPartitionTypeTarFS		"TAR Filesystem"
#define kPartitionTypeUDF		"UDF Filesystem"

// structure definitions as used in the boot loader
struct partition_module_info;
extern partition_module_info gAmigaPartitionModule;
extern partition_module_info gApplePartitionModule;
extern partition_module_info gEFIPartitionModule;
extern partition_module_info gIntelPartitionMapModule;
extern partition_module_info gIntelExtendedPartitionModule;

// the file system module info is not a standard module info;
// their modules are specifically written for the boot loader,
// and hence, don't need to follow the standard module specs.

struct file_system_module_info {
	const char	*module_name;
	const char	*pretty_name;
	float		(*identify_file_system)(boot::Partition *device);
	status_t	(*get_file_system)(boot::Partition *device, Directory **_root);
};

extern file_system_module_info gBFSFileSystemModule;
extern file_system_module_info gFATFileSystemModule;
extern file_system_module_info gHFSPlusFileSystemModule;
extern file_system_module_info gAmigaFFSFileSystemModule;
extern file_system_module_info gTarFileSystemModule;

#endif	/* KERNEL_BOOT_PARTITIONS_H */
