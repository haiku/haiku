//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <new.h>

#include <Drivers.h>

#include "RSession.h"
#include "RDiskDevice.h"
#include "RDiskDeviceList.h"
#include "RPartition.h"

// constructor
RSession::RSession()
	: fPartitions(10, true),
	  fDevice(NULL),
	  fID(-1),
	  fChangeCounter(0)
{
}

// destructor
RSession::~RSession()
{
}

// SetTo
status_t
RSession::SetTo(int fd, const session_info *sessionInfo)
{
	Unset();
	status_t error = B_OK;
	fID = _NextID();
	fChangeCounter = 0;
	fInfo = *sessionInfo;
	// iterate through the partitions
	for (int32 i = 0; ; i++) {
		// get the partition info
		extended_partition_info partitionInfo;
		status_t status = get_nth_partition_info(fd, sessionInfo->index, i,
			&partitionInfo, (i == 0 ? fPartitioningSystem : NULL));
		if (status != B_OK) {
			if (status != B_ENTRY_NOT_FOUND)
				error = status;
			break;
		}
		// create and add a RPartition
		if (RPartition *partition = new(nothrow) RPartition) {
			error = partition->SetTo(fd, &partitionInfo);
			if (error == B_OK)
				AddPartition(partition);
			else
				delete partition;
		} else
			error = B_NO_MEMORY;
	}
	// cleanup on error
	if (error != B_OK)
		Unset();
	return error;
}

// Unset
void
RSession::Unset()
{
	for (int32 i = CountPartitions() - 1; i >= 0; i--)
		RemovePartition(i);
	fID = -1;
}

// DeviceList
RDiskDeviceList *
RSession::DeviceList() const
{
	return (fDevice ? fDevice->DeviceList() : NULL);
}

// Index
int32
RSession::Index() const
{
	if (fDevice)
		return fDevice->IndexOfSession(this);
	return -1;
}

// AddPartition
bool
RSession::AddPartition(RPartition *partition)
{
	bool success = false;
	if (partition) {
		success = fPartitions.AddItem(partition);
		if (success) {
			partition->SetSession(this);
			if (RDiskDeviceList *deviceList = DeviceList())
				deviceList->PartitionAdded(partition);
		}
	}
	return success;
}

// RemovePartition
bool
RSession::RemovePartition(int32 index)
{
	RPartition *partition = PartitionAt(index);
	if (partition) {
		partition->SetSession(NULL);
		fPartitions.RemoveItemAt(index);
		if (RDiskDeviceList *deviceList = DeviceList())
			deviceList->PartitionRemoved(partition);
		delete partition;
	}
	return (partition != NULL);
}

// RemovePartition
bool
RSession::RemovePartition(RPartition *partition)
{
	bool success = false;
	if (partition) {
		int32 index = fPartitions.IndexOf(partition);
		if (index >= 0)
			success = RemovePartition(index);
	}
	return success;
}

// print_session_info
static
void
print_session_info(const char *prefix, const session_info &info)
{
	printf("%soffset:        %lld\n", prefix, info.offset);
	printf("%ssize:          %lld\n", prefix, info.size);
	printf("%sblock size:    %ld\n", prefix, info.logical_block_size);
	printf("%sindex:         %ld\n", prefix, info.index);
	printf("%sflags:         %lx\n", prefix, info.flags);
}

// Archive
status_t
RSession::Archive(BMessage *archive) const
{
	status_t error = (archive ? B_OK : B_BAD_VALUE);
	// ID, change counter and index
	if (error == B_OK)
		error = archive->AddInt32("id", fID);
	if (error == B_OK)
		error = archive->AddInt32("change_counter", fChangeCounter);
	if (error == B_OK)
		error = archive->AddInt32("index", Index());
	// fInfo.*
	if (error == B_OK)
		error = archive->AddInt64("offset", fInfo.offset);
	if (error == B_OK)
		error = archive->AddInt64("size", fInfo.size);
	if (error == B_OK)
		error = archive->AddInt32("flags", (int32)fInfo.flags);
	// other data
	if (error == B_OK)
		error = archive->AddString("partitioning", fPartitioningSystem);
	// partitions
	for (int32 i = 0; const RPartition *partition = PartitionAt(i); i++) {
		BMessage partitionArchive;
		error = partition->Archive(&partitionArchive);
		if (error == B_OK)
			error = archive->AddMessage("partitions", &partitionArchive);
		if (error != B_OK)
			break;
	}
	return error;
}

// Dump
void
RSession::Dump() const
{
	printf("  session %ld:\n", fInfo.index);
	print_session_info("    ", fInfo);
	printf("    partitioning : `%s'\n", fPartitioningSystem);
	for (int32 i = 0; RPartition *partition = PartitionAt(i); i++)
		partition->Dump();
}

// _NextID
int32
RSession::_NextID()
{
	return atomic_add(&fNextID, 1);
}

// fNextID
vint32 RSession::fNextID = 0;

