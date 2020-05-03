/*
 * Copyright 2007-2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "compatibility.h"

#include "fssh_unistd.h"

#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

#include <SupportDefs.h>

#include "fssh_drivers.h"
#include "fssh_errno.h"
#include "partition_support.h"

#if (defined(__BEOS__) || defined(__HAIKU__))
#	include <Drivers.h>
#else
#	if defined(HAIKU_HOST_PLATFORM_FREEBSD) \
		|| defined(HAIKU_HOST_PLATFORM_DARWIN)
#		include <sys/ioctl.h>
#		include <sys/stat.h>
#		include <sys/disk.h>
#		ifndef HAIKU_HOST_PLATFORM_DARWIN
#			include <sys/disklabel.h>
#		endif
#	elif defined(HAIKU_HOST_PLATFORM_MSYS)
#		include <sys/ioctl.h>
#		include <sys/stat.h>
#	elif defined(HAIKU_HOST_PLATFORM_LINUX)
#		include <linux/hdreg.h>
#		include <linux/fs.h>
#		include <sys/ioctl.h>
#	else
		// the (POSIX) correct place of definition for ioctl()
#		include <stropts.h>
#	endif
#endif


#if (!defined(__BEOS__) && !defined(__HAIKU__))
	// Defined in libroot_build.so.
#	define _kern_dup	_kernbuild_dup
#	define _kern_close	_kernbuild_close
	extern "C" int _kern_dup(int fd);
	extern "C" status_t _kern_close(int fd);
#endif


#ifdef HAIKU_HOST_PLATFORM_LINUX

static bool
test_size(int fd, off_t size)
{
	char buffer[1];

	if (size == 0)
		return true;

	if (lseek(fd, size - 1, SEEK_SET) < 0)
		return false;

	return (read(fd, &buffer, 1) == 1);
}


static off_t
get_partition_size(int fd, off_t maxSize)
{
	// binary search
	off_t lower = 0;
	off_t upper = maxSize;
	while (lower < upper) {
		off_t mid = (lower + upper + 1) / 2;
		if (test_size(fd, mid))
			lower = mid;
		else
			upper = mid - 1;
	}

	return lower;
}

#endif // HAIKU_HOST_PLATFORM_LINUX


int
fssh_dup(int fd)
{
	// Use the _kern_dup() defined in libroot on BeOS incompatible systems.
	// Required for proper attribute emulation support.
	int newFD;
	#if (defined(__BEOS__) || defined(__HAIKU__))
		newFD = dup(fd);
	#else
		newFD = _kern_dup(fd);
		if (newFD < 0) {
			fssh_set_errno(newFD);
			newFD = -1;
		}
	#endif

	FSShell::restricted_file_duped(fd, newFD);

	return newFD;
}


int
fssh_close(int fd)
{
	FSShell::restricted_file_closed(fd);

	// Use the _kern_close() defined in libroot on BeOS incompatible systems.
	// Required for proper attribute emulation support.
	#if (defined(__BEOS__) || defined(__HAIKU__))
		return close(fd);
	#else
		return _kern_close(fd);
	#endif
}


int
fssh_unlink(const char *name)
{
	return unlink(name);
}


int
fssh_ioctl(int fd, unsigned long op, ...)
{
	status_t error = B_BAD_VALUE;
	va_list list;

	// count arguments

	va_start(list, op);

	switch (op) {
		case FSSH_B_GET_GEOMETRY:
		{
			fssh_device_geometry *geometry
				= va_arg(list, fssh_device_geometry*);

			#if (defined(__BEOS__) || defined(__HAIKU__))
				device_geometry systemGeometry;
				if (ioctl(fd, B_GET_GEOMETRY, &systemGeometry) == 0) {
					geometry->bytes_per_sector
						= systemGeometry.bytes_per_sector;
					geometry->sectors_per_track
						= systemGeometry.sectors_per_track;
					geometry->cylinder_count = systemGeometry.cylinder_count;
					geometry->head_count = systemGeometry.head_count;
					geometry->device_type = systemGeometry.device_type;
					geometry->removable = systemGeometry.removable;
					geometry->read_only = systemGeometry.read_only;
					geometry->write_once = systemGeometry.write_once;
					error = B_OK;
				} else
					error = errno;

			#elif defined(HAIKU_HOST_PLATFORM_LINUX)
				// If BLKGETSIZE64 don't work for us, we will fall back to
				// HDIO_GETGEO (which is kind of obsolete, BTW), and
				// get the partition size via binary search.
				struct hd_geometry hdGeometry;
				int blockSize = 512;
				off_t size;
				if (ioctl(fd, BLKGETSIZE64, &size) == 0 && size > 0) {
					off_t blocks = size / blockSize;
					uint32_t heads = (blocks + ULONG_MAX - 1)
						/ ULONG_MAX;
					if (heads == 0)
						heads = 1;

					geometry->head_count = heads;
					geometry->cylinder_count = blocks / heads;
					geometry->sectors_per_track = 1;
					error = B_OK;
				} else if (ioctl(fd, HDIO_GETGEO, &hdGeometry) == 0) {
					if (hdGeometry.heads == 0) {
						error = B_ERROR;
					} else {
						off_t bytesPerCylinder = (off_t)hdGeometry.heads
							* hdGeometry.sectors * 512;
						off_t deviceSize = bytesPerCylinder * hdGeometry.cylinders;
						off_t partitionSize = get_partition_size(fd, deviceSize);

						geometry->head_count = hdGeometry.heads;
						geometry->cylinder_count = partitionSize / bytesPerCylinder;
						geometry->sectors_per_track = hdGeometry.sectors;
						error = B_OK;
					}
				} else
					error = errno;

				if (error == B_OK) {
					// TODO: Get the real values...
					geometry->bytes_per_sector = blockSize;
					geometry->device_type = FSSH_B_DISK;
					geometry->removable = false;
					geometry->read_only = false;
					geometry->write_once = false;
				}

			#elif HAIKU_HOST_PLATFORM_FREEBSD
			{
				// FreeBSD has not block devices

				struct stat status;

				if (fstat(fd, &status) == 0) {
					// Do nothing for a regular file
					if (S_ISREG(status.st_mode))
						break;

					struct disklabel disklabel;
					off_t mediaSize;

					memset(&disklabel,0,sizeof disklabel);

					// Ignore errors, this way we can use memory devices (md%d)
					ioctl(fd, DIOCGSECTORSIZE, &disklabel.d_secsize);
					ioctl(fd, DIOCGFWSECTORS, &disklabel.d_nsectors);
					ioctl(fd, DIOCGFWHEADS, &disklabel.d_ntracks);
					ioctl(fd, DIOCGMEDIASIZE, &mediaSize);

					if (disklabel.d_nsectors == 0) {
						// Seems to be a md device, then ioctls returns lots of
						// zeroes and hardcode some defaults
						disklabel.d_nsectors = 64;
						disklabel.d_ntracks = 16;
					}

					disklabel.d_secperunit = mediaSize / disklabel.d_secsize;
					disklabel.d_ncylinders = mediaSize / disklabel.d_secsize
						/ disklabel.d_nsectors / disklabel.d_ntracks;

					geometry->head_count = disklabel.d_ntracks;
					geometry->cylinder_count = disklabel.d_ncylinders;
					geometry->sectors_per_track = disklabel.d_nsectors;

					geometry->bytes_per_sector = disklabel.d_secsize;
					// FreeBSD supports device_type flag as disklabel.d_type,
					// for now we harcod it to B_DISK.
					geometry->device_type = FSSH_B_DISK;
					geometry->removable = disklabel.d_flags & D_REMOVABLE > 0;
					// read_only?
					geometry->read_only = false;
					// FreeBSD does not support write_once flag.
					geometry->write_once = false;
					error = B_OK;
				} else
					error = errno;
			}
			#elif HAIKU_HOST_PLATFORM_DARWIN
			{
				// Darwin does not seems to provide a way to access disk
				// geometry directly

				struct stat status;

				if (fstat(fd, &status) == 0) {
					// Do nothing for a regular file
					if (S_ISREG(status.st_mode))
						break;

					off_t mediaSize;

					if (ioctl(fd, DKIOCGETBLOCKCOUNT, &mediaSize) != 0) {
						error = errno;
						break;
					}

					geometry->head_count = 4;
					geometry->sectors_per_track = 63;
					geometry->cylinder_count = mediaSize / geometry->head_count
						/ geometry->sectors_per_track;

					while (geometry->cylinder_count > 1024
						&& geometry->head_count < 256) {
						geometry->head_count *= 2;
						geometry->cylinder_count /= 2;
					}

					if (geometry->head_count == 256) {
						geometry->head_count = 255;
						geometry->cylinder_count = mediaSize
							/ geometry->head_count
							/ geometry->sectors_per_track;
					}

					if (ioctl(fd, DKIOCGETBLOCKSIZE,
							&geometry->bytes_per_sector) != 0) {
						error = errno;
						break;
					}

					uint32_t isWritable;
					if (ioctl(fd, DKIOCISWRITABLE, &isWritable) != 0) {
						error = errno;
						break;
					}

					geometry->read_only = !isWritable;

					// TODO: Get the real values...
					geometry->device_type = FSSH_B_DISK;
					geometry->removable = false;
					geometry->write_once = false;

					error = B_OK;
				} else
					error = errno;
			}
			#else
				// Not implemented for this platform, i.e. we won't be able to
				// deal with disk devices.
			#endif

			break;
		}

		case FSSH_B_FLUSH_DRIVE_CACHE:
		{
			#if (defined(__BEOS__) || defined(__HAIKU__))
				if (ioctl(fd, B_FLUSH_DRIVE_CACHE) == 0)
					error = B_OK;
				else
					error = errno;
			#else
				error = B_OK;
			#endif

			break;
		}

		case 10000:	// IOCTL_FILE_UNCACHED_IO
		{
			#if (defined(__BEOS__) || defined(__HAIKU__))
				if (ioctl(fd, 10000) == 0)
					error = B_OK;
				else
					error = errno;
			#else
				error = B_OK;
			#endif

			break;
		}
	}

	va_end(list);

	if (error != B_OK) {
		fssh_set_errno(error);
		return -1;
	}
	return 0;
}


fssh_ssize_t
fssh_read(int fd, void *buffer, fssh_size_t count)
{
	#if !defined(HAIKU_HOST_PLATFORM_FREEBSD)
		fssh_off_t pos = -1;
		if (FSShell::restricted_file_restrict_io(fd, pos, count) < 0)
			return -1;
		return read(fd, buffer, count);
	#else
		fssh_ssize_t bytesRead = read_pos(fd, fssh_lseek(fd, 0, FSSH_SEEK_CUR),
			buffer, count);
		if (bytesRead > 0)
			fssh_lseek(fd, bytesRead, FSSH_SEEK_CUR);
		return bytesRead;
	#endif
}


fssh_ssize_t
fssh_read_pos(int fd, fssh_off_t pos, void *buffer, fssh_size_t count)
{
	if (FSShell::restricted_file_restrict_io(fd, pos, count) < 0)
		return -1;
	return read_pos(fd, pos, buffer, count);
}


fssh_ssize_t
fssh_write(int fd, const void *buffer, fssh_size_t count)
{
	#if !defined(HAIKU_HOST_PLATFORM_FREEBSD)
		fssh_off_t pos = -1;
		if (FSShell::restricted_file_restrict_io(fd, pos, count) < 0)
			return -1;
		return write(fd, buffer, count);
	#else
		fssh_ssize_t written = write_pos(fd, fssh_lseek(fd, 0, FSSH_SEEK_CUR),
			buffer, count);
		if (written > 0)
			fssh_lseek(fd, written, FSSH_SEEK_CUR);
		return written;
	#endif
}


fssh_ssize_t
fssh_write_pos(int fd, fssh_off_t pos, const void *buffer, fssh_size_t count)
{
	if (FSShell::restricted_file_restrict_io(fd, pos, count) < 0)
		return -1;
	return write_pos(fd, pos, buffer, count);
}


// fssh_lseek() -- implemented in partition_support.cpp


fssh_gid_t
fssh_getegid(void)
{
	return 0;
}


fssh_uid_t
fssh_geteuid(void)
{
	return 0;
}


fssh_gid_t
fssh_getgid(void)
{
	return 0;
}


#if 0
int
fssh_getgroups(int groupSize, fssh_gid_t groupList[])
{
}
#endif	// 0


fssh_uid_t
fssh_getuid(void)
{
	return 0;
}
