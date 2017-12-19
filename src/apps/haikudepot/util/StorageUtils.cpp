/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "StorageUtils.h"

#include <stdio.h>
#include <errno.h>

#include <Directory.h>
#include <File.h>
#include <Entry.h>
#include <String.h>

#include "Logger.h"

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
		BPath directoryEntryPath;

		result = directoryEntry.GetPath(&directoryEntryPath);

		if (result == B_OK) {
			result = ExistsObject(directoryEntryPath, &exists, &isDirectory,
				NULL);
		}

		if (result == B_OK) {
			if (isDirectory)
				RemoveDirectoryContents(directoryEntryPath);

			if (remove(directoryEntryPath.Path()) == 0) {
				if (Logger::IsDebugEnabled()) {
					fprintf(stdout, "did delete [%s]\n",
						directoryEntryPath.Path());
				}
			} else {
				fprintf(stderr, "unable to delete [%s]\n",
					directoryEntryPath.Path());
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
StorageUtils::ExistsObject(BPath& path,
	bool* exists,
	bool* isDirectory,
	off_t* size)
{
	struct stat s;

	if (exists != NULL)
		*exists = false;

	if (isDirectory != NULL)
		*isDirectory = false;

	if (size != NULL)
		*size = 0;

	if (-1 == stat(path.Path(), &s)) {
		if (ENOENT != errno)
			 return B_ERROR;
	} else {
		if (exists != NULL)
			*exists = true;

		if (isDirectory != NULL)
			*isDirectory = S_ISDIR(s.st_mode);

		if (size != NULL)
			*size = s.st_size;
	}

	return B_OK;
}
