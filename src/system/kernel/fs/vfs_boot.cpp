/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include "vfs_boot.h"

#include <stdio.h>

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
#include <util/KMessage.h>
#include <util/Stack.h>

#include "vfs_net_boot.h"


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
int
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


// #pragma mark - BootMethod


BootMethod::BootMethod(const KMessage& bootVolume, int32 method)
	: fBootVolume(bootVolume),
		fMethod(method)
{
}


BootMethod::~BootMethod()
{
}


status_t
BootMethod::Init()
{
	return B_OK;
}


// #pragma mark - DiskBootMethod


class DiskBootMethod : public BootMethod {
public:
	DiskBootMethod(const KMessage& bootVolume, int32 method)
		: BootMethod(bootVolume, method)
	{
	}

	virtual bool IsBootDevice(KDiskDevice* device, bool strict);
	virtual bool IsBootPartition(KPartition* partition, bool& foundForSure);
	virtual void SortPartitions(KPartition** partitions, int32 count);
};


bool
DiskBootMethod::IsBootDevice(KDiskDevice* device, bool strict)
{
	disk_identifier *disk;
	int32 diskIdentifierSize;
	if (fBootVolume.FindData(BOOT_VOLUME_DISK_IDENTIFIER, B_RAW_TYPE,
			(const void**)&disk, &diskIdentifierSize) != B_OK) {
		dprintf("DiskBootMethod::IsBootDevice(): no disk identifier!\n");
		return false;
	}

	TRACE(("boot device: bus %ld, device %ld\n", disk->bus_type,
		disk->device_type));

	switch (disk->bus_type) {
		case PCI_BUS:
		case LEGACY_BUS:
			// TODO: implement this! (and then enable this feature in the boot loader)
			// (we need a way to get the device_node of a device, then)
			break;

		case UNKNOWN_BUS:
			// nothing to do here
			break;
	}

	switch (disk->device_type) {
		case UNKNOWN_DEVICE:
			// test if the size of the device matches
			// (the BIOS might have given us the wrong value here, though)
			if (strict && device->Size() != disk->device.unknown.size)
				return false;

			// check if the check sums match, too
			for (int32 i = 0; i < NUM_DISK_CHECK_SUMS; i++) {
				if (disk->device.unknown.check_sums[i].offset == -1)
					continue;

				if (compute_check_sum(device,
						disk->device.unknown.check_sums[i].offset)
							!= disk->device.unknown.check_sums[i].sum) {
					return false;
				}
			}
			break;

		case ATA_DEVICE:
		case ATAPI_DEVICE:
		case SCSI_DEVICE:
		case USB_DEVICE:
		case FIREWIRE_DEVICE:
		case FIBRE_DEVICE:
			// TODO: implement me!
			break;
	}

	return true;
}


bool
DiskBootMethod::IsBootPartition(KPartition* partition, bool& foundForSure)
{
	if (!fBootVolume.GetBool(BOOT_VOLUME_BOOTED_FROM_IMAGE, false)) {
		// the simple case: we can just boot from the selected boot
		// device
		if (partition->Offset() == fBootVolume.GetBool(
				BOOT_VOLUME_PARTITION_OFFSET, 0)) {
			foundForSure = true;
			return true;
		}
	} else {
		// for now, we will just collect all BFS volumes
		if (fMethod == BOOT_METHOD_CD
			&& fBootVolume.GetBool(BOOT_VOLUME_USER_SELECTED, false)
			&& partition->Type() != NULL
			&& strcmp(partition->Type(), kPartitionTypeDataSession)) {
			return false;
		}

		if (partition->ContentType() != NULL
			&& !strcmp(partition->ContentType(), "Be File System")) {
			return true;
		}
	}

	return false;
}


void
DiskBootMethod::SortPartitions(KPartition** partitions, int32 count)
{
	qsort(partitions, count, sizeof(KPartition *),
		fMethod == BOOT_METHOD_CD ? compare_cd_boot : compare_image_boot);
}


// #pragma mark -


/**	Make the boot partition (and probably others) available. 
 *	The partitions that are a boot candidate a put into the /a partitions
 *	stack. If the user selected a boot device, there is will only be one
 *	entry in this stack; if not, the most likely is put up first.
 *	The boot code should then just try them one by one.
 */
