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
	  fChangeCounter()
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
	fChangeCounter.Reset();
	fInfo = *sessionInfo;
	// scan for partitions
	error = _RescanPartitions(fd, B_DEVICE_CAUSE_UNKNOWN);
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
		RemovePartition(i, B_DEVICE_CAUSE_UNKNOWN);
	fID = -1;
}

// DeviceList
RDiskDeviceList *
RSession::DeviceList() const
{
	return (fDevice ? fDevice->DeviceList() : NULL);
}

// PartitionLayoutChanged
status_t
RSession::PartitionLayoutChanged()
{
PRINT(("RSession::PartitionLayoutChanged()\n"));
	Changed();
	status_t error = (fDevice ? B_OK : B_ERROR);
	if (error == B_OK)
		error = _RescanPartitions(fDevice->FD(), B_DEVICE_CAUSE_UNKNOWN);
	return error;
}

// Changed
void
RSession::Changed()
{
	if (fChangeCounter.Increment() && fDevice)
		fDevice->Changed();
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
status_t
RSession::AddPartition(int fd, const extended_partition_info *partitionInfo,
					   uint32 cause)
{
	status_t error = B_OK;
	if (RPartition *partition = new(nothrow) RPartition) {
		error = partition->SetTo(fd, partitionInfo);
		if (error == B_OK)
			AddPartition(partition, cause);
		else
			delete partition;
	} else
		error = B_NO_MEMORY;
	return error;
}

// AddPartition
bool
RSession::AddPartition(RPartition *partition, uint32 cause)
{
	bool success = false;
	if (partition) {
		success = fPartitions.AddItem(partition);
		if (success) {
			partition->SetSession(this);
			Changed();
			// trigger notifications
			if (RDiskDeviceList *deviceList = DeviceList())
				deviceList->PartitionAdded(partition, cause);
		}
	}
	return success;
}

// RemovePartition
bool
RSession::RemovePartition(int32 index, uint32 cause)
{
	RPartition *partition = PartitionAt(index);
	if (partition) {
		Changed();
		if (RDiskDeviceList *deviceList = DeviceList())
			deviceList->PartitionRemoved(partition, cause);
		partition->SetSession(NULL);
		fPartitions.RemoveItemAt(index);
		delete partition;
	}
	return (partition != NULL);
}

// RemovePartition
bool
RSession::RemovePartition(RPartition *partition, uint32 cause)
{
	bool success = false;
	if (partition) {
		int32 index = fPartitions.IndexOf(partition);
		if (index >= 0)
			success = RemovePartition(index, cause);
	}
	return success;
}

// Update
status_t
RSession::Update(const session_info *sessionInfo)
{
	RChangeCounter::Locker lock(fChangeCounter);
	status_t error = (fDevice ? B_OK : B_ERROR);
	// check the session info for changes
	// Currently there is very little that can have changed. offset and size
	// must not be changed, logical_block_size is ignored anyway and index
	// doesn't change. So only flags remains, but that indicates only, if the
	// session is virtual or a data/audio partition. So we don't do anything
	// for now.
	fInfo = *sessionInfo;
	int fd = (fDevice ? fDevice->FD() : -1);
	// check the partitions
	for (int32 i = 0; error == B_OK; i++) {
		// get the partition info
		extended_partition_info partitionInfo;
		char partitioningSystem[B_FILE_NAME_LENGTH];
		status_t status = get_nth_partition_info(fd, sessionInfo->index, i,
			&partitionInfo, (i == 0 ? partitioningSystem : NULL));
		// check the partitioning system
		if ((status == B_OK || status == B_ENTRY_NOT_FOUND) && i == 0
			&& strcmp(partitioningSystem, fPartitioningSystem)) {
			// partitioning system has changed
			error = PartitionLayoutChanged();
			break;
		}
		if (status != B_OK) {
			if (status == B_ENTRY_NOT_FOUND) {
				// remove disappeared partitions
				for (int32 k = CountPartitions() - 1; k >= i; k--)
					RemovePartition(k, B_DEVICE_CAUSE_UNKNOWN);
			} else {
				// ignore errors -- we can't help it
//				error = status;
			}
			break;
		}
		// check the partition
		if (RPartition *partition = PartitionAt(i)) {
			if (partition->Offset() == partitionInfo.info.offset
				&& partition->Size() == partitionInfo.info.size) {
				partition->Update(&partitionInfo);
			} else {
				// partition layout changed
				error = PartitionLayoutChanged();
				break;
			}
		} else {
			// partition added
			error = AddPartition(fd, &partitionInfo, B_DEVICE_CAUSE_UNKNOWN);
		}
	}
	return error;
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
		error = archive->AddInt32("change_counter", ChangeCounter());
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

// _RescanPartitions
status_t
RSession::_RescanPartitions(int fd, uint32 cause)
{
	status_t error = B_OK;
	// remove the current partitions
	for (int32 i = CountPartitions() - 1; i >= 0; i--)
		RemovePartition(i, cause);
	// scan for partitions
	for (int32 i = 0; error == B_OK; i++) {
		// get the partition info
		extended_partition_info partitionInfo;
		status_t status = get_nth_partition_info(fd, fInfo.index, i,
			&partitionInfo, (i == 0 ? fPartitioningSystem : NULL));
		if (status != B_OK) {
			if (status != B_ENTRY_NOT_FOUND) {
				// ignore errors -- we can't help it
//				error = status;
			}
			break;
		}
		// create and add a RPartition
		error = AddPartition(fd, &partitionInfo, cause);
	}
	return error;
}

// _NextID
int32
RSession::_NextID()
{
	return atomic_add(&fNextID, 1);
}

// fNextID
vint32 RSession::fNextID = 0;

