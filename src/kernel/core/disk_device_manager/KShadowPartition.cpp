// KShadowPartition.cpp

#include <errno.h>
#include <fcntl.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Drivers.h>
#include <Errors.h>

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
	  fPhysicalPartition(NULL)
{
	SetPhysicalPartition(partition);
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
KShadowPartition::ShadowPartition()
{
	return this;
}

// SetPhysicalPartition
void
KShadowPartition::SetPhysicalPartition(KPhysicalPartition *partition)
{
	fPhysicalPartition = partition;
// TODO: clone the data of the physical partition.
}

// PhysicalPartition
KPhysicalPartition*
KShadowPartition::PhysicalPartition()
{
	return fPhysicalPartition;
}

// Dump
void
KShadowPartition::Dump(bool deep, int32 level)
{
	KPartition::Dump(deep, level);
}

