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
	  fChangeCounter(),
	  fVolume(NULL)
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
	fChangeCounter.Reset();
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

// Changed
void
RPartition::Changed()
{
	if (fChangeCounter.Increment() && fSession)
		fSession->Changed();
}

// Index
int32
RPartition::Index() const
{
	if (fSession)
		return fSession->IndexOfPartition(this);
	return -1;
}

// GetPath
void
RPartition::GetPath(char *path) const
{
	RDiskDevice *device = Device();
	if (path && device) {
		strcpy(path, device->Path());
		if (char *lastSlash = strrchr(path, '/'))
			sprintf(lastSlash + 1, "%ld_%ld", Session()->Index(), Index());
	}
}

// Update
status_t
RPartition::Update(const extended_partition_info *partitionInfo)
{
	status_t error = B_OK;
	// TODO: Check the partition info for changes!
	fInfo = *partitionInfo;
	return error;
}

// Archive
status_t
RPartition::Archive(BMessage *archive) const
{
	status_t error = (archive ? B_OK : B_BAD_VALUE);
	// ID, change counter and index
	if (error == B_OK)
		error = archive->AddInt32("id", fID);
	if (error == B_OK)
		error = archive->AddInt32("change_counter", ChangeCounter());
	if (error == B_OK)
		error = archive->AddInt32("index", Index());
	// fInfo.info.*
	if (error == B_OK)
		error = archive->AddInt64("offset", fInfo.info.offset);
	if (error == B_OK)
		error = archive->AddInt64("size", fInfo.info.size);
	// fInfo.*
	if (error == B_OK)
		error = archive->AddInt32("flags", (int32)fInfo.flags);
	if (error == B_OK)
		error = archive->AddString("name", fInfo.partition_name);
	if (error == B_OK)
		error = archive->AddString("type", fInfo.partition_type);
	if (error == B_OK) {
		error = archive->AddString("fs_short_name",
								   fInfo.file_system_short_name);
	}
	if (error == B_OK) {
		error = archive->AddString("fs_long_name",
								   fInfo.file_system_long_name);
	}
	if (error == B_OK)
		error = archive->AddString("volume_name", fInfo.volume_name);
	if (error == B_OK)
		error = archive->AddInt32("fs_flags", (int32)fInfo.file_system_flags);
	// dev_t, if mounted
	if (error == B_OK && fVolume)
		error = archive->AddInt32("volume_id", fVolume->ID());
	return error;
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
//	printf("%smounted at:     `%s'\n", prefix, info.mounted_at);
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

