/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MOVE_JOB_H
#define _MOVE_JOB_H

#include "DiskDeviceJob.h"


namespace BPrivate {


class MoveJob : public DiskDeviceJob {
public:

								MoveJob(PartitionReference* partition,
									PartitionReference* child);
	virtual						~MoveJob();

			status_t			Init(off_t offset,
									PartitionReference** contents,
									int32 contentsCount);

	virtual	status_t			Do();

protected:
			off_t				fOffset;
			PartitionReference** fContents;
			int32				fContentsCount;
};


}	// namespace BPrivate

using BPrivate::MoveJob;

#endif	// _MOVE_JOB_H
