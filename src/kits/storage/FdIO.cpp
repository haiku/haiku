/*
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <FdIO.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>


BFdIO::BFdIO()
	:
	BPositionIO(),
	fFd(-1),
	fOwnsFd(false)
{
}


BFdIO::BFdIO(int fd, bool keepFd)
	:
	BPositionIO(),
	fFd(fd),
	fOwnsFd(keepFd)
{
}


BFdIO::~BFdIO()
{
	Unset();
}


void
BFdIO::SetTo(int fd, bool keepFd)
{
	Unset();

	fFd = fd;
	fOwnsFd = keepFd;
}


void
BFdIO::Unset()
{
	if (fOwnsFd && fFd >= 0)
		close(fFd);

	fFd = -1;
	fOwnsFd = false;
}


ssize_t
BFdIO::Read(void* buffer, size_t size)
{
	ssize_t bytesRead = read(fFd, buffer, size);
	return bytesRead >= 0 ? bytesRead : errno;
}


ssize_t
BFdIO::Write(const void* buffer, size_t size)
{
	ssize_t bytesWritten = write(fFd, buffer, size);
	return bytesWritten >= 0 ? bytesWritten : errno;
}


ssize_t
BFdIO::ReadAt(off_t position, void* buffer, size_t size)
{
	ssize_t bytesRead = pread(fFd, buffer, size, position);
	return bytesRead >= 0 ? bytesRead : errno;
}


ssize_t
BFdIO::WriteAt(off_t position, const void* buffer, size_t size)
{
	ssize_t bytesWritten = pwrite(fFd, buffer, size, position);
	return bytesWritten >= 0 ? bytesWritten : errno;
}


off_t
BFdIO::Seek(off_t position, uint32 seekMode)
{
	off_t newPosition = lseek(fFd, position, seekMode);
	return newPosition >= 0 ? newPosition : errno;
}


off_t
BFdIO::Position() const
{
	return const_cast<BFdIO*>(this)->BFdIO::Seek(0, SEEK_CUR);
}


status_t
BFdIO::SetSize(off_t size)
{
	return ftruncate(fFd, size) == 0 ? B_OK : errno;
}


status_t
BFdIO::GetSize(off_t* _size) const
{
	struct stat st;
	if (fstat(fFd, &st) != 0)
		return errno;

	*_size = st.st_size;
	return B_OK;
}
