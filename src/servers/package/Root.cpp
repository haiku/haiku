/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "Root.h"

#include <Directory.h>
#include <Entry.h>
#include <Path.h>

#include "DebugSupport.h"


// #pragma mark - VolumeJob


struct Root::VolumeJob : public Job {
	VolumeJob(Volume* volume, void (Root::*method)(Volume*))
		:
		fVolume(volume),
		fMethod(method)
	{
	}

	virtual void Do()
	{
		(fVolume->GetRoot()->*fMethod)(fVolume);
	}

private:
	Volume*	fVolume;
	void	(Root::*fMethod)(Volume*);
};


// #pragma mark - Root


Root::Root()
	:
	fNodeRef(),
	fPath(),
	fSystemVolume(NULL),
	fCommonVolume(NULL),
	fHomeVolume(NULL),
	fJobQueue(),
	fJobRunner(-1)
{
}


Root::~Root()
{
	fJobQueue.Close();

	if (fJobRunner >= 0)
		wait_for_thread(fJobRunner, NULL);
}


status_t
Root::Init(const node_ref& nodeRef)
{
	fNodeRef = nodeRef;

	// init job queue and spawn job runner thread
	status_t error = fJobQueue.Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	fJobRunner = spawn_thread(&_JobRunnerEntry, "job runner", B_NORMAL_PRIORITY,
		this);
	if (fJobRunner < 0)
		RETURN_ERROR(fJobRunner);

	// get the path
	BDirectory directory;
	error = directory.SetTo(&fNodeRef);
	if (error != B_OK) {
		ERROR("Root::Init(): failed to open directory: %s\n", strerror(error));
		RETURN_ERROR(error);
	}

	BEntry entry;
	error = directory.GetEntry(&entry);

	BPath path;
	if (error == B_OK)
		error = entry.GetPath(&path);

	if (error != B_OK) {
		ERROR("Root::Init(): failed to get directory path: %s\n",
			strerror(error));
		RETURN_ERROR(error);
	}

	fPath = path.Path();
	if (fPath.IsEmpty())
		RETURN_ERROR(B_NO_MEMORY);

	resume_thread(fJobRunner);

	return B_OK;
}


status_t
Root::RegisterVolume(Volume* volume)
{
	Volume** volumeToSet = _GetVolume(volume->MountType());
	if (volumeToSet == NULL)
		return B_BAD_VALUE;

	if (*volumeToSet != NULL) {
		ERROR("Root::RegisterVolume(): can't register volume at \"%s\", since "
			"there's already volume at \"%s\" with the same type.\n",
			volume->Path().String(), (*volumeToSet)->Path().String());
		return B_BAD_VALUE;
	}

	*volumeToSet = volume;
	volume->SetRoot(this);

	// queue a job for reading the volume's packages
	status_t error = _QueueJob(
		new(std::nothrow) VolumeJob(volume, &Root::_InitPackages));
	if (error != B_OK) {
		volume->SetRoot(NULL);
		*volumeToSet = NULL;
		return error;
	}

	return B_OK;
}


void
Root::UnregisterVolume(Volume* volume)
{
	Volume** volumeToSet = _GetVolume(volume->MountType());
	if (volumeToSet == NULL || *volumeToSet != volume) {
		ERROR("Root::UnregisterVolume(): can't unregister unknown volume at "
			"\"%s.\n", volume->Path().String());
		return;
	}

	*volumeToSet = NULL;

	// Use the job queue to delete the volume to make sure there aren't any
	// pending jobs that reference the volume.
	_QueueJob(new(std::nothrow) VolumeJob(volume, &Root::_DeleteVolume));
}


Volume*
Root::FindVolume(dev_t deviceID) const
{
	Volume* volumes[] = { fSystemVolume, fCommonVolume, fHomeVolume };
	for (size_t i = 0; i < sizeof(volumes) / sizeof(volumes[0]); i++) {
		Volume* volume = volumes[i];
		if (volume != NULL && volume->DeviceID() == deviceID)
			return volume;
	}

	return NULL;
}


void
Root::VolumeNodeMonitorEventOccurred(Volume* volume)
{
	_QueueJob(
		new(std::nothrow) VolumeJob(volume, &Root::_ProcessNodeMonitorEvents));
}


void
Root::LastReferenceReleased()
{
}


Volume**
Root::_GetVolume(PackageFSMountType mountType)
{
	switch (mountType) {
		case PACKAGE_FS_MOUNT_TYPE_SYSTEM:
			return &fSystemVolume;
		case PACKAGE_FS_MOUNT_TYPE_COMMON:
			return &fCommonVolume;
		case PACKAGE_FS_MOUNT_TYPE_HOME:
			return &fHomeVolume;
		case PACKAGE_FS_MOUNT_TYPE_CUSTOM:
		default:
			return NULL;
	}
}


void
Root::_InitPackages(Volume* volume)
{
	volume->InitPackages(this);
}


void
Root::_DeleteVolume(Volume* volume)
{
	delete volume;
}


void
Root::_ProcessNodeMonitorEvents(Volume* volume)
{
	volume->ProcessPendingNodeMonitorEvents();

	if (volume->HasPendingPackageActivationChanges())
		volume->ProcessPendingPackageActivationChanges();
}


status_t
Root::_QueueJob(Job* job)
{
	if (job == NULL)
		return B_NO_MEMORY;

	BReference<Job> jobReference(job, true);
	if (!fJobQueue.QueueJob(job)) {
		// job queue already closed
		return B_BAD_VALUE;
	}

	return B_OK;
}


/*static*/ status_t
Root::_JobRunnerEntry(void* data)
{
	return ((Root*)data)->_JobRunner();
}


status_t
Root::_JobRunner()
{
	while (Job* job = fJobQueue.DequeueJob()) {
		job->Do();
		job->ReleaseReference();
	}

	return B_OK;
}
