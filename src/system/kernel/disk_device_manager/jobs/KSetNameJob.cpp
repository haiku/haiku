
#include "KSetNameJob.h"

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


KSetNameJob::KSetNameJob(partition_id parentID, partition_id partitionID, const char * name, 
						 const char * contentName)
	: KDiskDeviceJob(B_DISK_DEVICE_JOB_SET_NAME, partitionID, parentID),
	fName( !name ? NULL : strcpy( new char[strlen(name)+1], name ) ),
	fContentName( !contentName ? NULL : strcpy( new char[strlen(contentName)+1], contentName ) )
{
	SetDescription( "setting name for the partition or its content" );
}


KSetNameJob::~KSetNameJob()
{
	delete[] fName;
	delete[] fContentName;
}
 

status_t KSetNameJob::Do() {
	KDiskDeviceManager * manager = KDiskDeviceManager::Default();
	
	KPartition * partition = manager->WriteLockPartition( PartitionID() );
	if( partition ) {
		if( !fName && !fContentName ) {
			SetErrorMessage( "No name to set." );
			return B_BAD_VALUE;
		}
		
		PartitionRegistrar registrar(partition, true);
		PartitionRegistrar deviceRegistrar(partition->Device(), true);
		
		//TODO is lock necessary?
		DeviceWriteLocker locker(partition->Device(), true);
		
		//basic checks
		
		// all descendants should be marked busy/descendant busy
		if ( isPartitionNotBusy( partition ) ) {
			SetErrorMessage("Can't set name for non-busy partition!");
			return B_ERROR;
		}
		
		if( fName ) {
			//set name of the partition
			
			//setting name of this partition -> via our parent disk system
			if( !partition->ParentDiskSystem() ) {
				SetErrorMessage( "Partition has no parent disk system!" );
				return B_BAD_VALUE;
			}
			
			//TODO mayebe give copy of name
			status_t validation_result = validate_set_partition_name(
					partition, partition->ChangeCounter(),
					fName, false);
			
			if( validation_result != B_OK) {
				SetErrorMessage( "Validation of setting partition name failed!" );
				return validation_result;				
			}
			
			//everything OK, let's do the job
			KDiskSystem * parentDiskSystem = partition->ParentDiskSystem();
			
			DiskSystemLoader loader(parentDiskSystem);
			
			locker.Unlock();
			
			status_t set_name_result = parentDiskSystem->SetName(partition, fName, this);
			
			if( set_name_result != B_OK ) {
				SetErrorMessage( "Setting name of the partition failed!" );						
			}
			return set_name_result;
			
		}
		
		if( fContentName ) {
			//set name of the contents
			
			//setting name of our contents -> via our disk system
			if( !partition->DiskSystem() ) {
				SetErrorMessage( "Partition has no disk system!");
				return B_BAD_VALUE;
			}
			
			status_t validation_result = validate_set_partition_content_name(
					partition, partition->ChangeCounter(), 
					fContentName, false);
			
			if( validation_result != B_OK ) {
				SetErrorMessage( "Validation of setting partition content name failed!" );
				return validation_result;
			}
			
			//everything OK, let's do the job
			
			KDiskSystem * diskSystem = partition->DiskSystem();
			DiskSystemLoader loader( diskSystem );
			
			locker.Unlock();
			
			status_t set_cont_name_result = diskSystem->SetContentName(
					partition, fContentName, this);
			
			if( set_cont_name_result != B_OK ) {
				SetErrorMessage( "Setting name of the partition's content failed!" );
			}
			return set_cont_name_result;
			
		}
		
		
	} else {
		SetErrorMessage( "Couldn't find partition!" );
		return B_ENTRY_NOT_FOUND;
	}
			
			
}
