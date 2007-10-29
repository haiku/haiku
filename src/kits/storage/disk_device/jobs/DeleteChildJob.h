/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DELETE_CHILD_JOB_H
#define _DELETE_CHILD_JOB_H

#include "DiskDeviceJob.h"


namespace BPrivate {


class DeleteChildJob : public DiskDeviceJob {
public:

								DeleteChildJob(PartitionReference* partition,
									PartitionReference* child);
	virtual						~DeleteChildJob();

	virtual	status_t			Do();
};


}	// namespace BPrivate

using BPrivate::DeleteChildJob;

#endif	// _DELETE_CHILD_JOB_H
