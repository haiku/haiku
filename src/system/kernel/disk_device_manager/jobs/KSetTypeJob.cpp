//
// C++ Implementation: KSetTypeJob
//
// Description: 
//
//
// Author:  <lubos@radical.ed>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//


#include <KPartition.h>
#include <KernelExport.h>
#include <DiskDeviceDefs.h>
#include <KDiskDevice.h>
#include <KDiskDeviceManager.h>
#include <KDiskDeviceUtils.h>
#include <KDiskSystem.h>

#include <KPartitionVisitor.h>

#include <string.h>

#include "KSetTypeJob.h"
#include "ddm_operation_validation.h"

KSetTypeJob::KSetTypeJob(partition_id parentID, partition_id partitionID, const char *type)
	: KDiskDeviceJob(B_DISK_DEVICE_JOB_SET_TYPE, partitionID, parentID),
	fType( !type ? NULL : strcpy( new char[strlen(type)+1], type ) )
{
	SetDescription( "setting partition type" );
}


KSetTypeJob::~KSetTypeJob(){}

status_t KSetTypeJob::Do() {
	KDiskDeviceManager * manager = KDiskDeviceManager::Default();
	
	KPartition * partition = manager->WriteLockPartition( PartitionID() );
	
	if( partition ) {
		PartitionRegistrar registrar(partition, true);
		PartitionRegistrar deviceRegistrar(partition->Device(), true);
		
		//TODO is lock necessary?
		DeviceWriteLocker locker(partition->Device(), true);
		
		//basic checks
		if( !partition->ParentDiskSystem() ) {
			SetErrorMessage( "Partition has no parent disk system!" );
			return B_BAD_VALUE;
		}
		
		if( !fType ) {
			SetErrorMessage( "No type to set!" );
			return B_BAD_VALUE;
		}
		
		status_t validation_result = validate_set_partition_type(
				partition, partition->ChangeCounter(),
				fType, false);
		
		if( validation_result != B_OK ) {
			SetErrorMessage( "Validation of setting partition type failed!" );
			return validation_result;
		}
		
		//everything OK, let's do the job
		KDiskSystem * parentDiskSystem = partition->ParentDiskSystem();
		DiskSystemLoader loader( parentDiskSystem );
		
		status_t set_type_result = parentDiskSystem->SetType( partition, fType, this );
		
		if( set_type_result != B_OK ) {
			SetErrorMessage( "Setting partition type failed!" );
			return set_type_result;
		}
		
		return B_OK;
		
	} else {
		SetErrorMessage( "Couldn't find partition!" );
		return B_ENTRY_NOT_FOUND;
	}
	
}
