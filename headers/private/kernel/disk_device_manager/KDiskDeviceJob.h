// KDiskDeviceJob.h

#ifndef _K_DISK_DEVICE_JOB_H
#define _K_DISK_DEVICE_JOB_H

#include "disk_device_manager.h"
#include <KPartitionVisitor.h>

struct user_disk_device_job_info;

namespace BPrivate {
namespace DiskDevice {

class KDiskDeviceJobQueue;

/**
 * Represents some action executed on the disk device.
 * 
 * - 
 */
class KDiskDeviceJob {
public:
	/**
	 * Creates a new job
	 * 
	 * \param type actual type of the job (see DiskDeviceDefs.h for details)
	 * \param partitionID the partition/device on which the action should be executed
	 * \param scopeID partition/device which is the highest in the hierarchy (i.e. closest
	 * to the root) that can be affected by the action
 * 		- every descendant of this partition is marked busy and all ancestors are marked descendant-
	busy
	 */
	KDiskDeviceJob(uint32 type, partition_id partitionID,
				   partition_id scopeID = -1);
	virtual ~KDiskDeviceJob();

	/**
	 * Unique identification of the job
	 */
	disk_job_id ID() const;

	void SetJobQueue(KDiskDeviceJobQueue *queue);
	KDiskDeviceJobQueue *JobQueue() const;

	/**
	 * Gets actual type of the action
	 */
	uint32 Type() const;

	
	void SetStatus(uint32 status);
	uint32 Status() const;

	status_t SetDescription(const char *description);
	const char *Description() const;

	partition_id PartitionID() const;
	partition_id ScopeID() const;

	status_t SetErrorMessage(const char *message);
	const char *ErrorMessage() const;

	void SetTaskCount(int32 count);
	int32 TaskCount() const;

	void SetCompletedTasks(int32 count);
	int32 CompletedTasks() const;

	void SetInterruptProperties(uint32 properties);
	uint32 InterruptProperties() const;

	virtual void GetTaskDescription(char *description);

	void UpdateProgress(float progress);
		// may trigger a notification
	void UpdateExtraProgress(const char *info);
		// triggers a notification
	float Progress() const;

	
	status_t GetInfo(user_disk_device_job_info *info);
	status_t GetProgressInfo(disk_device_job_progress_info *info);

	/**
	 * Do the actual work of the job.
	 * 
	 * - is supposed to be implemented in descendants
	 * - doesn't have any parameter - every operation needs different ones -> they're passed
	 * 		to the constructor
	 * - the implementations will 
	 * 		- check the parameters given in constructor (e.g. if given partition exists...)
	 * 		- check whether the partition has needed disk system (its own or parent - depends
	 * 			on the operation)
	 * 		- using the disk system, validate the operation for given params
	 * 		- finally execute the action
	 * 
	 * \return B_OK when everything went OK, some error otherwise
	 */
	virtual status_t Do() = 0;

private:
	static disk_job_id _NextID();

	disk_job_id			fID;
	KDiskDeviceJobQueue	*fJobQueue;
	uint32				fType;
	uint32				fStatus;
	partition_id		fPartitionID;
	partition_id		fScopeID;
	char				*fDescription;
	char				*fErrorMessage;
	int32				fTaskCount;
	int32				fCompletedTasks;
	uint32				fInterruptProperties;
	float				fProgress;

	static disk_job_id	fNextID;
	
private:
	/**
	 * Visitor which checks if every descendant of given partition is busy or descendant-busy
	 */
	struct IsNotBusyVisitor	: KPartitionVisitor {
		virtual bool VisitPre(KPartition * partition);
	};
	IsNotBusyVisitor fNotBusyVisitor;
	
protected: 
	//some stuff useful for all descendants
		
	/**
	 * Checks if there's any descendant which is not busy/descendant-busy.
	 * 
	 * - the condition of busy descendant is common for many disk device operations ->
	 * 		many jobs can use this
	 * 
	 * \param partition the root of checked subtree of the whole partition hieararchy
	 */
	bool isPartitionNotBusy( KPartition * partition );	
	
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KDiskDeviceJob;

#endif	// _DISK_DEVICE_JOB_H
