// KResizeJob.cpp

#include <stdio.h>

#include <DiskDeviceDefs.h>

#include "KResizeJob.h"

// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT printf

// constructor
KResizeJob::KResizeJob(partition_id parentID, partition_id partitionID,
					   off_t size, bool resizeContents)
	: KDiskDeviceJob(B_DISK_DEVICE_JOB_RESIZE, partitionID, parentID),
	  fResizeContents(resizeContents)
{
	SetDescription("uninitializing partition");
}

// destructor
KResizeJob::~KResizeJob()
{
}

// Do
status_t
KResizeJob::Do()
{
// TODO: implement
	DBG(OUT("KResizeJob::Do(%ld)\n", PartitionID()));
	return B_OK;
}

