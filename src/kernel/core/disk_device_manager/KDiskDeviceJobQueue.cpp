// KDiskDeviceJobQueue.cpp

#include <new>
#include <stdio.h>
#include <string.h>

#include <Vector.h>

#include "KDiskDevice.h"
#include "KDiskDeviceJob.h"
#include "KDiskDeviceJobQueue.h"
#include "KDiskDeviceManager.h"
#include "KDiskDeviceUtils.h"

// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT printf

using namespace std;

// flags
enum {
	JOB_QUEUE_EXECUTING				= 0x01,
	JOB_QUEUE_CANCELED				= 0x02,
	JOB_QUEUE_REVERSE				= 0x04,
	JOB_QUEUE_PAUSED				= 0x08,
	JOB_QUEUE_PAUSE_REQUESTED		= 0x10,
};

struct KDiskDeviceJobQueue::JobQueue : Vector<KDiskDeviceJob*> {};

// constructor
KDiskDeviceJobQueue::KDiskDeviceJobQueue()
	: fDevice(NULL),
	  fActiveJob(0),
	  fJobs(NULL),
	  fFlags(0),
	  fThread(-1),
	  fSyncSemaphore(-1)
{
	fJobs = new(nothrow) JobQueue;
}

// destructor
KDiskDeviceJobQueue::~KDiskDeviceJobQueue()
{
	// Delete the semaphore. This awakes our thread in case it is paused --
	// that shouldn't happen, though.
	if (fSyncSemaphore >= 0)
		delete_sem(fSyncSemaphore);
	// The thread should not run or is just deleting us via the manager
	// method DeleteJobQueue(). At any rate, fThread should be unset.
	if (fThread >= 0) {
		// something's weird
		DBG(OUT("WARNING: KDiskDeviceJobQueue::~KDiskDeviceJobQueue(): jobber "
				" thread is still running!\n"));
	}
	// delete the jobs and the queue
	if (fJobs) {
		int32 count = fJobs->Count();
		for (int32 i = 0; i < count; i++)
			delete fJobs->ElementAt(i);
		delete fJobs;
	}
	// unset the device
	SetDevice(NULL);
}

// InitCheck
status_t
KDiskDeviceJobQueue::InitCheck() const
{
	return (fJobs ? B_OK : B_NO_MEMORY);
}

// SetDevice
void
KDiskDeviceJobQueue::SetDevice(KDiskDevice *device)
{
	// unset the old device
	if (fDevice) {
		fDevice->Unregister();
		fDevice = NULL;
	}
	// set the new one
	if (device) {
		fDevice = device;
		fDevice->Register();
	}
}

// Device
BPrivate::DiskDevice::KDiskDevice *
KDiskDeviceJobQueue::Device() const
{
	return fDevice;
}

// ActiveJob
KDiskDeviceJob *
KDiskDeviceJobQueue::ActiveJob() const
{
	return JobAt(fActiveJob);
}

// AddJob
bool
KDiskDeviceJobQueue::AddJob(KDiskDeviceJob *job)
{
	if (InitCheck() != B_OK || IsExecuting() || !job || job->JobQueue())
		return false;
	status_t error = fJobs->PushBack(job);
	if (error == B_OK)
		job->SetJobQueue(this);
	return (error == B_OK);
}

// RemoveJob
KDiskDeviceJob *
KDiskDeviceJobQueue::RemoveJob(int32 index)
{
	if (InitCheck() != B_OK || IsExecuting() || index < 0
		|| index >= fJobs->Count()) {
		return NULL;
	}
	KDiskDeviceJob *job = fJobs->ElementAt(index);
	fJobs->Erase(index);
	job->SetJobQueue(NULL);
	return job;
}

// RemoveJob
bool
KDiskDeviceJobQueue::RemoveJob(KDiskDeviceJob *job)
{
	if (InitCheck() != B_OK || IsExecuting() || !job
		|| job->JobQueue() != this) {
		return false;
	}
	return RemoveJob(fJobs->IndexOf(job));
}

// JobAt
KDiskDeviceJob *
KDiskDeviceJobQueue::JobAt(int32 index) const
{
	if (InitCheck() != B_OK || index < 0 || index >= fJobs->Count())
		return NULL;
	return fJobs->ElementAt(index);
}

// CountJobs
int32
KDiskDeviceJobQueue::CountJobs() const
{
	if (InitCheck() != B_OK)
		return 0;
	return fJobs->Count();
}

// Execute
status_t
KDiskDeviceJobQueue::Execute()
{
	if (InitCheck() != B_OK)
		return InitCheck();
	if (IsExecuting())
		return B_BAD_VALUE;
	// create a synchronization semaphore
	fSyncSemaphore = create_sem(0, "disk device job queue sync");
	if (fSyncSemaphore < 0)
		return fSyncSemaphore;
	// spawn a thread and run it
	fThread = spawn_thread(_ThreadEntry, "disk device jobber",
						   B_NORMAL_PRIORITY, this);
	if (fThread < 0) {
		delete_sem(fSyncSemaphore);
		fSyncSemaphore = -1;
		return fThread;
	}
	resume_thread(fThread);
	// wait till the thread has resumed its work
	acquire_sem(fSyncSemaphore);
	return B_OK;
}

