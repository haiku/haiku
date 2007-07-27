// KRepairJob.cpp

#include "KRepairJob.h"


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






KRepairJob::KRepairJob(partition_id partition, bool checkOnly)
	: KDiskDeviceJob( B_DISK_DEVICE_JOB_REPAIR, partition, partition ),
	fCheckOnly( checkOnly )
{
	SetDescription( "repairing partition" );
}
KRepairJob::~KRepairJob() {}

status_t KRepairJob::Do(){
	
	KDiskDeviceManager * manager = KDiskDeviceManager::Default();
	
	KPartition * partition = manager->WriteLockPartition( PartitionID() );
	
	if( partition ) {
		PartitionRegistrar registrar(partition, true);
		PartitionRegistrar deviceRegistrar(partition->Device(), true);
		
		DeviceWriteLocker locker(partition->Device(), true);
		
		//basic checks
		if( !partition->DiskSystem() ) {
			SetErrorMessage( "Partition has no disk system." );
			return B_BAD_VALUE;
		}
		
		if ( isPartitionNotBusy( partition ) ) {
			SetErrorMessage("Can't repair non-busy partition!");
			return B_ERROR;
		}
		
		bool whileMounted;
		
		status_t validation_result = validate_repair_partition(
				partition, partition->ChangeCounter(),
				fCheckOnly, &whileMounted,
				false);
		
		if( validation_result != B_OK ) {
			SetErrorMessage( "Validation repairing partition failed!" );
			return validation_result;
		}
		
		//everything OK, let's do the job
		KDiskSystem * diskSystem = partition->DiskSystem();
		DiskSystemLoader loader(diskSystem); 
		
		locker.Unlock();
				
		status_t repair_result = diskSystem->Repair( partition, fCheckOnly, this );
		
		if( repair_result != B_OK ) {
			if( fCheckOnly ) {
				SetErrorMessage( "Checking for repairing partition failed!" );
			} else {
				SetErrorMessage( "Repairing partition failed!" );				
			}
			return repair_result;
		}
		
		return B_OK;
		
	}else {
		SetErrorMessage( "Couldn't find partition!" );
		return B_ENTRY_NOT_FOUND;
	}
	
	
	
	
	
	
}
