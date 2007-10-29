/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _UNINITIALIZE_JOB_H
#define _UNINITIALIZE_JOB_H

#include "DiskDeviceJob.h"


namespace BPrivate {


class UninitializeJob : public DiskDeviceJob {
public:

								UninitializeJob(PartitionReference* partition);
	virtual						~UninitializeJob();

	virtual	status_t			Do();
};


}	// namespace BPrivate

using BPrivate::UninitializeJob;

#endif	// _UNINITIALIZE_JOB_H