// Cancel
status_t
KDiskDeviceJobQueue::Cancel(bool reverse)
{
	if (InitCheck() != B_OK)
		return InitCheck();
	if (!IsExecuting() || IsCanceled())
		return B_BAD_VALUE;
	KDiskDeviceJob *job = ActiveJob();
	if (!job)
		return B_ERROR;
	// check, if the active job allows canceling
	if (!(job->InterruptProperties() & B_DISK_DEVICE_JOB_CAN_CANCEL))
		return B_BAD_VALUE;
	if (reverse && !(job->InterruptProperties()
					 & B_DISK_DEVICE_JOB_REVERSE_ON_CANCEL)) {
		return B_BAD_VALUE;
	}
	// update the flags
	fFlags |= JOB_QUEUE_CANCELED;
	if (reverse)
		fFlags |= JOB_QUEUE_REVERSE;
	// let the thread continue, if paused
	if (IsPaused() || IsPauseRequested())
		Continue();
	return B_OK;
}

// Pause
status_t
KDiskDeviceJobQueue::Pause()
{
	if (InitCheck() != B_OK)
		return InitCheck();
	if (!IsExecuting() || IsCanceled() || IsPaused() || IsPauseRequested())
		return B_BAD_VALUE;
	fFlags |= JOB_QUEUE_PAUSE_REQUESTED;
	return B_OK;
}

// Continue
status_t
KDiskDeviceJobQueue::Continue()
{
	if (InitCheck() != B_OK)
		return InitCheck();
	if (!IsExecuting() || !(IsPaused() || IsPauseRequested()))
		return B_BAD_VALUE;
	if (IsPaused()) {
		fFlags &= ~(uint32)JOB_QUEUE_PAUSED;
		release_sem(fSyncSemaphore);
	} else
		fFlags &= ~(uint32)JOB_QUEUE_PAUSE_REQUESTED;
	return B_OK;
}

// ReadyToPause
sem_id
KDiskDeviceJobQueue::ReadyToPause()
{
	if (InitCheck() != B_OK)
		return InitCheck();
	if (!IsExecuting() || !IsPauseRequested())
		return B_BAD_VALUE;
	fFlags &= ~(uint32)JOB_QUEUE_PAUSE_REQUESTED;
	fFlags |= JOB_QUEUE_PAUSED;
	return fSyncSemaphore;
}

// IsExecuting
bool
KDiskDeviceJobQueue::IsExecuting() const
{
	return (fFlags & JOB_QUEUE_EXECUTING);
}

// IsCanceled
bool
KDiskDeviceJobQueue::IsCanceled() const
{
	return (fFlags & JOB_QUEUE_CANCELED);
}

// ShallReverse
bool
KDiskDeviceJobQueue::ShallReverse() const
{
	return (fFlags & JOB_QUEUE_REVERSE);
}

// IsPaused
bool
KDiskDeviceJobQueue::IsPaused() const
{
	return (fFlags & JOB_QUEUE_PAUSED);
}

// IsPauseRequested
bool
KDiskDeviceJobQueue::IsPauseRequested() const
{
	return (fFlags & JOB_QUEUE_PAUSE_REQUESTED);
}

// _ThreadLoop
int32
KDiskDeviceJobQueue::_ThreadLoop()
{
// TODO: notifications
	// mark all jobs scheduled
	for (int32 i = 0; KDiskDeviceJob *job = JobAt(i); i++)
		job->SetStatus(B_DISK_DEVICE_JOB_SCHEDULED);
	// mark the queue executing and notify the thread waiting in Execute()
	fFlags |= JOB_QUEUE_EXECUTING;
	release_sem(fSyncSemaphore);
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// main loop: process all jobs
	while (KDiskDeviceJob *activeJob = ActiveJob()) {
		activeJob->SetStatus(B_DISK_DEVICE_JOB_IN_PROGRESS);
		status_t error = activeJob->Do();
		if (error == B_OK) {
			if (ManagerLocker locker = manager) {
				// if canceled, mark this and all succeeding jobs canceled
				if (IsCanceled()) {
					for (int32 i = fActiveJob; KDiskDeviceJob *job = JobAt(i);
						 i++) {
						job->SetStatus(B_DISK_DEVICE_JOB_CANCELED);
					}
					break;
				}
				// if pause was requested ignore the request
				if (IsPauseRequested())
					Continue();
				// mark the job succeeded and go to the next one
				activeJob->SetStatus(B_DISK_DEVICE_JOB_SUCCEEDED);
				fActiveJob++;
			} else
				break;
		} else {
			// job failed: mark this and all succeeding jobs failed
			if (ManagerLocker locker = manager) {
				for (int32 i = fActiveJob; KDiskDeviceJob *job = JobAt(i); i++)
					job->SetStatus(B_DISK_DEVICE_JOB_FAILED);
			}
			break;
		}
	}
	// tell the manager, that we are done.
	if (ManagerLocker locker = manager) {
		fFlags &= ~(uint32)JOB_QUEUE_EXECUTING;
		fThread = -1;
		manager->DeleteJobQueue(this);
	}
	return 0;
}

// _ThreadEntry
int32
KDiskDeviceJobQueue::_ThreadEntry(void *data)
{
	return static_cast<KDiskDeviceJobQueue*>(data)->_ThreadLoop();
}

