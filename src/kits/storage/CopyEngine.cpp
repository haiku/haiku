/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <CopyEngine.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <fs_attr.h>
#include <Path.h>
#include <SymLink.h>
#include <TypeConstants.h>


namespace BPrivate {


static const size_t kDefaultBufferSize = 1024 * 1024;
static const size_t kSmallBufferSize = 64 * 1024;


// #pragma mark - BCopyEngine


BCopyEngine::BCopyEngine(uint32 flags)
	:
	fController(NULL),
	fFlags(flags),
	fBuffer(NULL),
	fBufferSize(0)
{
}


BCopyEngine::~BCopyEngine()
{
	delete[] fBuffer;
}


BCopyEngine::BController*
BCopyEngine::Controller() const
{
	return fController;
}


void
BCopyEngine::SetController(BController* controller)
{
	fController = controller;
}


uint32
BCopyEngine::Flags() const
{
	return fFlags;
}


BCopyEngine&
BCopyEngine::SetFlags(uint32 flags)
{
	fFlags = flags;
	return *this;
}


BCopyEngine&
BCopyEngine::AddFlags(uint32 flags)
{
	fFlags |= flags;
	return *this;
}


BCopyEngine&
BCopyEngine::RemoveFlags(uint32 flags)
{
	fFlags &= ~flags;
	return *this;
}


status_t
BCopyEngine::CopyEntry(const Entry& sourceEntry, const Entry& destEntry)
{
	if (fBuffer == NULL) {
		fBuffer = new(std::nothrow) char[kDefaultBufferSize];
		if (fBuffer == NULL) {
			fBuffer = new(std::nothrow) char[kSmallBufferSize];
			if (fBuffer == NULL) {
				_NotifyError(B_NO_MEMORY, "Failed to allocate buffer");
				return B_NO_MEMORY;
			}
			fBufferSize = kSmallBufferSize;
		} else
			fBufferSize = kDefaultBufferSize;
	}

	BPath sourcePathBuffer;
	const char* sourcePath;
	status_t error = sourceEntry.GetPath(sourcePathBuffer, sourcePath);
	if (error != B_OK)
		return error;

	BPath destPathBuffer;
	const char* destPath;
	error = destEntry.GetPath(destPathBuffer, destPath);
	if (error != B_OK)
		return error;

	return _CopyEntry(sourcePath, destPath);
}


status_t
BCopyEngine::_CopyEntry(const char* sourcePath, const char* destPath)
{
	// apply entry filter
	if (fController != NULL && !fController->EntryStarted(sourcePath))
		return B_OK;

	// stat source
	struct stat sourceStat;
	if (lstat(sourcePath, &sourceStat) < 0) {
		return _HandleEntryError(sourcePath, errno,
			"Couldn't access \"%s\": %s\n", sourcePath, strerror(errno));
	}

	// stat destination
	struct stat destStat;
	bool destExists = lstat(destPath, &destStat) == 0;

	// check whether to delete/create the destination
	bool unlinkDest = destExists;
	bool createDest = true;
	if (destExists) {
		if (S_ISDIR(destStat.st_mode)) {
			if (!S_ISDIR(sourceStat.st_mode)
				|| (fFlags & MERGE_EXISTING_DIRECTORIES) == 0) {
				return _HandleEntryError(sourcePath, B_FILE_EXISTS,
					"Can't copy \"%s\", since directory \"%s\" is in the "
					"way.\n", sourcePath, destPath);
			}

			if (S_ISDIR(sourceStat.st_mode)) {
				// both are dirs; nothing to do
				unlinkDest = false;
				destExists = false;
			}
		} else if ((fFlags & UNLINK_DESTINATION) == 0) {
			return _HandleEntryError(sourcePath, B_FILE_EXISTS,
				"Can't copy \"%s\", since entry \"%s\" is in the way.\n",
				sourcePath, destPath);
		}
	}

	// unlink the destination
	if (unlinkDest) {
		if (unlink(destPath) < 0) {
			return _HandleEntryError(sourcePath, errno,
				"Failed to unlink \"%s\": %s\n", destPath, strerror(errno));
		}
	}

	// open source node
	BNode _sourceNode;
	BFile sourceFile;
	BDirectory sourceDir;
	BNode* sourceNode = NULL;
	status_t error;

	if (S_ISDIR(sourceStat.st_mode)) {
		error = sourceDir.SetTo(sourcePath);
		sourceNode = &sourceDir;
	} else if (S_ISREG(sourceStat.st_mode)) {
		error = sourceFile.SetTo(sourcePath, B_READ_ONLY);
		sourceNode = &sourceFile;
	} else {
		error = _sourceNode.SetTo(sourcePath);
		sourceNode = &_sourceNode;
	}

	if (error != B_OK) {
		return _HandleEntryError(sourcePath, error,
			"Failed to open \"%s\": %s\n", sourcePath, strerror(error));
	}

	// create the destination
	BNode _destNode;
	BDirectory destDir;
	BFile destFile;
	BSymLink destSymLink;
	BNode* destNode = NULL;

	if (createDest) {
		if (S_ISDIR(sourceStat.st_mode)) {
			// create dir
			error = BDirectory().CreateDirectory(destPath, &destDir);
			if (error != B_OK) {
				return _HandleEntryError(sourcePath, error,
					"Failed to make directory \"%s\": %s\n", destPath,
					strerror(error));
			}

			destNode = &destDir;
		} else if (S_ISREG(sourceStat.st_mode)) {
			// create file
			error = BDirectory().CreateFile(destPath, &destFile);
			if (error != B_OK) {
				return _HandleEntryError(sourcePath, error,
					"Failed to create file \"%s\": %s\n", destPath,
					strerror(error));
			}

			destNode = &destFile;

			// copy file contents
			error = _CopyFileData(sourcePath, sourceFile, destPath, destFile);
			if (error != B_OK) {
				if (fController != NULL
					&& fController->EntryFinished(sourcePath, error)) {
					return B_OK;
				}
				return error;
			}
		} else if (S_ISLNK(sourceStat.st_mode)) {
			// read symlink
			char* linkTo = fBuffer;
			ssize_t bytesRead = readlink(sourcePath, linkTo, fBufferSize - 1);
			if (bytesRead < 0) {
				return _HandleEntryError(sourcePath, errno,
					"Failed to read symlink \"%s\": %s\n", sourcePath,
					strerror(errno));
			}

			// null terminate the link contents
			linkTo[bytesRead] = '\0';

			// create symlink
			error = BDirectory().CreateSymLink(destPath, linkTo, &destSymLink);
			if (error != B_OK) {
				return _HandleEntryError(sourcePath, error,
					"Failed to create symlink \"%s\": %s\n", destPath,
					strerror(error));
			}

			destNode = &destSymLink;

		} else {
			return _HandleEntryError(sourcePath, B_NOT_SUPPORTED,
				"Source file \"%s\" has unsupported type.\n", sourcePath);
		}

		// copy attributes (before setting the permissions!)
		error = _CopyAttributes(sourcePath, *sourceNode, destPath, *destNode);
		if (error != B_OK) {
			if (fController != NULL
				&& fController->EntryFinished(sourcePath, error)) {
				return B_OK;
			}
			return error;
		}

		// set file owner, group, permissions, times
		destNode->SetOwner(sourceStat.st_uid);
		destNode->SetGroup(sourceStat.st_gid);
		destNode->SetPermissions(sourceStat.st_mode);
		#ifdef HAIKU_TARGET_PLATFORM_HAIKU
			destNode->SetCreationTime(sourceStat.st_crtime);
		#endif
		destNode->SetModificationTime(sourceStat.st_mtime);
	}

	// the destination node is no longer needed
	destNode->Unset();

	// recurse
	if ((fFlags & COPY_RECURSIVELY) != 0 && S_ISDIR(sourceStat.st_mode)) {
		char buffer[sizeof(dirent) + B_FILE_NAME_LENGTH];
		dirent *entry = (dirent*)buffer;
		while (sourceDir.GetNextDirents(entry, sizeof(buffer), 1) == 1) {
			if (strcmp(entry->d_name, ".") == 0
				|| strcmp(entry->d_name, "..") == 0) {
				continue;
			}

			// construct new entry paths
			BPath sourceEntryPath;
			error = sourceEntryPath.SetTo(sourcePath, entry->d_name);
			if (error != B_OK) {
				return _HandleEntryError(sourcePath, error,
					"Failed to construct entry path from dir \"%s\" and name "
					"\"%s\": %s\n", sourcePath, entry->d_name, strerror(error));
			}

			BPath destEntryPath;
			error = destEntryPath.SetTo(destPath, entry->d_name);
			if (error != B_OK) {
				return _HandleEntryError(sourcePath, error,
					"Failed to construct entry path from dir \"%s\" and name "
					"\"%s\": %s\n", destPath, entry->d_name, strerror(error));
			}

			// copy the entry
			error = _CopyEntry(sourceEntryPath.Path(), destEntryPath.Path());
			if (error != B_OK) {
				if (fController != NULL
					&& fController->EntryFinished(sourcePath, error)) {
					return B_OK;
				}
				return error;
			}
		}
	}

	if (fController != NULL)
		fController->EntryFinished(sourcePath, B_OK);
	return B_OK;
}


status_t
BCopyEngine::_CopyFileData(const char* sourcePath, BFile& source,
	const char* destPath, BFile& destination)
{
	off_t offset = 0;
	while (true) {
		// read
		ssize_t bytesRead = source.ReadAt(offset, fBuffer, fBufferSize);
		if (bytesRead < 0) {
			_NotifyError(bytesRead, "Failed to read from file \"%s\": %s\n",
				sourcePath, strerror(bytesRead));
			return bytesRead;
		}

		if (bytesRead == 0)
			return B_OK;

		// write
		ssize_t bytesWritten = destination.WriteAt(offset, fBuffer, bytesRead);
		if (bytesWritten < 0) {
			_NotifyError(bytesWritten, "Failed to write to file \"%s\": %s\n",
				destPath, strerror(bytesWritten));
			return bytesWritten;
		}

		if (bytesWritten != bytesRead) {
			_NotifyError(B_ERROR, "Failed to write all data to file \"%s\"\n",
				destPath);
			return B_ERROR;
		}

		offset += bytesRead;
	}
}


status_t
BCopyEngine::_CopyAttributes(const char* sourcePath, BNode& source,
	const char* destPath, BNode& destination)
{
	char attrName[B_ATTR_NAME_LENGTH];
	while (source.GetNextAttrName(attrName) == B_OK) {
		// get attr info
		attr_info attrInfo;
		status_t error = source.GetAttrInfo(attrName, &attrInfo);
		if (error != B_OK) {
			// Delay reporting/handling the error until the controller has been
			// asked whether it is interested.
			attrInfo.type = B_ANY_TYPE;
		}

		// filter
		if (fController != NULL
			&& !fController->AttributeStarted(sourcePath, attrName,
				attrInfo.type)) {
			if (error != B_OK) {
				_NotifyError(error, "Failed to get info of attribute \"%s\" "
					"of file \"%s\": %s\n", attrName, sourcePath,
					strerror(error));
			}
			continue;
		}

		if (error != B_OK) {
			error = _HandleAttributeError(sourcePath, attrName, attrInfo.type,
				error, "Failed to get info of attribute \"%s\" of file \"%s\": "
				"%s\n", attrName, sourcePath, strerror(error));
			if (error != B_OK)
				return error;
			continue;
		}

		// copy the attribute
		off_t offset = 0;
		off_t bytesLeft = attrInfo.size;
		// go at least once through the loop, so that an empty attribute will be
		// created as well
		do {
			size_t toRead = fBufferSize;
			if ((off_t)toRead > bytesLeft)
				toRead = bytesLeft;

			// read
			ssize_t bytesRead = source.ReadAttr(attrName, attrInfo.type,
				offset, fBuffer, toRead);
			if (bytesRead < 0) {
				error = _HandleAttributeError(sourcePath, attrName,
					attrInfo.type, bytesRead, "Failed to read attribute \"%s\" "
					"of file \"%s\": %s\n", attrName, sourcePath,
					strerror(bytesRead));
				if (error != B_OK)
					return error;
				break;
			}

			if (bytesRead == 0 && offset > 0)
				break;

			// write
			ssize_t bytesWritten = destination.WriteAttr(attrName,
				attrInfo.type, offset, fBuffer, bytesRead);
			if (bytesWritten < 0) {
				error = _HandleAttributeError(sourcePath, attrName,
					attrInfo.type, bytesWritten, "Failed to write attribute "
					"\"%s\" of file \"%s\": %s\n", attrName, destPath,
					strerror(bytesWritten));
				if (error != B_OK)
					return error;
				break;
			}

			bytesLeft -= bytesRead;
			offset += bytesRead;
		} while (bytesLeft > 0);

		if (fController != NULL) {
			fController->AttributeFinished(sourcePath, attrName, attrInfo.type,
				B_OK);
		}
	}

	return B_OK;
}


void
BCopyEngine::_NotifyError(status_t error, const char* format, ...)
{
	if (fController != NULL) {
		va_list args;
		va_start(args, format);
		_NotifyErrorVarArgs(error, format, args);
		va_end(args);
	}
}


void
BCopyEngine::_NotifyErrorVarArgs(status_t error, const char* format,
	va_list args)
{
	if (fController != NULL) {
		BString message;
		message.SetToFormatVarArgs(format, args);
		fController->ErrorOccurred(message, error);
	}
}


status_t
BCopyEngine::_HandleEntryError(const char* path, status_t error,
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


status_t
BCopyEngine::_HandleAttributeError(const char* path, const char* attribute,
	uint32 attributeType, status_t error, const char* format, ...)
{
	if (fController == NULL)
		return error;

	va_list args;
	va_start(args, format);
	_NotifyErrorVarArgs(error, format, args);
	va_end(args);

	if (fController->AttributeFinished(path, attribute, attributeType, error))
		return B_OK;
	return error;
}


// #pragma mark - BController


BCopyEngine::BController::BController()
{
}


BCopyEngine::BController::~BController()
{
}


bool
BCopyEngine::BController::EntryStarted(const char* path)
{
	return true;
}


bool
BCopyEngine::BController::EntryFinished(const char* path, status_t error)
{
	return error == B_OK;
}


bool
BCopyEngine::BController::AttributeStarted(const char* path,
	const char* attribute, uint32 attributeType)
{
	return true;
}


bool
BCopyEngine::BController::AttributeFinished(const char* path,
	const char* attribute, uint32 attributeType, status_t error)
{
	return error == B_OK;
}


void
BCopyEngine::BController::ErrorOccurred(const char* message, status_t error)
{
}


} // namespace BPrivate
