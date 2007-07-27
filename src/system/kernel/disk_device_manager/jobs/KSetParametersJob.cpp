// KSetParametersJob.cpp



#include <KernelExport.h>
#include <DiskDeviceDefs.h>
#include <KDiskDevice.h>
#include <KDiskDeviceManager.h>
#include <KDiskDeviceUtils.h>
#include <KDiskSystem.h>
#include <KPartition.h>
#include <KPartitionVisitor.h>

#include <string.h>


#include "KSetParametersJob.h"
#include "ddm_operation_validation.h"



KSetParametersJob::KSetParametersJob(partition_id parent, partition_id partition,
				  const char *parameters, const char *contentParameters)
	: KDiskDeviceJob(B_DISK_DEVICE_JOB_SET_TYPE, partition, parent), 
	fParams( !parameters ? NULL : strcpy( new char[strlen(parameters)+1], parameters )),
	fContentParams( 
			!contentParameters ? NULL : 
			strcpy( new char[strlen(contentParameters)+1], contentParameters))
{
	SetDescription( "setting partition parameters&content parameters" );
}
				  
				  
KSetParametersJob::~KSetParametersJob(){}

status_t KSetParametersJob::Do(){
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KPartition *partition = manager->WriteLockPartition(PartitionID());
	if (partition) {
		if( !fParams && !fContentParams ) {	
			SetErrorMessage( "No parameter to set." );		
			return B_BAD_VALUE;
		}
		PartitionRegistrar registrar(partition, true);
		PartitionRegistrar deviceRegistrar(partition->Device(), true);
		
		//TODO is lock necessary?
		DeviceWriteLocker locker(partition->Device(), true);
		
		
		
		
		// all descendants should be marked busy/descendant busy
		if ( isPartitionNotBusy( partition ) ) {
			SetErrorMessage("Can't set parameters for non-busy partition!");
			return B_ERROR;
		}
		
		//TODO unlock?
		
		if( fParams ) {
			// basic checks
			
			if (!partition->ParentDiskSystem()) {
				SetErrorMessage("Partition has no parent disk system!");
				return B_BAD_VALUE;
			}
					
			//TODO mayebe give copy of parameters?
			//we have some parameters to set
			status_t validation_result = validate_set_partition_parameters( partition, partition->ChangeCounter(), fParams, false );
			
			KDiskSystem *parentDiskSystem = partition->ParentDiskSystem();
			DiskSystemLoader loader(parentDiskSystem);
			
			if( validation_result != B_OK ) {
				SetErrorMessage( "Validation of setting partition parameters failed." );
				return validation_result;
			}
			
			locker.Unlock();
			status_t set_pars_result = parentDiskSystem->SetParameters( partition, fParams, this );
			
			if( set_pars_result != B_OK ) {
				SetErrorMessage( "Setting partition parameters failed." );
			}			
		}
		
		
		if( fContentParams ) {
			// basic checks
			
			if (!partition->DiskSystem()) {
				SetErrorMessage("Partition has no disk system!");
				return B_BAD_VALUE;
			}
		
			
			
			status_t validation_result = validate_set_partition_content_parameters (
					partition, partition->ChangeCounter(), fContentParams, false );
			
			if( validation_result != B_OK ) {
				SetErrorMessage( "Validation of setting partition content parameters failed." );
				return validation_result;
			}
			
			KDiskSystem *diskSystem = partition->DiskSystem();
			DiskSystemLoader loader(diskSystem);
			
			locker.Unlock();
			status_t set_cont_pars_result = diskSystem->SetContentParameters( 
					partition, fContentParams, this );
			
			if( set_cont_pars_result != B_OK ) {
				SetErrorMessage( "Setting partition content parameters failed." );
			}
		}
		
			
		return B_OK;
		
		
	} else {
		SetErrorMessage( "Couldn't find partition" );
		return B_ENTRY_NOT_FOUND;
	}
	
}
