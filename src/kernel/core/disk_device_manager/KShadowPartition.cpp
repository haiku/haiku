// KShadowPartition.cpp

#include <errno.h>
#include <fcntl.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Drivers.h>
#include <Errors.h>

#include "ddm_userland_interface.h"
#include "KDiskDevice.h"
#include "KDiskDeviceManager.h"
#include "KDiskDeviceUtils.h"
#include "KShadowPartition.h"

using namespace std;

// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT printf

// constructor
KShadowPartition::KShadowPartition(KPhysicalPartition *partition)
	: KPartition(),
	  fPhysicalPartition(partition)
{
	SyncWithPhysicalPartition();
}

// destructor
KShadowPartition::~KShadowPartition()
{
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
	fPhysicalPartition = NULL;
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
	// Cookie, ContentCookie, MountCookie?
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

