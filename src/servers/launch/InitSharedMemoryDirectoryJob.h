/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef INIT_SHARED_MEMORY_DIRECTORY_JOB_H
#define INIT_SHARED_MEMORY_DIRECTORY_JOB_H


#include "AbstractEmptyDirectoryJob.h"


class InitSharedMemoryDirectoryJob : public AbstractEmptyDirectoryJob {
public:
								InitSharedMemoryDirectoryJob();

protected:
	virtual	status_t			Execute();
};


#endif // INIT_SHARED_MEMORY_DIRECTORY_JOB_H
