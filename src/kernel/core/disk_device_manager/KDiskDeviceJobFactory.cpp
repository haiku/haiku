// KDiskDeviceJob.cpp

#include "KDiskDeviceJob.h"
#include "KDiskDeviceJobFactory.h"
#include "KScanPartitionJob.h"

// constructor
KDiskDeviceJobFactory::KDiskDeviceJobFactory()
{
}

// destructor
KDiskDeviceJobFactory::~KDiskDeviceJobFactory()
{
}

// CreateDefragmentJob
KDiskDeviceJob *
KDiskDeviceJobFactory::CreateDefragmentJob(partition_id partitionID)
{
	// not implemented
	return NULL;
}

// CreateRepairJob
KDiskDeviceJob *
KDiskDeviceJobFactory::CreateRepairJob(partition_id partitionID,
									   bool checkOnly)
{
	// not implemented
	return NULL;
}

// CreateResizeJob
KDiskDeviceJob *
KDiskDeviceJobFactory::CreateResizeJob(partition_id parentID,
									   partition_id partitionID, off_t size,
									   bool resizeContents)
{
	// not implemented
	return NULL;
}

// CreateMoveJob
KDiskDeviceJob *
KDiskDeviceJobFactory::CreateMoveJob(partition_id parentID,
									 partition_id partitionID, off_t offset,
									 Vector<partition_id> *contentsToMove)
{
	// not implemented
	return NULL;
}

// CreateSetNameJob
KDiskDeviceJob *
KDiskDeviceJobFactory::CreateSetNameJob(partition_id parentID,
										partition_id partitionID,
										const char *name)
{
	// not implemented
	return NULL;
}

// CreateSetContentNameJob
KDiskDeviceJob *
KDiskDeviceJobFactory::CreateSetContentNameJob(partition_id partitionID,
											   const char *name)
{
	// not implemented
	return NULL;
}

// CreateSetTypeJob
KDiskDeviceJob *
KDiskDeviceJobFactory::CreateSetTypeJob(partition_id parentID,
										partition_id partitionID,
										const char *type)
{
	// not implemented
	return NULL;
}

// CreateSetParametersJob
KDiskDeviceJob *
KDiskDeviceJobFactory::CreateSetParametersJob(partition_id parentID,
											  partition_id partitionID,
											  const char *parameters)
{
	// not implemented
	return NULL;
}

// CreateSetContentParametersJob
KDiskDeviceJob *
KDiskDeviceJobFactory::CreateSetContentParametersJob(partition_id partitionID,
													 const char *parameters)
{
	// not implemented
	return NULL;
}

// CreateInitializeJob
KDiskDeviceJob *
KDiskDeviceJobFactory::CreateInitializeJob(partition_id partitionID,
										   disk_system_id diskSystemID,
										   const char *name,
										   const char *parameters)
{
	// not implemented
	return NULL;
}

// CreateCreateChildJob
KDiskDeviceJob *
KDiskDeviceJobFactory::CreateCreateChildJob(partition_id partitionID,
											partition_id child, off_t offset,
											off_t size, const char *type,
											const char *parameters)
{
	// not implemented
	return NULL;
}

// CreateDeleteChildJob
KDiskDeviceJob *
KDiskDeviceJobFactory::CreateDeleteChildJob(partition_id parentID,
											partition_id partitionID)
{
	// not implemented
	return NULL;
}

// CreateScanPartitionJob
KDiskDeviceJob *
KDiskDeviceJobFactory::CreateScanPartitionJob(partition_id partitionID)
{
	// not implemented
	return new(nothrow) KScanPartitionJob(partitionID);
}

