// KInitializeJob.cpp

#include <stdio.h>

#include <DiskDeviceDefs.h>

#include "KUninitializeJob.h"

// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT printf

// constructor
KUninitializeJob::KUninitializeJob(partition_id partition)
	: KDiskDeviceJob(B_DISK_DEVICE_JOB_UNINITIALIZE, partition)
{
	SetDescription("uninitializing partition");
}

// destructor
KUninitializeJob::~KUninitializeJob()
{
}

// Do
status_t
KUninitializeJob::Do()
{
// TODO: implement
	DBG(OUT("KUninitializeJob::Do(%ld)\n", PartitionID()));
	return B_OK;
}

