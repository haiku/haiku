/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _REPAIR_JOB_H
#define _REPAIR_JOB_H

#include "DiskDeviceJob.h"


namespace BPrivate {


class RepairJob : public DiskDeviceJob {
public:

								RepairJob(PartitionReference* partition,
									bool checkOnly);
	virtual						~RepairJob();

	virtual	status_t			Do();

private:
			bool				fCheckOnly;
};


}	// namespace BPrivate

using BPrivate::RepairJob;

#endif	// _REPAIR_JOB_H
