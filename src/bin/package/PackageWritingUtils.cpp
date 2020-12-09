/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageWritingUtils.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <package/hpkg/HPKGDefs.h>

#include <AutoDeleter.h>
#include <AutoDeleterPosix.h>


status_t
add_current_directory_entries(BPackageWriter& packageWriter,
	BPackageWriterListener& listener, bool skipPackageInfo)
{
	// open the current directory
	DIR* dir = opendir(".");
	if (dir == NULL) {
		listener.PrintError("Error: Failed to opendir '.': %s\n",
			strerror(errno));
		return errno;
	}
	DirCloser dirCloser(dir);

	while (dirent* entry = readdir(dir)) {
		// skip "." and ".."
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		// skip the .PackageInfo, if requested
		if (skipPackageInfo
			&& strcmp(entry->d_name,
				BPackageKit::BHPKG::B_HPKG_PACKAGE_INFO_FILE_NAME) == 0) {
			continue;
		}

		status_t error = packageWriter.AddEntry(entry->d_name);
		if (error != B_OK)
			return error;
	}

	return B_OK;
}
