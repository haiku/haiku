/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "StorageUtils.h"

#include <errno.h>
#include <stdlib.h>

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Entry.h>
#include <String.h>

#include "HaikuDepotConstants.h"
#include "Logger.h"

#define FILE_TO_STRING_BUFFER_LEN 64


static bool sAreWorkingFilesAvailable = true;


/*static*/ bool
StorageUtils::AreWorkingFilesAvailable()
{
	return sAreWorkingFilesAvailable;
}


/*static*/ void
StorageUtils::SetWorkingFilesUnavailable()
{
	sAreWorkingFilesAvailable = false;
}


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

			if (remove(directoryEntryPath.Path()) == 0)
				HDDEBUG("did delete [%s]", directoryEntryPath.Path());
			else {
				HDERROR("unable to delete [%s]", directoryEntryPath.Path());
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
StorageUtils::ExistsObject(const BPath& path,
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


/*! This method will check that it is possible to write to the specified file.
    This may create the file, write some data to it and then read that data
    back again to be sure.  This can be used as an effective safety measure as
    the application starts up in order to ensure that the storage systems are
    in place for the application to startup.

    It is assumed here that the directory containing the test file exists.
*/

/*static*/ status_t
StorageUtils::CheckCanWriteTo(const BPath& path)
{
	status_t result = B_OK;
	bool exists = false;
	uint8 buffer[16];

	// create some random latin letters into the buffer to write.
	for (int i = 0; i < 16; i++)
		buffer[i] = 65 + (abs(rand()) % 26);

	if (result == B_OK)
		result = ExistsObject(path, &exists, NULL, NULL);

	if (result == B_OK && exists) {
		HDTRACE("an object exists at the candidate path "
			"[%s] - it will be deleted", path.Path());

		if (remove(path.Path()) == 0) {
			HDTRACE("did delete the candidate file [%s]", path.Path());
		} else {
			HDERROR("unable to delete the candidate file [%s]", path.Path());
			result = B_ERROR;
		}
	}

	if (result == B_OK) {
		BFile file(path.Path(), O_WRONLY | O_CREAT);
		if (file.Write(buffer, 16) != 16) {
			HDERROR("unable to write test data to candidate file [%s]",
				path.Path());
			result = B_ERROR;
		}
	}

	if (result == B_OK) {
		BFile file(path.Path(), O_RDONLY);
		uint8 readBuffer[16];
		if (file.Read(readBuffer, 16) != 16) {
			HDERROR("unable to read test data from candidate file [%s]",
				path.Path());
			result = B_ERROR;
		}

		for (int i = 0; result == B_OK && i < 16; i++) {
			if (readBuffer[i] != buffer[i]) {
				HDERROR("mismatched read..write check on candidate file [%s]",
					path.Path());
				result = B_ERROR;
			}
		}
	}

	return result;
}


/*! As the application runs it will need to store some files into the local
    disk system.  This method, given a leafname, will write into the supplied
    path variable, a final path where this leafname should be stored.
*/

/*static*/ status_t
StorageUtils::LocalWorkingFilesPath(const BString leaf, BPath& path,
	bool failOnCreateDirectory)
{
	BPath resultPath;
	status_t result = B_OK;

	if (result == B_OK)
		result = find_directory(B_USER_CACHE_DIRECTORY, &resultPath);

	if (result == B_OK)
		result = resultPath.Append(CACHE_DIRECTORY_APP);

	if (result == B_OK) {
		if (failOnCreateDirectory)
			result = create_directory(resultPath.Path(), 0777);
		else
			create_directory(resultPath.Path(), 0777);
	}

	if (result == B_OK)
		result = resultPath.Append(leaf);

	if (result == B_OK)
		path.SetTo(resultPath.Path());
	else {
		path.Unset();
		HDERROR("unable to find the user cache file for "
			"[%s] data; %s", leaf.String(), strerror(result));
	}

	return result;
}


/*static*/ status_t
StorageUtils::LocalWorkingDirectoryPath(const BString leaf, BPath& path,
	bool failOnCreateDirectory)
{
	BPath resultPath;
	status_t result = B_OK;

	if (result == B_OK)
		result = find_directory(B_USER_CACHE_DIRECTORY, &resultPath);

	if (result == B_OK)
		result = resultPath.Append(CACHE_DIRECTORY_APP);

	if (result == B_OK)
		result = resultPath.Append(leaf);

	if (result == B_OK) {
		if (failOnCreateDirectory)
			result = create_directory(resultPath.Path(), 0777);
		else
			create_directory(resultPath.Path(), 0777);
	}

	if (result == B_OK)
		path.SetTo(resultPath.Path());
	else {
		path.Unset();
		HDERROR("unable to find the user cache directory for "
			"[%s] data; %s", leaf.String(), strerror(result));
	}

	return result;
}


/*static*/ status_t
StorageUtils::SwapExtensionOnPath(BPath& path, const char* extension)
{
	BPath parent;
	status_t result = path.GetParent(&parent);
	if (result == B_OK) {
		path.SetTo(parent.Path(),
			SwapExtensionOnPathComponent(path.Leaf(), extension).String());
	}
	return result;
}


/*static*/ BString
StorageUtils::SwapExtensionOnPathComponent(const char* pathComponent,
	const char* extension)
{
	BString result(pathComponent);
	int32 lastDot = result.FindLast(".");
	if (lastDot != B_ERROR) {
		result.Truncate(lastDot);
	}
	result.Append(".");
	result.Append(extension);
	return result;
}