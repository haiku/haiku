//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include "RPartition.h"
#include "RDiskDevice.h"
#include "RDiskDeviceList.h"
#include "RSession.h"

// constructor
RPartition::RPartition()
	: fSession(NULL),
	  fID(-1),
	  fChangeCounter(0)
{
}

// destructor
RPartition::~RPartition()
{
}

// SetTo
status_t
RPartition::SetTo(int fd, const extended_partition_info *partitionInfo)
{
	Unset();
	status_t error = B_OK;
	fID = _NextID();
	fChangeCounter = 0;
	fInfo = *partitionInfo;
	return error;
}

// Unset
void
RPartition::Unset()
{
	fID = -1;
}

// DeviceList
RDiskDeviceList *
RPartition::DeviceList() const
{
	return (fSession ? fSession->DeviceList() : NULL);
}

// Device
RDiskDevice *
RPartition::Device() const
{
	return (fSession ? fSession->Device() : NULL);
}

// print_partition_info
static
void
print_partition_info(const char *prefix, const extended_partition_info &info)
{
	printf("%soffset:         %lld\n", prefix, info.info.offset);
	printf("%ssize:           %lld\n", prefix, info.info.size);
	printf("%sblock size:     %ld\n", prefix, info.info.logical_block_size);
	printf("%ssession ID:     %ld\n", prefix, info.info.session);
	printf("%spartition ID:   %ld\n", prefix, info.info.partition);
	printf("%sdevice:         `%s'\n", prefix, info.info.device);
	printf("%sflags:          %lx\n", prefix, info.flags);
	printf("%spartition code: 0x%lx\n", prefix, info.partition_code);
	printf("%spartition name: `%s'\n", prefix, info.partition_name);
	printf("%spartition type: `%s'\n", prefix, info.partition_type);
	printf("%sFS short name:  `%s'\n", prefix, info.file_system_short_name);
	printf("%sFS long name:   `%s'\n", prefix, info.file_system_long_name);
	printf("%svolume name:    `%s'\n", prefix, info.volume_name);
	printf("%smounted at:     `%s'\n", prefix, info.mounted_at);
	printf("%sFS flags:       0x%lx\n", prefix, info.file_system_flags);
}

// Dump
void
RPartition::Dump() const
{
	printf("    partition %ld:\n", fInfo.info.partition);
	print_partition_info("      ", fInfo);
}

// _NextID
int32
RPartition::_NextID()
{
	return atomic_add(&fNextID, 1);
}

// fNextID
vint32 RPartition::fNextID = 0;

