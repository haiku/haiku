/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de.
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
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
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


/**	No image was chosen - prefer disks with names like "Haiku", or "System"
 */

static int
compare_image_boot(const void *_a, const void *_b)
{
	KPartition *a = *(KPartition **)_a;
	KPartition *b = *(KPartition **)_b;

	if (a->ContentName() != NULL) {
		if (b->ContentName() == NULL)
			return 1;
	} else if (b->ContentName() != NULL) {
		return -1;
	} else
		return 0;

	int compare = strcmp(a->ContentName(), b->ContentName());
	if (!compare)
		return 0;

	if (!strcasecmp(a->ContentName(), "Haiku"))
		return 1;
	if (!strcasecmp(b->ContentName(), "Haiku"))
		return -1;
	if (!strncmp(a->ContentName(), "System", 6))
		return 1;
	if (!strncmp(b->ContentName(), "System", 6))
		return -1;

	return compare;	
}


/**	The system was booted from CD - prefer CDs over other entries. If there
 *	is no CD, fall back to the standard mechanism (as implemented by
 *	compare_image_boot().
 */

static int
compare_cd_boot(const void *_a, const void *_b)
{
	KPartition *a = *(KPartition **)_a;
	KPartition *b = *(KPartition **)_b;

	bool aIsCD = a->Type() != NULL && !strcmp(a->Type(), kPartitionTypeDataSession);
	bool bIsCD = b->Type() != NULL && !strcmp(b->Type(), kPartitionTypeDataSession);

	int compare = (int)aIsCD - (int)bIsCD;
	if (compare != 0)
		return compare;

	return compare_image_boot(_a, _b);
}


/**	Computes a check sum for the specified block.
 *	The check sum is the sum of all data in that block interpreted as an
 *	array of uint32 values.
 *	Note, this must use the same method as the one used in
 *	boot/platform/bios_ia32/devices.cpp (or similar solutions).
 */

static uint32
compute_check_sum(KDiskDevice *device, off_t offset)
{
	char buffer[512];
	ssize_t bytesRead = read_pos(device->FD(), offset, buffer, sizeof(buffer));
	if (bytesRead < B_OK)
		return 0;

	if (bytesRead < (ssize_t)sizeof(buffer))
		memset(buffer + bytesRead, 0, sizeof(buffer) - bytesRead);

	uint32 *array = (uint32 *)buffer;
	uint32 sum = 0;

	for (uint32 i = 0; i < (bytesRead + sizeof(uint32) - 1) / sizeof(uint32); i++) {
		sum += array[i];
	}

	return sum;
}


/**	Checks if the device matches the boot device as specified by the
 *	boot loader.
 */

static bool
is_boot_device(kernel_args *args, KDiskDevice *device, bool strict)
{
	disk_identifier &disk = args->boot_disk.identifier;

	TRACE(("boot device: bus %ld, device %ld\n", disk.bus_type,
		disk.device_type));

	switch (disk.bus_type) {
		case PCI_BUS:
		case LEGACY_BUS:
			// TODO: implement this! (and then enable this feature in the boot loader)
			// (we need a way to get the device_node of a device, then)
			break;

		case UNKNOWN_BUS:
			// nothing to do here
			break;
	}

	switch (disk.device_type) {
		case UNKNOWN_DEVICE:
			// test if the size of the device matches
			// (the BIOS might have given us the wrong value here, though)
			if (strict && device->Size() != disk.device.unknown.size)
				return false;

			// check if the check sums match, too
			for (int32 i = 0; i < NUM_DISK_CHECK_SUMS; i++) {
				if (disk.device.unknown.check_sums[i].offset == -1)
					continue;

				if (compute_check_sum(device, disk.device.unknown.check_sums[i].offset)
						!= disk.device.unknown.check_sums[i].sum)
					return false;
			}
			break;

		case ATA_DEVICE:
		case ATAPI_DEVICE:
		case SCSI_DEVICE:
		case USB_DEVICE:
		case FIREWIRE_DEVICE:
		case FIBRE_DEVICE:
		case NETWORK_DEVICE:
			// TODO: implement me!
			break;
	}

	return true;
}


