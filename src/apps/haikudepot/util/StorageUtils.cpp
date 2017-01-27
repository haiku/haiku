/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "StorageUtils.h"

#include <Directory.h>
#include <File.h>
#include <Entry.h>
#include <String.h>

#include <stdio.h>
#include <errno.h>

#define FILE_TO_STRING_BUFFER_LEN 64


/* This method will append the contents of the file at the supplied path to the
 * string provided.
 */

status_t
StorageUtils::AppendToString(BPath& path, BString& result)
{
	BFile file(path.Path(), O_RDONLY);
	uint8_t buffer[FILE_TO_STRING_BUFFER_LEN];
	size_t buffer_read;

	while((buffer_read = file.Read(buffer, FILE_TO_STRING_BUFFER_LEN)) > 0)
		result.Append((char *) buffer, buffer_read);

	return (status_t) buffer_read;
}


/* This method will traverse the directory structure and will remove all of the
 * files that are present in the directories as well as the directories
 * themselves.
 */

status_t
StorageUtils::RemoveDirectoryContents(BPath& path)
{
	BDirectory directory(path.Path());
	BEntry directoryEntry;
	status_t result = B_OK;

	while (result == B_OK &&
		directory.GetNextEntry(&directoryEntry) != B_ENTRY_NOT_FOUND) {

		bool exists = false;
		bool isDirectory = false;
		BPath directroyEntryPath;

		result = directoryEntry.GetPath(&directroyEntryPath);

		if (result == B_OK)
			result = ExistsDirectory(directroyEntryPath, &exists, &isDirectory);

		if (result == B_OK) {
			if (isDirectory)
				RemoveDirectoryContents(directroyEntryPath);

			if (remove(directroyEntryPath.Path()) == 0) {
				fprintf(stdout, "did delete [%s]\n",
					directroyEntryPath.Path());
			} else {
				fprintf(stderr, "unable to delete [%s]\n",
					directroyEntryPath.Path());
				result = B_ERROR;
			}
		}

	}

	return result;
}


/* This method checks to see if a file object exists at the path specified.  If
 * something does exist then the value of the 'exists' pointer is set to true.
 * If the object is a directory then this value is also set to true.
 */

status_t
StorageUtils::ExistsDirectory(BPath& directory,
	bool* exists,
	bool* isDirectory)
{
	struct stat s;

	*exists = false;
	*isDirectory = false;

	if (-1 == stat(directory.Path(), &s)) {
		if (ENOENT != errno)
			 return B_ERROR;
	} else {
		*exists = true;
		*isDirectory = S_ISDIR(s.st_mode);
	}

	return B_OK;
}
