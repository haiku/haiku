/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "FSUtils.h"

#include <string.h>

#include <algorithm>
#include <string>

#include <Directory.h>
#include <File.h>
#include <Path.h>
#include <SymLink.h>

#include <AutoDeleter.h>

#include "DebugSupport.h"


static const size_t kCompareDataBufferSize = 64 * 1024;
const char* const kShellEscapeCharacters = " ~`#$&*()\\|[]{};'\"<>?!";


/*static*/ BString
FSUtils::ShellEscapeString(const BString& string)
{
	BString result(string);
	result.CharacterEscape(kShellEscapeCharacters, '\\');
	if (result.IsEmpty())
		throw std::bad_alloc();
	return result;
}


/*static*/ status_t
FSUtils::OpenSubDirectory(const BDirectory& baseDirectory,
	const RelativePath& path, bool create, BDirectory& _directory)
{
	// get a string for the path
	BString pathString = path.ToString();
	if (pathString.IsEmpty())
		RETURN_ERROR(B_NO_MEMORY);

	// If creating is not allowed, just try to open it.
	if (!create)
		RETURN_ERROR(_directory.SetTo(&baseDirectory, pathString));

	// get an absolute path and create the subdirectory
	BPath absolutePath;
	status_t error = absolutePath.SetTo(&baseDirectory, pathString);
	if (error != B_OK) {
		ERROR("Volume::OpenSubDirectory(): failed to get absolute path "
			"for subdirectory \"%s\": %s\n", pathString.String(),
			strerror(error));
		RETURN_ERROR(error);
	}

	error = create_directory(absolutePath.Path(),
		S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
	if (error != B_OK) {
		ERROR("Volume::OpenSubDirectory(): failed to create "
			"subdirectory \"%s\": %s\n", pathString.String(),
			strerror(error));
		RETURN_ERROR(error);
	}

	RETURN_ERROR(_directory.SetTo(&baseDirectory, pathString));
}


/*static*/ status_t
FSUtils::CompareFileContent(const Entry& entry1, const Entry& entry2,
	bool& _equal)
{
	BFile file1;
	status_t error = _OpenFile(entry1, file1);
	if (error != B_OK)
		return error;

	BFile file2;
	error = _OpenFile(entry2, file2);
	if (error != B_OK)
		return error;

	return CompareFileContent(file1, file2, _equal);
}


/*static*/ status_t
FSUtils::CompareFileContent(BPositionIO& content1, BPositionIO& content2,
	bool& _equal)
{
	// get and compare content size
	off_t size1;
	status_t error = content1.GetSize(&size1);
	if (error != B_OK)
		return error;

	off_t size2;
	error = content2.GetSize(&size2);
	if (error != B_OK)
		return error;

	if (size1 != size2) {
		_equal = false;
		return B_OK;
	}

	if (size1 == 0) {
		_equal = true;
		return B_OK;
	}

	// allocate a data buffer
	uint8* buffer1 = new(std::nothrow) uint8[2 * kCompareDataBufferSize];
	if (buffer1 == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<uint8> bufferDeleter(buffer1);
	uint8* buffer2 = buffer1 + kCompareDataBufferSize;

	// compare the data
	off_t offset = 0;
	while (offset < size1) {
		size_t toCompare = std::min(size_t(size1 - offset),
			kCompareDataBufferSize);
		ssize_t bytesRead = content1.ReadAt(offset, buffer1, toCompare);
		if (bytesRead < 0)
			return bytesRead;
		if ((size_t)bytesRead != toCompare)
			return B_ERROR;

		bytesRead = content2.ReadAt(offset, buffer2, toCompare);
		if (bytesRead < 0)
			return bytesRead;
		if ((size_t)bytesRead != toCompare)
			return B_ERROR;

		if (memcmp(buffer1, buffer2, toCompare) != 0) {
			_equal = false;
			return B_OK;
		}

		offset += bytesRead;
	}

	_equal = true;
	return B_OK;
}


/*static*/ status_t
FSUtils::CompareSymLinks(const Entry& entry1, const Entry& entry2, bool& _equal)
{
	BSymLink symLink1;
	status_t error = _OpenSymLink(entry1, symLink1);
	if (error != B_OK)
		return error;

	BSymLink symLink2;
	error = _OpenSymLink(entry2, symLink2);
	if (error != B_OK)
		return error;

	return CompareSymLinks(symLink1, symLink2, _equal);
}


/*static*/ status_t
FSUtils::CompareSymLinks(BSymLink& symLink1, BSymLink& symLink2, bool& _equal)
{
	char buffer1[B_PATH_NAME_LENGTH];
	ssize_t bytesRead1 = symLink1.ReadLink(buffer1, sizeof(buffer1));
	if (bytesRead1 < 0)
		return bytesRead1;

	char buffer2[B_PATH_NAME_LENGTH];
	ssize_t bytesRead2 = symLink2.ReadLink(buffer2, sizeof(buffer2));
	if (bytesRead2 < 0)
		return bytesRead2;

	_equal = bytesRead1 == bytesRead2
		&& memcmp(buffer1, buffer2, bytesRead1) == 0;
	return B_OK;
}


/*static*/ status_t
FSUtils::ExtractPackageContent(const Entry& packageEntry,
	const char* contentPath, const Entry& targetDirectoryEntry)
{
	BPath packagePathBuffer;
	const char* packagePath;
	status_t error = packageEntry.GetPath(packagePathBuffer, packagePath);
	if (error != B_OK)
		return error;

	BPath targetPathBuffer;
	const char* targetPath;
	error = targetDirectoryEntry.GetPath(targetPathBuffer, targetPath);
	if (error != B_OK)
		return error;

	return ExtractPackageContent(packagePath, contentPath, targetPath);
}


/*static*/ status_t
FSUtils::ExtractPackageContent(const char* packagePath, const char* contentPath,
	const char* targetDirectoryPath)
{
	std::string commandLine = std::string("package extract -C ")
		+ ShellEscapeString(targetDirectoryPath).String()
		+ " "
		+ ShellEscapeString(packagePath).String()
		+ " "
		+ ShellEscapeString(contentPath).String();
	if (system(commandLine.c_str()) != 0)
		return B_ERROR;
	return B_OK;
}


/*static*/ status_t
FSUtils::_OpenFile(const Entry& entry, BFile& file)
{
	BPath pathBuffer;
	const char* path;
	status_t error = entry.GetPath(pathBuffer, path);
	if (error != B_OK)
		return error;

	return file.SetTo(path, B_READ_ONLY);
}


/*static*/ status_t
FSUtils::_OpenSymLink(const Entry& entry, BSymLink& symLink)
{
	BPath pathBuffer;
	const char* path;
	status_t error = entry.GetPath(pathBuffer, path);
	if (error != B_OK)
		return error;

	return symLink.SetTo(path);
}
