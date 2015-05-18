/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ABSTRACT_EMPTY_DIRECTORY_JOB_H
#define ABSTRACT_EMPTY_DIRECTORY_JOB_H


#include <Job.h>


class BEntry;


class AbstractEmptyDirectoryJob : public BSupportKit::BJob {
public:
								AbstractEmptyDirectoryJob(const BString& name);

protected:
			status_t			CreateAndEmpty(const char* path) const;

private:
			status_t			_EmptyDirectory(BEntry& directoryEntry,
									bool remove) const;
};


#endif // ABSTRACT_EMPTY_DIRECTORY_JOB_H
