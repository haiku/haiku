/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef INIT_TEMPORARY_DIRECTORY_JOB_H
#define INIT_TEMPORARY_DIRECTORY_JOB_H


#include "AbstractEmptyDirectoryJob.h"


class InitTemporaryDirectoryJob : public AbstractEmptyDirectoryJob {
public:
								InitTemporaryDirectoryJob();

protected:
	virtual	status_t			Execute();
};


#endif // INIT_TEMPORARY_DIRECTORY_JOB_H
