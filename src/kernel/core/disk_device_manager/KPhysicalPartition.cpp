// KPhysicalPartition.cpp

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Drivers.h>
#include <Errors.h>
#include <util/kernel_cpp.h>
#include <devfs.h>

#include "KDiskDevice.h"
#include "KDiskDeviceManager.h"
#include "KDiskDeviceUtils.h"
#include "KPath.h"
#include "KPhysicalPartition.h"
#include "KShadowPartition.h"

using namespace std;

// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT dprintf

// constructor
KPhysicalPartition::KPhysicalPartition(partition_id id)
	: KPartition(id),
	  fShadowPartition(NULL)
{
}

// destructor
KPhysicalPartition::~KPhysicalPartition()
{
}

// Register
// PrepareForRemoval
bool
KPhysicalPartition::PrepareForRemoval()
{
	bool result = KPartition::PrepareForRemoval();
	if (result) {
		UnsetShadowPartition(false);
		UnpublishDevice();
	}
	return result;
}

// Open
status_t
KPhysicalPartition::Open(int flags, int *fd)
{
	if (!fd)
		return B_BAD_VALUE;
	// get the path
	KPath path;
	status_t error = GetPath(&path);
	if (error != B_OK)
		return error;
	// open the device
	*fd = open(path.Path(), flags);
	if (*fd < 0)
		return errno;
	return B_OK;
}

// PublishDevice
status_t
KPhysicalPartition::PublishDevice()
{
	// get the path
	KPath path;
	status_t error = GetPath(&path);
	if (error != B_OK)
		return error;

	// prepare a partition_info
	partition_info info;
	info.offset = Offset();
	info.size = Size();
	info.logical_block_size = BlockSize();
	info.session = 0;
	info.partition = ID();
	if (strlcpy(info.device, Device()->Path(), B_PATH_NAME_LENGTH)
		>= B_PATH_NAME_LENGTH) {
		return B_NAME_TOO_LONG;
	}

	return devfs_publish_partition(path.Path(), &info);
}

// UnpublishDevice
status_t
KPhysicalPartition::UnpublishDevice()
{
	// get the path
	KPath path;
	status_t error = GetPath(&path);
	if (error != B_OK)
		return error;

	return devfs_unpublish_partition(path.Path());
}

// Mount
status_t
KPhysicalPartition::Mount(uint32 mountFlags, const char *parameters)
{
	// not implemented
	return B_ERROR;
}

// Unmount
status_t
KPhysicalPartition::Unmount()
{
	// not implemented
	return B_ERROR;
}

// CreateChild
status_t
KPhysicalPartition::CreateChild(partition_id id, int32 index,
								KPartition **_child)
{
	// check parameters
	int32 count = fPartitionData.child_count;
	if (index == -1)
		index = count;
	if (index < 0 || index > count)
		return B_BAD_VALUE;
	// create and add partition
	KPhysicalPartition *child = new(nothrow) KPhysicalPartition(id);
	if (!child)
		return B_NO_MEMORY;
	status_t error = AddChild(child, index);
	// cleanup / set result
	if (error != B_OK)
		delete child;
	else if (_child)
		*_child = child;
	return error;
}

// CreateShadowPartition
status_t
KPhysicalPartition::CreateShadowPartition()
{
	if (fShadowPartition)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		// create shadow partition
		fShadowPartition = new(nothrow) KShadowPartition(this);
		if (!fShadowPartition)
			return B_NO_MEMORY;
		// make it known to the manager
		if (!manager->PartitionAdded(fShadowPartition)) {
			delete fShadowPartition;
			fShadowPartition = NULL;
			return B_NO_MEMORY;
		}
	}
	// create shadows for children
	for (int32 i = 0; KPartition *child = ChildAt(i); i++) {
		status_t error = child->CreateShadowPartition();
		if (error == B_OK)
			error = fShadowPartition->AddChild(child->ShadowPartition(), i);
		// cleanup on error
		if (error != B_OK) {
			for (int32 k = 0; k <= i; i++)
				ChildAt(k)->UnsetShadowPartition(true);
			UnsetShadowPartition(true);
			return error;
		}
	}
	return B_OK;
}

// UnsetShadowPartition
void
KPhysicalPartition::UnsetShadowPartition(bool doDelete)
{
	if (!fShadowPartition)
		return;
	// unset children's shadows
	for (int32 i = 0; KPartition *child = ChildAt(i); i++)
		child->UnsetShadowPartition(false);
	// unset the thing
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		fShadowPartition->UnsetPhysicalPartition();
		if (doDelete) {
			PartitionRegistrar _(fShadowPartition);
			manager->PartitionRemoved(fShadowPartition);
		}
		fShadowPartition = NULL;
	}
}

// ShadowPartition
KShadowPartition *
KPhysicalPartition::ShadowPartition() const
{
	return fShadowPartition;
}

// IsShadowPartition
bool
KPhysicalPartition::IsShadowPartition() const
{
	return false;
}

// PhysicalPartition
KPhysicalPartition *
KPhysicalPartition::PhysicalPartition() const
{
	return NULL;
}

// Dump
void
KPhysicalPartition::Dump(bool deep, int32 level)
{
	KPartition::Dump(deep, level);
}

