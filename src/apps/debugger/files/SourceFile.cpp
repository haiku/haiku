/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "SourceFile.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <new>


static const int32 kMaxSourceFileSize = 10 * 1024 * 1024;


// #pragma mark - SourceFileOwner


SourceFileOwner::~SourceFileOwner()
{
}


// #pragma mark - SourceFile


SourceFile::SourceFile(SourceFileOwner* owner)
	:
	fOwner(owner),
	fFileContent(NULL),
	fLineOffsets(NULL),
	fLineCount(0)
{
}


SourceFile::~SourceFile()
{
	free(fFileContent);
	delete[] fLineOffsets;
	fOwner->SourceFileDeleted(this);
}


status_t
SourceFile::Init(const char* path)
{
	// open the file
	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return errno;

	// stat the file to get its size
	struct stat st;
	if (fstat(fd, &st) < 0) {
		close(fd);
		return errno;
	}

	if (st.st_size > kMaxSourceFileSize) {
		close(fd);
		return B_FILE_TOO_LARGE;
	}
	size_t fileSize = st.st_size;

	if (fileSize == 0) {
		close(fd);
		return B_BAD_VALUE;
	}

	// allocate the content buffer
	fFileContent = (char*)malloc(fileSize + 1);
		// one more byte for a terminating null
	if (fFileContent == NULL) {
		close(fd);
		return B_NO_MEMORY;
	}

	// read the file
	ssize_t bytesRead = read(fd, fFileContent, fileSize);
	close(fd);
	if (bytesRead < 0 || (size_t)bytesRead != fileSize)
		return bytesRead < 0 ? errno : B_FILE_ERROR;

	// null-terminate
	fFileContent[fileSize] = '\0';

	// count lines
	fLineCount = 1;
	for (size_t i = 0; i < fileSize; i++) {
		if (fFileContent[i] == '\n')
			fLineCount++;
	}

	// allocate line offset array
	fLineOffsets = new(std::nothrow) int32[fLineCount + 1];
	if (fLineOffsets == NULL)
		return B_NO_MEMORY;

	// get the line offsets and null-terminate the lines
	int32 lineIndex = 0;
	fLineOffsets[lineIndex++] = 0;
	for (size_t i = 0; i < fileSize; i++) {
		if (fFileContent[i] == '\n') {
			fFileContent[i] = '\0';
			fLineOffsets[lineIndex++] = i + 1;
		}
	}
	fLineOffsets[fLineCount] = fileSize + 1;

	return B_OK;
}


int32
SourceFile::CountLines() const
{
	return fLineCount;
}


const char*
SourceFile::LineAt(int32 index) const
{
	return index >= 0 && index < fLineCount
		? fFileContent + fLineOffsets[index] : NULL;
}


int32
SourceFile::LineLengthAt(int32 index) const
{
	return index >= 0 && index < fLineCount
		? fLineOffsets[index + 1] - fLineOffsets[index] - 1: 0;
}

void
SourceFile::LastReferenceReleased()
{
	fOwner->SourceFileUnused(this);
	delete this;
}
