/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SET_STRING_JOB_H
#define _SET_STRING_JOB_H

#include "DiskDeviceJob.h"


namespace BPrivate {


class SetStringJob : public DiskDeviceJob {
public:

								SetStringJob(PartitionReference* partition,
									PartitionReference* child = NULL);
	virtual						~SetStringJob();

			status_t			Init(const char* string, uint32 jobType);

	virtual	status_t			Do();

protected:
			char*				fString;
			uint32				fJobType;
};


}	// namespace BPrivate

using BPrivate::SetStringJob;

#endif	// _SET_STRING_JOB_H
