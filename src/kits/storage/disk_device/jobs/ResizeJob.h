/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _RESIZE_JOB_H
#define _RESIZE_JOB_H

#include "DiskDeviceJob.h"


namespace BPrivate {


class ResizeJob : public DiskDeviceJob {
public:

								ResizeJob(PartitionReference* partition,
									PartitionReference* child, off_t size,
									off_t contentSize);
	virtual						~ResizeJob();

	virtual	status_t			Do();

protected:
			off_t				fSize;
			off_t				fContentSize;
};


}	// namespace BPrivate

using BPrivate::ResizeJob;

#endif	// _RESIZE_JOB_H
