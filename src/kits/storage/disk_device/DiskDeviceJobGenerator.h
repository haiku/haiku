/*
 * Copyright 2003-2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DISK_DEVICE_JOB_GENERATOR_H
#define _DISK_DEVICE_JOB_GENERATOR_H

#include <DiskDeviceDefs.h>


class BDiskDevice;
class BMutablePartition;
class BPartition;


namespace BPrivate {


class DiskDeviceJob;
class DiskDeviceJobQueue;


class DiskDeviceJobGenerator {
public:
								DiskDeviceJobGenerator(BDiskDevice* device,
									DiskDeviceJobQueue* jobQueue);
								~DiskDeviceJobGenerator();

			status_t			GenerateJobs();

private:
			status_t			_AddJob(DiskDeviceJob* job);

			status_t			_GenerateCleanupJobs(BPartition* partition);
			status_t			_GeneratePlacementJobs(BPartition* partition);
			status_t			_GenerateChildPlacementJobs(
									BPartition* partition);
			status_t			_GenerateRemainingJobs(BPartition* parent,
									BPartition* partition);

			BMutablePartition*	_GetMutablePartition(BPartition* partition);

			status_t			_GenerateInitializeJob(BPartition* partition);
			status_t			_GenerateUninitializeJob(BPartition* partition);
			status_t			_GenerateSetContentNameJob(
									BPartition* partition);
			status_t			_GenerateSetContentParametersJob(
									BPartition* partition);
			status_t			_GenerateDefragmentJob(BPartition* partition);
			status_t			_GenerateRepairJob(BPartition* partition,
									bool repair);

			status_t			_GenerateCreateChildJob(BPartition* parent,
									BPartition* partition);
			status_t			_GenerateDeleteChildJob(BPartition* parent,
									BPartition* child);
			status_t			_GenerateResizeJob(BPartition* partition);
			status_t			_GenerateMoveJob(BPartition* partition);
			status_t 			_GenerateSetNameJob(BPartition* parent,
									BPartition* partition);
			status_t			_GenerateSetTypeJob(BPartition* parent,
									BPartition* partition);
			status_t			_GenerateSetParametersJob(BPartition* parent,
									BPartition* partition);

			status_t			_CollectContentsToMove(BPartition* partition);

	static	int					_CompareMoveInfoPosition(const void* _a,
									const void* _b);

private:
			struct move_info;

			BDiskDevice*		fDevice;
			DiskDeviceJobQueue*	fJobQueue;
			move_info*			fMoveInfos;
			int32				fMoveInfoCount;
};


}	// namespace BPrivate

using BPrivate::DiskDeviceJobGenerator;

#endif	// _DISK_DEVICE_JOB_GENERATOR_H
