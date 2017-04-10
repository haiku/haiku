/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "AbstractEmptyDirectoryJob.h"

#include <stdio.h>

#include <Directory.h>
#include <Entry.h>


AbstractEmptyDirectoryJob::AbstractEmptyDirectoryJob(const BString& name)
	:
	BJob(name)
{
}


status_t
AbstractEmptyDirectoryJob::CreateAndEmpty(const char* path) const
{
	BEntry entry(path);
	if (!entry.Exists()) {
		create_directory(path, 0777);

		status_t status = entry.SetTo(path);
		if (status != B_OK) {
			fprintf(stderr, "Cannot create directory \"%s\": %s\n", path,
				strerror(status));
			return status;
		}
	}

	return _EmptyDirectory(entry, false);
}


status_t
AbstractEmptyDirectoryJob::_EmptyDirectory(BEntry& directoryEntry,
	bool remove) const
{
	BDirectory directory(&directoryEntry);
	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK) {
		if (entry.IsDirectory())
			_EmptyDirectory(entry, true);
		else
			entry.Remove();
	}

	return remove ? directoryEntry.Remove() : B_OK;
}
