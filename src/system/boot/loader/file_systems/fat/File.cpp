/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "File.h"

#include <sys/stat.h>
#include <unistd.h>

#include <util/kernel_cpp.h>

#include "Directory.h"


//#define TRACE(x) dprintf x
#define TRACE(x) do {} while (0)


namespace FATFS {


File::File(Volume &volume, off_t dirEntryOffset, uint32 cluster, off_t size,
	const char *name)
	:
	fVolume(volume),
	fStream(volume, cluster, size, name),
	fDirEntryOffset(dirEntryOffset)
{
	TRACE(("FATFS::File::()\n"));
}


File::~File()
{
	TRACE(("FATFS::File::~()\n"));
}


status_t
File::InitCheck()
{
	if (fStream.InitCheck() != B_OK)
		return fStream.InitCheck();

	return B_OK;
}


status_t
File::Open(void **_cookie, int mode)
{
	TRACE(("FATFS::File::%s(, %d)\n", __FUNCTION__, mode));
	if (fStream.InitCheck() < B_OK)
		return fStream.InitCheck();

	return Node::Open(_cookie, mode);
}


status_t
File::Close(void *cookie)
{
	return Node::Close(cookie);
}


ssize_t
File::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	TRACE(("FATFS::File::%s(, %Ld,, %d)\n", __FUNCTION__, pos, bufferSize));
	status_t err;
	err = fStream.ReadAt(pos, buffer, &bufferSize);
	if (err < B_OK)
		return err;
	return bufferSize;
}


ssize_t
File::WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize)
{
	off_t streamSize = fStream.Size();
	uint32 firstCluster = fStream.FirstCluster();

	// write data
	size_t written = bufferSize;
	status_t error = fStream.WriteAt(pos, buffer, &written);
	if (error != B_OK)
		return error;

	// If the file size has changed, we need to adjust the directory entry.
	if (fStream.Size() > streamSize || fStream.FirstCluster() != firstCluster) {
		error = Directory::UpdateDirEntry(fVolume, fDirEntryOffset,
			fStream.FirstCluster(), fStream.Size());
		if (error != B_OK)
			return error;
			// TODO: Undo the changes!
	}

	return written;
}


status_t
File::GetName(char *nameBuffer, size_t bufferSize) const
{
	return fStream.GetName(nameBuffer, bufferSize);
}


status_t
File::GetFileMap(struct file_map_run *runs, int32 *count)
{
	return fStream.GetFileMap(runs, count);
}


int32
File::Type() const
{
	return S_IFREG;
}


off_t
File::Size() const
{
	return fStream.Size();
}


ino_t
File::Inode() const
{
	return fStream.FirstCluster() << 16;
}

}	// namespace FATFS
