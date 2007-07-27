// KInitializeJob.cpp

#include <stdio.h>

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
#include "KInitializeJob.h"

// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT dprintf


KInitializeJob::KInitializeJob(partition_id partition, disk_system_id diskSystemID,
				const char *name, const char *parameters)
	: KDiskDeviceJob( B_DISK_DEVICE_JOB_INITIALIZE, partition, partition ), 
	  fDiskSystemID( diskSystemID ),
	  fName( !name ? NULL : strcpy( new char[strlen(name)+1], name ) ),
	  fParameters( !parameters ? NULL : strcpy( new char[strlen(parameters)+1], parameters ) )
{
	SetDescription( "initializing the partition with given disk system" );
}

KInitializeJob::~KInitializeJob() {
	delete[] fParameters;
}

status_t KInitializeJob::Do() {
DBG(OUT("KInitializeJob::Do(%ld)\n", PartitionID()));
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KPartition *partition = manager->WriteLockPartition(PartitionID());
	if (partition) {
		PartitionRegistrar registrar(partition, true);
		PartitionRegistrar deviceRegistrar(partition->Device(), true);
		
		DeviceWriteLocker locker(partition->Device(), true);
		// basic checks
		
// 		if (!partition->ParentDiskSystem()) {
// 			SetErrorMessage("Partition has no parent disk system!");
// 			return B_BAD_VALUE;
// 		}
		
		// all descendants should be marked busy/descendant busy
		if ( isPartitionNotBusy( partition ) ) {
			SetErrorMessage("Can't initialize non-busy partition!");
			return B_ERROR;
		}
		
/*		if( !fParameters ) {
			//no parameters for the operation
			SetErrorMessage( "No parameters for partition initialization." );
			return B_ERROR;
		}*/
		
		
		//TODO shouldn't we load the disk system AFTER the validation?
		KDiskSystem *diskSystemToInit = manager->LoadDiskSystem( fDiskSystemID );
		if( ! diskSystemToInit ) {
			SetErrorMessage( "Given DiskSystemID doesn't correspond to any known DiskSystem.");
			return B_BAD_VALUE;
		}
		DiskSystemLoader loader2(diskSystemToInit);
		
		status_t validation_result = validate_initialize_partition(
				partition, partition->ChangeCounter(),
				diskSystemToInit->Name(), fName,
				fParameters, false );
		
		if( validation_result != B_OK ) {
			SetErrorMessage( "Validation of initializing partition failed!" );
			return validation_result;
		}
		
		//everything seems OK -> let's do the job
		locker.Unlock();
		
		status_t init_result = diskSystemToInit->Initialize( partition, fName, fParameters, this );
		
		if( init_result != B_OK ) {
			SetErrorMessage( "Initialization of partition failed!" );
			return init_result;
		}
		
		partition->SetDiskSystem(diskSystemToInit);
		
		return B_OK;
		
		
	} else {
		SetErrorMessage( "Couldn't find partition." );
		return B_ENTRY_NOT_FOUND;
	}
	
}
