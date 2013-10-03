/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "Root.h"

#include <Alert.h>
#include <Directory.h>
#include <Entry.h>
#include <package/PackageDefs.h>
#include <Path.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <Server.h>

#include <package/DaemonDefs.h>
#include <package/manager/Exceptions.h>

#include "DebugSupport.h"
#include "PackageManager.h"


using namespace BPackageKit::BPrivate;
using namespace BPackageKit::BManager::BPrivate;


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


// #pragma mark - RequestJob


struct Root::RequestJob : public Job {
	RequestJob(Root* root, BMessage* message)
		:
		fRoot(root),
		fMessage(message)
	{
	}

	virtual void Do()
	{
		fRoot->_HandleRequest(fMessage.Get());
	}

private:
	Root*					fRoot;
	ObjectDeleter<BMessage>	fMessage;
};


// #pragma mark - Root


Root::Root()
	:
	fLock("packagefs root"),
	fNodeRef(),
	fIsSystemRoot(false),
	fPath(),
	fSystemVolume(NULL),
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
Root::Init(const node_ref& nodeRef, bool isSystemRoot)
{
	fNodeRef = nodeRef;
	fIsSystemRoot = isSystemRoot;

	// init members and spawn job runner thread
	status_t error = fJobQueue.Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	error = fLock.InitCheck();
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
	AutoLocker<BLocker> locker(fLock);

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
	AutoLocker<BLocker> locker(fLock);

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
	AutoLocker<BLocker> locker(fLock);

	Volume* volumes[] = { fSystemVolume, fHomeVolume };
	for (size_t i = 0; i < sizeof(volumes) / sizeof(volumes[0]); i++) {
		Volume* volume = volumes[i];
		if (volume != NULL && volume->DeviceID() == deviceID)
			return volume;
	}

	return NULL;
}


Volume*
Root::GetVolume(BPackageInstallationLocation location)
{
	switch ((BPackageInstallationLocation)location) {
		case B_PACKAGE_INSTALLATION_LOCATION_SYSTEM:
			return fSystemVolume;
		case B_PACKAGE_INSTALLATION_LOCATION_HOME:
			return fHomeVolume;
		default:
			return NULL;
	}
}


void
Root::HandleRequest(BMessage* message)
{
	RequestJob* job = new(std::nothrow) RequestJob(this, message);
	if (job == NULL) {
		delete message;
		return;
	}

	_QueueJob(job);
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
		case PACKAGE_FS_MOUNT_TYPE_HOME:
			return &fHomeVolume;
		case PACKAGE_FS_MOUNT_TYPE_CUSTOM:
		default:
			return NULL;
	}
}


Volume*
Root::_NextVolumeFor(Volume* volume)
{
	if (volume == NULL)
		return NULL;

	PackageFSMountType mountType = volume->MountType();

	do {
		switch (mountType) {
			case PACKAGE_FS_MOUNT_TYPE_HOME:
				mountType = PACKAGE_FS_MOUNT_TYPE_SYSTEM;
				break;
			case PACKAGE_FS_MOUNT_TYPE_SYSTEM:
			case PACKAGE_FS_MOUNT_TYPE_CUSTOM:
			default:
				return NULL;
		}

		volume = *_GetVolume(mountType);
	} while (volume == NULL);

	return volume;
}


void
Root::_InitPackages(Volume* volume)
{
	if (volume->InitPackages(this) == B_OK) {
		AutoLocker<BLocker> locker(fLock);
		Volume* nextVolume = _NextVolumeFor(volume);
		Volume* nextNextVolume = _NextVolumeFor(nextVolume);
		locker.Unlock();

		volume->InitialVerify(nextVolume, nextNextVolume);
	}
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

	if (!volume->HasPendingPackageActivationChanges())
		return;

	// If this is not the system root, just activate/deactivate the packages.
	if (!fIsSystemRoot) {
		volume->ProcessPendingPackageActivationChanges();
		return;
	}

	// For the system root do the full dependency analysis.

	PRINT("Root::_ProcessNodeMonitorEvents(): running package manager...\n");
	try {
		PackageManager packageManager(this, volume);
		packageManager.HandleUserChanges();
	} catch (BNothingToDoException&) {
		PRINT("Root::_ProcessNodeMonitorEvents(): -> nothing to do\n");
	} catch (std::bad_alloc&) {
		_ShowError(
			"Insufficient memory while trying to apply package changes.");
	} catch (BFatalErrorException& exception) {
		if (exception.Error() == B_OK) {
			_ShowError(exception.Message());
		} else {
			_ShowError(BString().SetToFormat("%s: %s",
				exception.Message().String(), strerror(exception.Error())));
		}
		// TODO: Print exception.Details()?
	} catch (BAbortedByUserException&) {
		PRINT("Root::_ProcessNodeMonitorEvents(): -> aborted by user\n");
	}

	volume->ClearPackageActivationChanges();
}


void
Root::_HandleRequest(BMessage* message)
{
	int32 location;
	if (message->FindInt32("location", &location) != B_OK
		|| location < 0
		|| location >= B_PACKAGE_INSTALLATION_LOCATION_ENUM_COUNT) {
		return;
	}

	// get the volume and let it handle the message
	AutoLocker<BLocker> locker(fLock);
	Volume* volume = GetVolume((BPackageInstallationLocation)location);
	locker.Unlock();

	if (volume != NULL) {
		switch (message->what) {
			case B_MESSAGE_GET_INSTALLATION_LOCATION_INFO:
				volume->HandleGetLocationInfoRequest(message);
				break;
			case B_MESSAGE_COMMIT_TRANSACTION:
				volume->HandleCommitTransactionRequest(message);
				break;
		}
	}
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


/*static*/ void
Root::_ShowError(const char* errorMessage)
{
	BServer* server = dynamic_cast<BServer*>(be_app);
	if (server != NULL && server->InitGUIContext() == B_OK) {
		BAlert* alert = new(std::nothrow) BAlert("Package error",
			errorMessage, "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		if (alert != NULL) {
			alert->SetShortcut(0, B_ESCAPE);
			alert->Go();
			return;
		}
	}

	ERROR("Root::_ShowError(): %s\n", errorMessage);
}
