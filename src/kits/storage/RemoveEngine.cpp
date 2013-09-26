/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <RemoveEngine.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <Directory.h>
#include <Entry.h>
#include <Path.h>


namespace BPrivate {


// #pragma mark - BRemoveEngine


BRemoveEngine::BRemoveEngine()
	:
	fController(NULL)
{
}


BRemoveEngine::~BRemoveEngine()
{
}


BRemoveEngine::BController*
BRemoveEngine::Controller() const
{
	return fController;
}


void
BRemoveEngine::SetController(BController* controller)
{
	fController = controller;
}


status_t
BRemoveEngine::RemoveEntry(const Entry& entry)
{
	BPath pathBuffer;
	const char* path;
	status_t error = entry.GetPath(pathBuffer, path);
	if (error != B_OK)
		return error;

	return _RemoveEntry(path);
}


status_t
BRemoveEngine::_RemoveEntry(const char* path)
{
	// apply entry filter
	if (fController != NULL && !fController->EntryStarted(path))
		return B_OK;

	// stat entry
	struct stat st;
	if (lstat(path, &st) < 0) {
		return _HandleEntryError(path, errno, "Couldn't access \"%s\": %s\n",
			path, strerror(errno));
	}

	// recurse, if entry is a directory
	if (S_ISDIR(st.st_mode)) {
		// open directory
		BDirectory directory;
		status_t error = directory.SetTo(path);
		if (error != B_OK) {
			return _HandleEntryError(path, error,
				"Failed to open directory \"%s\": %s\n", path, strerror(error));
		}

		char buffer[sizeof(dirent) + B_FILE_NAME_LENGTH];
		dirent *entry = (dirent*)buffer;
		while (directory.GetNextDirents(entry, sizeof(buffer), 1) == 1) {
			if (strcmp(entry->d_name, ".") == 0
				|| strcmp(entry->d_name, "..") == 0) {
				continue;
			}

			// construct child entry path
			BPath childPath;
			error = childPath.SetTo(path, entry->d_name);
			if (error != B_OK) {
				return _HandleEntryError(path, error,
					"Failed to construct entry path from dir \"%s\" and name "
					"\"%s\": %s\n", path, entry->d_name, strerror(error));
			}

			// remove the entry
			error = _RemoveEntry(childPath.Path());
			if (error != B_OK) {
				if (fController != NULL
					&& fController->EntryFinished(path, error)) {
					return B_OK;
				}
				return error;
			}
		}
	}

	// remove entry
	if (S_ISDIR(st.st_mode)) {
		if (rmdir(path) < 0) {
			return _HandleEntryError(path, errno,
				"Failed to remove \"%s\": %s\n", path, strerror(errno));
		}
	} else {
		if (unlink(path) < 0) {
			return _HandleEntryError(path, errno,
				"Failed to unlink \"%s\": %s\n", path, strerror(errno));
		}
	}

	if (fController != NULL)
		fController->EntryFinished(path, B_OK);

	return B_OK;
}


void
BRemoveEngine::_NotifyErrorVarArgs(status_t error, const char* format,
	va_list args)
{
	if (fController != NULL) {
		BString message;
		message.SetToFormatVarArgs(format, args);
		fController->ErrorOccurred(message, error);
	}
}


status_t
BRemoveEngine::_HandleEntryError(const char* path, status_t error,
	const char* format, ...)
{
	if (fController == NULL)
		return error;

	va_list args;
	va_start(args, format);
	_NotifyErrorVarArgs(error, format, args);
	va_end(args);

	if (fController->EntryFinished(path, error))
		return B_OK;
	return error;
}


// #pragma mark - BController


BRemoveEngine::BController::BController()
{
}


BRemoveEngine::BController::~BController()
{
}


bool
BRemoveEngine::BController::EntryStarted(const char* path)
{
	return true;
}


bool
BRemoveEngine::BController::EntryFinished(const char* path, status_t error)
{
	return error == B_OK;
}


void
BRemoveEngine::BController::ErrorOccurred(const char* message, status_t error)
{
}


} // namespace BPrivate
