// KShadowPartition.cpp

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Drivers.h>
#include <Errors.h>
#include <util/kernel_cpp.h>

#include "ddm_userland_interface.h"
#include "KDiskDevice.h"
#include "KDiskDeviceManager.h"
#include "KDiskDeviceUtils.h"
#include "KShadowPartition.h"

using namespace std;

// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT dprintf

// constructor
KShadowPartition::KShadowPartition(KPhysicalPartition *partition)
	: KPartition(),
	  KPartitionListener(),
	  fPhysicalPartition(partition)
{
	SyncWithPhysicalPartition();
	if (fPhysicalPartition)
		fPhysicalPartition->AddListener(this);
}

// destructor
KShadowPartition::~KShadowPartition()
{
	UnsetPhysicalPartition();
}

// CreateChild
status_t
KShadowPartition::CreateChild(partition_id id, int32 index,
							  KPartition **_child)
{
	// check parameters
	int32 count = fPartitionData.child_count;
	if (index == -1)
		index = count;
	if (index < 0 || index > count)
		return B_BAD_VALUE;
	// create and add partition
	KShadowPartition *child = new(nothrow) KShadowPartition(NULL);
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

// IsShadowPartition
bool
KShadowPartition::IsShadowPartition() const
{
	return true;
}

// ShadowPartition
KShadowPartition*
KShadowPartition::ShadowPartition() const
{
	return NULL;
}

// UnsetPhysicalPartition
void
KShadowPartition::UnsetPhysicalPartition()
{
	if (fPhysicalPartition) {
		fPhysicalPartition->RemoveListener(this);
		fPhysicalPartition = NULL;
	}
}

// PhysicalPartition
KPhysicalPartition*
KShadowPartition::PhysicalPartition() const
{
	return fPhysicalPartition;
}

// SyncWithPhysicalPartition
void
KShadowPartition::SyncWithPhysicalPartition()
{
	if (!fPhysicalPartition)
		return;
	SetDevice(fPhysicalPartition->Device());
	SetDiskSystem(fPhysicalPartition->DiskSystem());
	SetOffset(fPhysicalPartition->Offset());
	SetSize(fPhysicalPartition->Size());
	SetContentSize(fPhysicalPartition->ContentSize());
	SetBlockSize(fPhysicalPartition->BlockSize());
	SetStatus(fPhysicalPartition->Status());
	SetFlags(fPhysicalPartition->Flags());
	SetName(fPhysicalPartition->Name());
	SetContentName(fPhysicalPartition->ContentName());
	SetType(fPhysicalPartition->Type());
	SetVolumeID(fPhysicalPartition->VolumeID());
	SetParameters(fPhysicalPartition->Parameters());
	SetContentParameters(fPhysicalPartition->ContentParameters());
	// TODO: Cookie, ContentCookie, MountCookie?
}

// WriteUserData
void
KShadowPartition::WriteUserData(UserDataWriter &writer,
								user_partition_data *data)
{
	KPartition::WriteUserData(writer, data);
	// fix the ID in the user data
	if (data) {
		if (fPhysicalPartition)
			data->id = fPhysicalPartition->ID();
		data->shadow_id = ID();
	}
}

// Dump
void
KShadowPartition::Dump(bool deep, int32 level)
{
	KPartition::Dump(deep, level);
}

// OffsetChanged
void
KShadowPartition::OffsetChanged(KPartition *partition, off_t offset)
{
	SetOffset(offset);
}

// SizeChanged
void
KShadowPartition::SizeChanged(KPartition *partition, off_t size)
{
	SetSize(size);
}

// ContentSizeChanged
void
KShadowPartition::ContentSizeChanged(KPartition *partition, off_t size)
{
	SetContentSize(size);
}

// BlockSizeChanged
void
KShadowPartition::BlockSizeChanged(KPartition *partition, uint32 blockSize)
{
	SetBlockSize(blockSize);
}

// IndexChanged
void
KShadowPartition::IndexChanged(KPartition *partition, int32 index)
{
	// should be set automatically
}

// StatusChanged
void
KShadowPartition::StatusChanged(KPartition *partition, uint32 status)
{
	SetStatus(status);
}

// FlagsChanged
void
KShadowPartition::FlagsChanged(KPartition *partition, uint32 flags)
{
	SetFlags(flags);
}

// NameChanged
void
KShadowPartition::NameChanged(KPartition *partition, const char *name)
{
	SetName(name);
}

// ContentNameChanged
void
KShadowPartition::ContentNameChanged(KPartition *partition, const char *name)
{
	SetContentName(name);
}

// TypeChanged
void
KShadowPartition::TypeChanged(KPartition *partition, const char *type)
{
	SetType(type);
}

// IDChanged
void
KShadowPartition::IDChanged(KPartition *partition, partition_id id)
{
	// nothing to do
}

// VolumeIDChanged
void
KShadowPartition::VolumeIDChanged(KPartition *partition, dev_t volumeID)
{
	SetVolumeID(volumeID);
}

// MountCookieChanged
void
KShadowPartition::MountCookieChanged(KPartition *partition, void *cookie)
{
	// TODO: set the mount cookie?
}

// ParametersChanged
void
KShadowPartition::ParametersChanged(KPartition *partition,
									const char *parameters)
{
	SetParameters(parameters);
}

// ContentParametersChanged
void
KShadowPartition::ContentParametersChanged(KPartition *partition,
										   const char *parameters)
{
	SetContentParameters(parameters);
}

// ChildAdded
void
KShadowPartition::ChildAdded(KPartition *partition, KPartition *child,
							 int32 index)
{
	// TODO: Mmh, in the CreateShadowPartition() phase, creating and
	// adding the shadow partitions is done recursively and we shouldn't
	// do that here. But when a new partition is added later?
	// Maybe KPhysicalPartition::CreateChild() should create a shadow for
	// the new child manually.
}

// ChildRemoved
void
KShadowPartition::ChildRemoved(KPartition *partition, KPartition *child,
							   int32 index)
{
	// TODO: We could remove the corresponding partition, but for consistency
	// we should proceed analogously to adding partitions.
}

// DiskSystemChanged
void
KShadowPartition::DiskSystemChanged(KPartition *partition,
									KDiskSystem *diskSystem)
{
	SetDiskSystem(diskSystem);
}

// CookieChanged
void
KShadowPartition::CookieChanged(KPartition *partition, void *cookie)
{
	// TODO: set the cookie?
}

// ContentCookieChanged
void
KShadowPartition::ContentCookieChanged(KPartition *partition, void *cookie)
{
	// TODO: set the content cookie?
}

