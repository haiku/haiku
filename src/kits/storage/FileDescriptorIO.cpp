/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <FileDescriptorIO.h>

#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>


BFileDescriptorIO::BFileDescriptorIO(int fd, bool takeOverOwnership)
	:
	fFD(fd),
	fOwnsFD(takeOverOwnership)
{
}


BFileDescriptorIO::~BFileDescriptorIO()
{
	if (fOwnsFD)
		close(fFD);
}


ssize_t
BFileDescriptorIO::Read(void* buffer, size_t size)
{
	ssize_t bytesRead = read(fFD, buffer, size);
	return bytesRead >= 0 ? bytesRead : errno;
}


ssize_t
BFileDescriptorIO::Write(const void* buffer, size_t size)
{
	ssize_t bytesWritten = write(fFD, buffer, size);
	return bytesWritten >= 0 ? bytesWritten : errno;
}


ssize_t
BFileDescriptorIO::ReadAt(off_t position, void* buffer, size_t size)
{
	ssize_t bytesRead = pread(fFD, buffer, size, position);
	return bytesRead >= 0 ? bytesRead : errno;
}


ssize_t
BFileDescriptorIO::WriteAt(off_t position, const void* buffer, size_t size)
{
	ssize_t bytesWritten = pwrite(fFD, buffer, size, position);
	return bytesWritten >= 0 ? bytesWritten : errno;
}


off_t
BFileDescriptorIO::Seek(off_t position, uint32 seekMode)
{
	off_t result = lseek(fFD, position, seekMode);
	return result >= 0 ? result : errno;
}


off_t
BFileDescriptorIO::Position() const
{
	off_t result = lseek(fFD, 0, SEEK_CUR);
	return result >= 0 ? result : errno;
}


status_t
BFileDescriptorIO::SetSize(off_t size)
{
	return ftruncate(fFD, size) == 0 ? B_OK : errno;
}


status_t
BFileDescriptorIO::GetSize(off_t* size) const
{
	struct stat st;
	if (fstat(fFD, &st) < 0)
		return errno;

	*size = st.st_size;
	return B_OK;
}
