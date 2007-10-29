/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DEFRAGMENT_JOB_H
#define _DEFRAGMENT_JOB_H

#include "DiskDeviceJob.h"


namespace BPrivate {


class DefragmentJob : public DiskDeviceJob {
public:

								DefragmentJob(PartitionReference* partition);
	virtual						~DefragmentJob();

	virtual	status_t			Do();
};


}	// namespace BPrivate

using BPrivate::DefragmentJob;

#endif	// _DEFRAGMENT_JOB_H
