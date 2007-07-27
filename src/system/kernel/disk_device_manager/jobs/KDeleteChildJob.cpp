// KDeleteChildJob.cpp

#include "KDeleteChildJob.h"



#include <KernelExport.h>
#include <DiskDeviceDefs.h>
#include <KDiskDevice.h>
#include <KDiskDeviceManager.h>
#include <KDiskDeviceUtils.h>
#include <KDiskSystem.h>
#include <KPartition.h>
#include <KPartitionVisitor.h>
#include <string.h>

#include "ddm_operation_validation.h"



KDeleteChildJob::KDeleteChildJob(partition_id parent, partition_id partition)
	: KDiskDeviceJob( B_DISK_DEVICE_JOB_DELETE, partition, parent )
{
	SetDescription( "deleting child of the partition" );
}
		
KDeleteChildJob::~KDeleteChildJob() {}

status_t KDeleteChildJob::Do(){
	KDiskDeviceManager * manager = KDiskDeviceManager::Default();
	
	KPartition * partition = manager->WriteLockPartition( PartitionID() );
	
	if( partition ) {
		PartitionRegistrar registrar(partition, true);
		PartitionRegistrar deviceRegistrar(partition->Device(), true);
				
		DeviceWriteLocker locker(partition->Device(), true);
		
		
		if (!partition->ParentDiskSystem()) {
			SetErrorMessage("Partition has no parent disk system!");
			return B_BAD_VALUE;
		}
				
		// all descendants should be marked busy/descendant busy
		if ( isPartitionNotBusy( partition ) ) {
			SetErrorMessage("Can't delete child of non-busy partition!");
			return B_ERROR;
		}
		
		status_t validation_result = validate_delete_child_partition(
				partition, partition->ChangeCounter(),
				false);
		
		if( validation_result != B_OK ) {
			SetErrorMessage( "Validation of deleting child failed!" );
			return validation_result;
		}
		
		//everything OK, let's do the job!
		KDiskSystem *parentDiskSystem = partition->ParentDiskSystem();
		DiskSystemLoader loader(parentDiskSystem );
		
		locker.Unlock();
		
		status_t delete_result = parentDiskSystem->DeleteChild( partition, this );
		
		if( delete_result != B_OK ) {
			SetErrorMessage( "Deleting child failed!" );
			return delete_result;
		}
		
		return B_OK;
		
	} else {
		SetErrorMessage( "Couldn't find partition!" );
		return B_ENTRY_NOT_FOUND;
	}
}