/**	Make the boot partition (and probably others) available. 
 *	The partitions that are a boot candidate a put into the /a partitions
 *	stack. If the user selected a boot device, there is will only be one
 *	entry in this stack; if not, the most likely is put up first.
 *	The boot code should then just try them one by one.
 */

static status_t
get_boot_partitions(kernel_args *args, PartitionStack &partitions)
{
	KDiskDeviceManager::CreateDefault();
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();

	status_t status = manager->InitialDeviceScan();
	if (status != B_OK) {
		dprintf("KDiskDeviceManager::InitialDeviceScan() failed: %s\n", strerror(status));
		return status;
	}

	if (args->boot_disk.booted_from_network) {
		panic("get_boot_partitions: boot from network, server %08lx, client %08lx\n",
			args->boot_disk.identifier.device.network.server_ip,
			args->boot_disk.identifier.device.network.client_ip);
	}

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

	bool strict = true;

	while (true) {
		KDiskDevice *device;
		int32 cookie = 0;
		while ((device = manager->NextDevice(&cookie)) != NULL) {
			if (!is_boot_device(args, device, strict))
				continue;

			if (device->VisitEachDescendant(&visitor) != NULL)
				break;
		}

		if (!partitions.IsEmpty() || !strict)
			break;

		// we couldn't find any potential boot devices, try again less strict
		strict = false;
	}

	if (!args->boot_disk.user_selected) {
		// sort partition list (ie. when booting from CD, CDs should come first in the list)
		qsort(partitions.Array(), partitions.CountItems(), sizeof(KPartition *),
			args->boot_disk.cd ? compare_cd_boot : compare_image_boot);
	}

	return B_OK;
}


//	#pragma mark -


status_t
vfs_bootstrap_file_systems(void)
{
	status_t status;

	// bootstrap the root filesystem
	status = _kern_mount("/", NULL, "rootfs", 0, NULL, 0);
	if (status < B_OK)
		panic("error mounting rootfs!\n");

	_kern_setcwd(-1, "/");

	// bootstrap the devfs
	_kern_create_dir(-1, "/dev", 0755);
	status = _kern_mount("/dev", NULL, "devfs", 0, NULL, 0);
	if (status < B_OK)
		panic("error mounting devfs\n");

	// bootstrap the pipefs
	_kern_create_dir(-1, "/pipe", 0755);
	status = _kern_mount("/pipe", NULL, "pipefs", 0, NULL, 0);
	if (status < B_OK)
		panic("error mounting pipefs\n");

	// create directory for the boot volume
	_kern_create_dir(-1, "/boot", 0755);

	// create some standard links on the rootfs

	for (int32 i = 0; sPredefinedLinks[i].path != NULL; i++) {
		_kern_create_symlink(-1, sPredefinedLinks[i].path,
			sPredefinedLinks[i].target, 0);
			// we don't care if it will succeed or not
	}

	return B_OK;
}


void
vfs_mount_boot_file_system(kernel_args *args)
{
	PartitionStack partitions;
	status_t status = get_boot_partitions(args, partitions);
	if (status < B_OK) {
		panic("get_boot_partitions failed!");
	}
	if (partitions.IsEmpty()) {
		panic("did not find any boot partitions!");
	}

	KPartition *bootPartition;
	while (partitions.Pop(&bootPartition)) {
		KPath path;
		if (bootPartition->GetPath(&path) != B_OK)
			panic("could not get boot device!\n");

		TRACE(("trying to mount boot partition: %s\n", path.Path()));
		gBootDevice = _kern_mount("/boot", path.Path(), NULL, 0, NULL, 0);
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

	// search for other disk systems
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	manager->RescanDiskSystems();
}

