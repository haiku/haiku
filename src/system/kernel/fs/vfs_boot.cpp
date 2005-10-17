/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <OS.h>
#include <fs_info.h>

#include <disk_device_manager/KDiskDevice.h>
#include <disk_device_manager/KDiskDeviceManager.h>
#include <disk_device_manager/KPartitionVisitor.h>
#include <DiskDeviceTypes.h>

#include <vfs.h>
#include <file_cache.h>
#include <KPath.h>
#include <syscalls.h>
#include <boot/kernel_args.h>
#include <util/Stack.h>

#include <stdio.h>


//#define TRACE_VFS
#ifdef TRACE_VFS
#	define PRINT(x) dprintf x
#	define FUNCTION(x) dprintf x
#else
#	define PRINT(x) ;
#	define FUNCTION(x) ;
#endif


typedef Stack<KPartition *> PartitionStack;

static struct {
	const char *path;
	const char *target;
} sPredefinedLinks[] = {
	{"/system", "/boot/beos/system"},
	{"/bin", "/boot/beos/bin"},
	{"/etc", "/boot/beos/etc"},
	{"/var", "/boot/var"},
	{"/tmp", "/boot/var/tmp"},
	{NULL}
};

// This can be used by other code to see if there is a boot file system already
dev_t gBootDevice = -1;


static status_t
get_boot_partitions(kernel_args *args, PartitionStack &partitions)
{
	// make the boot partition (and probably others) available
	KDiskDeviceManager::CreateDefault();
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();

	status_t status = manager->InitialDeviceScan();
	if (status == B_OK) {
		// ToDo: do this for real... (no hacks allowed :))
		for (;;) {
			snooze(500000);
			if (manager->CountJobs() == 0)
				break;
		}
	} else {
		dprintf("KDiskDeviceManager::InitialDeviceScan() failed: %s\n", strerror(status));
		return status;
	}

	// ToDo: do this for real! It will currently only use the partition offset;
	//	it does not yet use the disk_identifier information.

	struct BootPartitionVisitor : KPartitionVisitor {
		BootPartitionVisitor(kernel_args &args, PartitionStack &stack)
			: fArgs(args), fPartitions(stack) {}

		virtual bool VisitPre(KPartition *partition)
		{
			if (!partition->ContainsFileSystem())
				return false;

			if (!fArgs.boot_disk.booted_from_image) {
				// the simple case: we can just boot from the selected boot device
				if (partition->Offset() == fArgs.boot_disk.partition_offset) {
					fPartitions.Push(partition);
					return true;
				}
			} else {
				// for now, we will just collect all BFS volumes
				if (fArgs.boot_disk.cd && fArgs.boot_disk.user_selected
					&& partition->Type() != NULL
					&& strcmp(partition->Type(), kPartitionTypeDataSession))
					return false;

				if (partition->ContentType() != NULL
					&& !strcmp(partition->ContentType(), "Be File System"))
					fPartitions.Push(partition);
			}
			return false;
		}
		private:
			kernel_args		&fArgs;
			PartitionStack	&fPartitions;
	} visitor(*args, partitions);

	KDiskDevice *device;
	int32 cookie = 0;
	while ((device = manager->NextDevice(&cookie)) != NULL) {
		if (device->VisitEachDescendant(&visitor) != NULL)
			break;
	}

	// ToDo: sort partition list (ie. when booting from CD, CDs should come first in the list)
	return B_OK;
}


//	#pragma mark -


status_t
vfs_bootstrap_file_systems(void)
{
	status_t status;

	// bootstrap the root filesystem
	status = _kern_mount("/", NULL, "rootfs", 0, NULL);
	if (status < B_OK)
		panic("error mounting rootfs!\n");

	_kern_setcwd(-1, "/");

	// bootstrap the devfs
	_kern_create_dir(-1, "/dev", 0755);
	status = _kern_mount("/dev", NULL, "devfs", 0, NULL);
	if (status < B_OK)
		panic("error mounting devfs\n");

	// bootstrap the pipefs
	_kern_create_dir(-1, "/pipe", 0755);
	status = _kern_mount("/pipe", NULL, "pipefs", 0, NULL);
	if (status < B_OK)
		panic("error mounting pipefs\n");

	// bootstrap the bootfs (if possible)
	_kern_create_dir(-1, "/boot", 0755);
	status = _kern_mount("/boot", NULL, "bootfs", 0, NULL);
	if (status < B_OK) {
		// this is no fatal exception at this point, as we may mount
		// a real on disk file system later
		dprintf("Can't mount bootfs (will try disk file system later)\n");
	}

	// create some standard links on the rootfs

	for (int32 i = 0; sPredefinedLinks[i].path != NULL; i++) {
		_kern_create_symlink(-1, sPredefinedLinks[i].path,
			sPredefinedLinks[i].target, 0);
			// we don't care if it will succeed or not
	}

	return B_OK;
}


status_t
vfs_mount_boot_file_system(kernel_args *args)
{
	PartitionStack partitions;
	status_t status = get_boot_partitions(args, partitions);
	if (status < B_OK)
		return status;

	KPartition *bootPartition;
	while (partitions.Pop(&bootPartition)) {
		KPath path;
		if (bootPartition->GetPath(&path) != B_OK)
			panic("could not get boot device!\n");

		gBootDevice = _kern_mount("/boot", path.Path(), NULL, 0, NULL);
		if (gBootDevice >= B_OK)
			break;
	}

	if (gBootDevice < B_OK)
		panic("could not mount boot device!\n");

	// create link for the name of the boot device

	fs_info info;
	if (_kern_read_fs_info(gBootDevice, &info) == B_OK) {
		char path[B_FILE_NAME_LENGTH + 1];
		snprintf(path, sizeof(path), "/%s", info.volume_name);

		_kern_create_symlink(-1, path, "/boot", 0);
	}

	file_cache_init_post_boot_device();
	return B_OK;
}

