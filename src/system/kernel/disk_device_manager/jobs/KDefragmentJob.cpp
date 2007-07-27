// KDefragmentJob.cpp

#include "KDefragmentJob.h"


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


KDefragmentJob::KDefragmentJob(partition_id partition)
	: KDiskDeviceJob( B_DISK_DEVICE_JOB_DEFRAGMENT, partition, partition )	
{
	SetDescription( "defragmenting partition" );
}


KDefragmentJob::~KDefragmentJob() {}

status_t KDefragmentJob::Do(){
	
	KDiskDeviceManager * manager = KDiskDeviceManager::Default();
	
	KPartition * partition = manager->WriteLockPartition( PartitionID() );
	
	if( partition ) {
		PartitionRegistrar registrar(partition, true);
		PartitionRegistrar deviceRegistrar(partition->Device(), true);
				
		DeviceWriteLocker locker(partition->Device(), true);
		
		//some basic checks
		
		if (!partition->DiskSystem()) {
			SetErrorMessage("Partition has no disk system!");
			return B_BAD_VALUE;
		}
			
		
		// all descendants should be marked busy/descendant busy
		if ( isPartitionNotBusy( partition ) ) {
			SetErrorMessage("Can't defragment non-busy partition!");
			return B_ERROR;
		}
		
		bool whileMounted;
		//OK, seems alright, let's validate the job
		status_t validation_result = validate_defragment_partition(
				partition, partition->ChangeCounter(),
				&whileMounted, false);
		
		if( validation_result != B_OK ) {
			SetErrorMessage( "Validation of defragmenting partition failed.");
			return validation_result;
		}
		
		if( !whileMounted && partition->IsMounted() ) {
			SetErrorMessage( "This partition cannot be defragmented while mounted." );
			return B_ERROR;
		}
				
		//everything OK, let's do the job!
		
		KDiskSystem *diskSystem = partition->DiskSystem();
		DiskSystemLoader loader(diskSystem);
		
		locker.Unlock();
		
		status_t defrag_result = diskSystem->Defragment( partition, this );
		
		if( defrag_result != B_OK ) {
			SetErrorMessage( "Defragmenting partition failed!" );
			return defrag_result;
		}
		
		return B_OK;
		
		
	} else {
		SetErrorMessage( "Couldn't find partition!" );
		return B_ENTRY_NOT_FOUND;
	}
	
}