static status_t
get_boot_partitions(kernel_args *args, PartitionStack &partitions)
{
	const KMessage& bootVolume = args->boot_volume;

	dprintf("get_boot_partitions(): boot volume message:\n");
	KMessageField field;
	while (bootVolume.GetNextField(&field) == B_OK) {
		type_code type = field.TypeCode();
		uint32 bigEndianType = B_HOST_TO_BENDIAN_INT32(type);
		dprintf("field: \"%s\", type: %.4s (0x%lx):\n", field.Name(),
			(char*)&bigEndianType, type);

		if (field.CountElements() == 0)
			dprintf("\n");

		int32 size;
		for (int i = 0; const void* data = field.ElementAt(i, &size); i++) {
			dprintf("  [%2d] ", i);
			bool isIntType = false;
			int64 intData = 0;
			switch (type) {
				case B_BOOL_TYPE:
					dprintf("%s\n", (*(bool*)data ? "true" : "false"));
					break;
				case B_INT8_TYPE:
					isIntType = true;
					intData = *(int8*)data;
					break;
				case B_INT16_TYPE:
					isIntType = true;
					intData = *(int16*)data;
					break;
				case B_INT32_TYPE:
					isIntType = true;
					intData = *(int32*)data;
					break;
				case B_INT64_TYPE:
					isIntType = true;
					intData = *(int64*)data;
					break;
				case B_STRING_TYPE:
					dprintf("\"%s\"\n", (char*)data);
					break;
				default:
					dprintf("data: \"%p\", %ld bytes\n", (char*)data, size);
					break;
			}
			if (isIntType)
				dprintf("%lld (0x%llx)\n", intData, intData);
		}
	}


	// create boot method
	int32 bootMethodType = bootVolume.GetInt32(BOOT_METHOD,
		BOOT_METHOD_DEFAULT);
dprintf("get_boot_partitions(): boot method type: %ld\n", bootMethodType);
	BootMethod* bootMethod = NULL;
	switch (bootMethodType) {
		case BOOT_METHOD_NET:
			bootMethod = new(nothrow) NetBootMethod(bootVolume, bootMethodType);
			break;

		case BOOT_METHOD_HARD_DISK:
		case BOOT_METHOD_CD:
		default:
			bootMethod = new(nothrow) DiskBootMethod(bootVolume,
				bootMethodType);
			break;
	}

	status_t status = bootMethod->Init();
	if (status != B_OK)
		return status;

	KDiskDeviceManager::CreateDefault();
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();

	status = manager->InitialDeviceScan();
	if (status != B_OK) {
		dprintf("KDiskDeviceManager::InitialDeviceScan() failed: %s\n", strerror(status));
		return status;
	}

	struct BootPartitionVisitor : KPartitionVisitor {
		BootPartitionVisitor(BootMethod* bootMethod, PartitionStack &stack)
			: fPartitions(stack),
			  fBootMethod(bootMethod)
		{
		}

		virtual bool VisitPre(KPartition *partition)
		{
			if (!partition->ContainsFileSystem())
				return false;

			bool foundForSure = false;
			if (fBootMethod->IsBootPartition(partition, foundForSure))
				fPartitions.Push(partition);

			// if found for sure, we can terminate the search
			return foundForSure;
		}

		private:
			PartitionStack	&fPartitions;
			BootMethod*		fBootMethod;
	} visitor(bootMethod, partitions);

	bool strict = true;

	while (true) {
		KDiskDevice *device;
		int32 cookie = 0;
		while ((device = manager->NextDevice(&cookie)) != NULL) {
			if (!bootMethod->IsBootDevice(device, strict))
				continue;

			if (device->VisitEachDescendant(&visitor) != NULL)
				break;
		}

		if (!partitions.IsEmpty() || !strict)
			break;

		// we couldn't find any potential boot devices, try again less strict
		strict = false;
	}

	// sort partition list (e.g.. when booting from CD, CDs should come first in
	// the list)
	if (!args->boot_volume.GetBool(BOOT_VOLUME_USER_SELECTED, false))
		bootMethod->SortPartitions(partitions.Array(), partitions.CountItems());

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

