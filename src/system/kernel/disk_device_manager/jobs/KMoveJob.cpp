// KMoveJob.cpp
#include "KMoveJob.h"

#include <stdio.h>

#include <KernelExport.h>
#include <DiskDeviceDefs.h>
#include <KDiskDevice.h>
#include <KDiskDeviceManager.h>
#include <KDiskDeviceUtils.h>
#include <KDiskSystem.h>
#include <KPartition.h>
#include <KPartitionVisitor.h>

#include "ddm_operation_validation.h"



// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT dprintf

KMoveJob::KMoveJob(partition_id parentID, partition_id partitionID, off_t offset)
	: KDiskDeviceJob(B_DISK_DEVICE_JOB_MOVE, partitionID, parentID ),
	fNewOffset(offset)
{
	SetDescription( "moving partition" );	
}

KMoveJob::~KMoveJob() {}


status_t KMoveJob::Do() {
	
	DBG(OUT( "KMoveJob::Do(%ld)\n", PartitionID() ));
	
	//get the partition
	KDiskDeviceManager * manager = KDiskDeviceManager::Default();
	KPartition *partition = manager->WriteLockPartition(PartitionID());
	if (partition) {
		//OK, we have the partition, do some checks
		PartitionRegistrar registrar(partition, true);
		PartitionRegistrar deviceRegistrar(partition->Device(), true);
		
		DeviceWriteLocker locker(partition->Device(), true);
		
		// basic checks
		if (!partition->ParentDiskSystem()) {
			SetErrorMessage("Partition has no parent disk system!");
			return B_BAD_VALUE;
		}
		
		if(partition->Offset() == fNewOffset ) {
			//we are already on the right place -> nothing to do...
			return B_OK;
		}
		
		off_t new_offset = fNewOffset;
		
		status_t validate_result = validate_move_partition(partition, partition->ChangeCounter(),
				&new_offset, true, false ); //TODO posledni 2 parametry????
		if( validate_result != B_OK ) {
			SetErrorMessage( "Validation of the new partition offset failed." );
			return validate_result;
		}
		
		if( new_offset != fNewOffset ) {
			SetErrorMessage( "Requested partition offset not valid." );
			return B_ERROR;
		}
		
		// all descendants should be marked busy/descendant busy
		if ( isPartitionNotBusy( partition ) ) {
			SetErrorMessage("Can't move non-busy partition!");
			return B_ERROR;
		}
		
		//get all necessary objects
		KDiskSystem *parentDiskSystem = partition->ParentDiskSystem();
		KDiskSystem *childDiskSystem = partition->DiskSystem();
				
		
		locker.Unlock();
		
		status_t move_result = parentDiskSystem->Move( partition, fNewOffset, this );
		if( move_result != B_OK ) {
			SetErrorMessage( "Moving of partition failed." );
		}
		return move_result;
	//do the move
		
	} else {
		SetErrorMessage("Couldn't find partition.");
		return B_ENTRY_NOT_FOUND;
	}
	return B_ERROR;
}
