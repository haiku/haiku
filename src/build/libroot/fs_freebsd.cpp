/*
 * Copyright 2008, Samuel Rodriguez Perez, samuelgaliza@gmail.com.
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "fs_freebsd.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/disk.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>


// Read and write operations in FreeBSD only work on devices block by block.

ssize_t
haiku_freebsd_read(int fd, void *buf, size_t nbytes)
{
	struct stat st;
	if (fstat(fd, &st) != 0)
		return -1;

	if (S_ISREG(st.st_mode))
		return read(fd, buf, nbytes); // Is a file! Good :)

	int sectorSize;
	if (ioctl(fd, DIOCGSECTORSIZE, &sectorSize) == -1)
		sectorSize = 512; // If fail, hardcode to 512 for now

	off_t cur = lseek(fd, 0, SEEK_CUR);
	if (cur == -1)
		perror("lseek 1");

	off_t seekDiff = (sectorSize - (cur % sectorSize)) % sectorSize;
	off_t nbytesDiff = (nbytes - seekDiff) % sectorSize;
	
	if (seekDiff == 0 && nbytesDiff == 0) {
		// Not needed but this saves malloc and free operations
		return read(fd, buf, nbytes);
		
	} else if (cur % sectorSize + nbytes <= sectorSize) {
		// Read complete in only a block
		char* tmpBlock = (char*)malloc(sectorSize);

		// Put at start of the block
		off_t sdCur = lseek(fd, -(cur % sectorSize), SEEK_CUR);
		if (sdCur == -1)
			perror("lseek oneblock");
		if (read(fd, tmpBlock, sectorSize) == -1)
			perror("read oneblock");
		memcpy((char*)buf, tmpBlock + cur % sectorSize, nbytes);
		// repos at byte offset of latest wrote block
		if (lseek(fd, -sectorSize + (cur % sectorSize) + nbytes, SEEK_CUR)
				== -1) {
			perror("lseek2 oneblock");
		}

		free(tmpBlock);
		
		return nbytes;

	} else {
		// Needs to write more than a block

		char* tmpBlock = (char*)malloc(sectorSize);
		
		// First block if seek isn't
		if (seekDiff > 0) {
			// read entire block at 0 pos
			if (lseek(fd, -(sectorSize - seekDiff), SEEK_CUR) == -1)
				perror("lseek seekDiff");
			off_t sdCur = lseek(fd,0,SEEK_CUR);
			if (sdCur == -1)
				perror("lseek2 seekDiff");
			if (read(fd, tmpBlock, sectorSize) == -1 )
				perror("read seekDiff");
			// alter content
			memcpy(buf, tmpBlock + (sectorSize - seekDiff), seekDiff);
			
		}

		// Blocks between
		if ((nbytes - seekDiff) >= sectorSize) {
			if (read(fd, ((char*)buf) + seekDiff, nbytes - seekDiff - nbytesDiff) == -1)
				perror("read between");
		}
		
		// Last block if overflow
		if (nbytesDiff > 0 ) {

			off_t sdCur = lseek(fd, 0, SEEK_CUR);
			if (sdCur == -1)
				perror("lseek last");
			if (read(fd, tmpBlock, sectorSize) == -1)
				perror("read last");
			memcpy(((char*)buf) + nbytes - nbytesDiff, tmpBlock, nbytesDiff);
			// repos at byte offset of latest wrote block
			if (lseek(fd, -(sectorSize - nbytesDiff), SEEK_CUR) == -1)
				perror("lseek2 last");
		}

		free(tmpBlock);
		
		return nbytes;
	}

}


ssize_t
haiku_freebsd_write(int fd, const void *buf, size_t nbytes)
{
	struct stat st;
	if (fstat(fd, &st) != 0)
		return -1;

	if (S_ISREG(st.st_mode))
		return write(fd, buf, nbytes); // Is a file! Good :)

	int sectorSize;
	if (ioctl(fd, DIOCGSECTORSIZE, &sectorSize) == -1)
		sectorSize = 512; // If fail, hardcode do 512 for now

	off_t cur = lseek(fd, 0, SEEK_CUR);
	if (cur == -1)
		perror("lseek 1");

	off_t seekDiff = (sectorSize - (cur % sectorSize)) % sectorSize;
	off_t nbytesDiff = (nbytes - seekDiff) % sectorSize;

	if (seekDiff == 0 && nbytesDiff == 0) {
		// Not needed but this saves malloc and free operations
		return write(fd, buf, nbytes);
		
	} else if (cur % sectorSize + nbytes <= sectorSize) {
		// Write complete in only a block
		char* tmpBlock = (char*)malloc(sectorSize);

		// Put at start of the block
		off_t sdCur = lseek(fd, -(cur % sectorSize), SEEK_CUR);
		if (sdCur == -1)
			perror("lseek oneblock");
		if (pread(fd, tmpBlock, sectorSize, sdCur) == -1)
			perror("pread oneblock");
		memcpy(tmpBlock + cur % sectorSize, (char*)buf, nbytes);
		if (write(fd, tmpBlock, sectorSize) == -1)
			perror("write oneblock");
		// repos at byte offset of latest written block
		if (lseek(fd, -sectorSize + (cur % sectorSize) + nbytes, SEEK_CUR) == -1)
			perror("lseek2 oneblock");

		free(tmpBlock);
		
		return nbytes;

	} else {
		// Needs to write more than a block

		char* tmpBlock = (char*)malloc(sectorSize);
		
		// First block if seek isn't
		if (seekDiff > 0) {
			// read entire block at 0 pos
			if (lseek(fd, -(sectorSize - seekDiff), SEEK_CUR) == -1)
				perror("lseek seekDiff");
			off_t sdCur = lseek(fd, 0, SEEK_CUR);
			if (sdCur == -1)
				perror("lseek2 seekDiff");
			if (pread(fd, tmpBlock, sectorSize, sdCur) == -1 )
				perror("pread seekDiff");
			// alter content
			memcpy(tmpBlock + (sectorSize - seekDiff), buf, seekDiff);
			if (write(fd, tmpBlock, sectorSize)==-1)
				perror("write seekDiff");
			
		}

		// Blocks between
		if ((nbytes - seekDiff) >= sectorSize) {
			if (write(fd, ((char*)buf) + seekDiff, nbytes - seekDiff - nbytesDiff) == -1)
				perror("write between");
		}
		
		// Last block if overflow
		if (nbytesDiff > 0) {

			off_t sdCur = lseek(fd, 0, SEEK_CUR);
			if (sdCur == -1)
				perror("lseek last");
			if (pread(fd, tmpBlock, sectorSize, sdCur) == -1)
				perror("pread last");
			memcpy(tmpBlock, ((char*)buf) + nbytes - nbytesDiff, nbytesDiff);
			if (write(fd, tmpBlock, sectorSize) == -1)
				perror("write last");
			// repos at byte offset of latest wrote block
			if (lseek(fd, -(sectorSize - nbytesDiff), SEEK_CUR) == -1)
				perror("lseek2 last");
		}

		free(tmpBlock);
		
		return nbytes;
	}

}


ssize_t
haiku_freebsd_readv(int fd, const iovec *vecs, size_t count)
{
	ssize_t bytesRead = 0;

	for (size_t i = 0; i < count; i++) {
		ssize_t currentRead = haiku_freebsd_read(fd, vecs[i].iov_base,
			vecs[i].iov_len);

		if (currentRead < 0)
			return bytesRead > 0 ? bytesRead : -1;

		bytesRead += currentRead;

		if ((size_t)currentRead != vecs[i].iov_len)
			break;
	}

	return bytesRead;
}


ssize_t
haiku_freebsd_writev(int fd, const struct iovec *vecs, size_t count)
{
	ssize_t bytesWritten = 0;

	for (size_t i = 0; i < count; i++) {
		ssize_t written = haiku_freebsd_write(fd, vecs[i].iov_base,
			vecs[i].iov_len);

		if (written < 0)
			return bytesWritten > 0 ? bytesWritten : -1;

		bytesWritten += written;

		if ((size_t)written != vecs[i].iov_len)
			break;
	}

	return bytesWritten;
}


#if defined(_HAIKU_BUILD_NO_FUTIMENS) || defined(_HAIKU_BUILD_NO_FUTIMENS)

template<typename File>
static int
utimes_helper(File& file, const struct timespec times[2])
{
	if (times == NULL)
		return file.SetTimes(NULL);

	timeval timeBuffer[2];
	timeBuffer[0].tv_sec = times[0].tv_sec;
	timeBuffer[0].tv_usec = times[0].tv_nsec / 1000;
	timeBuffer[1].tv_sec = times[1].tv_sec;
	timeBuffer[1].tv_usec = times[1].tv_nsec / 1000;

	if (times[0].tv_nsec == UTIME_OMIT || times[1].tv_nsec == UTIME_OMIT) {
		struct stat st;
		if (file.GetStat(st) != 0)
			return -1;

		if (times[0].tv_nsec == UTIME_OMIT && times[1].tv_nsec == UTIME_OMIT)
			return 0;

		if (times[0].tv_nsec == UTIME_OMIT) {
			timeBuffer[0].tv_sec = st.st_atimespec.tv_sec;
			timeBuffer[0].tv_usec = st.st_atimespec.tv_nsec / 1000;
		}

		if (times[1].tv_nsec == UTIME_OMIT) {
			timeBuffer[1].tv_sec = st.st_mtimespec.tv_sec;
			timeBuffer[1].tv_usec = st.st_mtimespec.tv_nsec / 1000;
		}
	}

	if (times[0].tv_nsec == UTIME_NOW || times[1].tv_nsec == UTIME_NOW) {
		timeval now;
		gettimeofday(&now, NULL);

		if (times[0].tv_nsec == UTIME_NOW)
			timeBuffer[0] = now;

		if (times[1].tv_nsec == UTIME_NOW)
			timeBuffer[1] = now;
	}

	return file.SetTimes(timeBuffer);	
}

#endif	// _HAIKU_BUILD_NO_FUTIMENS || _HAIKU_BUILD_NO_FUTIMENS


#ifdef _HAIKU_BUILD_NO_FUTIMENS

struct FDFile {
	FDFile(int fd)
		:
		fFD(fd)
	{
	}

	int GetStat(struct stat& _st)
	{
		return fstat(fFD, &_st);
	}

	int SetTimes(const timeval times[2])
	{
		return futimes(fFD, times);
	}

private:
	int fFD;
};


int
futimens(int fd, const struct timespec times[2])
{
	FDFile file(fd);
	return utimes_helper(file, times);
}

#endif	// _HAIKU_BUILD_NO_FUTIMENS


#ifdef _HAIKU_BUILD_NO_UTIMENSAT

struct FDPathFile {
	FDPathFile(int fd, const char* path, int flag)
		:
		fFD(fd),
		fPath(path),
		fFlag(flag)
	{
	}

	int GetStat(struct stat& _st)
	{
		return fstatat(fFD, fPath, &_st, fFlag);
	}

	int SetTimes(const timeval times[2])
	{
		// TODO: fFlag (AT_SYMLINK_NOFOLLOW) is not supported here!
		return futimesat(fFD, fPath, times);
	}

private:
	int			fFD;
	const char*	fPath;
	int			fFlag;
};


int
utimensat(int fd, const char* path, const struct timespec times[2], int flag)
{
	FDPathFile file(fd, path, flag);
	return utimes_helper(file, times);
}

#endif	// _HAIKU_BUILD_NO_UTIMENSAT
