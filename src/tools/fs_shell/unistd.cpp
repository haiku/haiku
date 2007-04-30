/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
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

#ifdef __BEOS__
#	include <Drivers.h>
#else
#	include <stropts.h>		// the correct place of definition for ioctl()

#	if defined(HAIKU_HOST_PLATFORM_LINUX)
#			include <linux/hdreg.h>
#	endif
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
fssh_close(int fd)
{
	return close(fd);
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

			#ifdef __BEOS__
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
				struct hd_geometry hdGeometry;
				// BLKGETSIZE and BLKGETSIZE64 don't seem to work for
				// partitions. So we get the device geometry (there only seems
				// to be HDIO_GETGEO, which is kind of obsolete, BTW), and
				// get the partition size via binary search.
				if (ioctl(fd, HDIO_GETGEO, &hdGeometry) == 0) {
					off_t bytesPerCylinder = (off_t)hdGeometry.heads
						* hdGeometry.sectors * 512;
					off_t deviceSize = bytesPerCylinder * hdGeometry.cylinders;
					off_t partitionSize = get_partition_size(fd, deviceSize);

					geometry->head_count = hdGeometry.heads;
					geometry->cylinder_count = partitionSize / bytesPerCylinder;
					geometry->sectors_per_track = hdGeometry.sectors;

					// TODO: Get the real values...
					geometry->bytes_per_sector = 512;
					geometry->device_type = FSSH_B_DISK;
					geometry->removable = false;
					geometry->read_only = false;
					geometry->write_once = false;
					error = B_OK;
				} else
					error = errno;

			#else
				// Not implemented for this platform, i.e. we won't be able to
				// deal with block devices.
			#endif

			break;
		}

		case FSSH_B_FLUSH_DRIVE_CACHE:
		{
			#ifdef __BEOS__
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
			#ifdef __BEOS__
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
fssh_read_pos(int fd, fssh_off_t pos, void *buffer, fssh_size_t count)
{
	return read_pos(fd, pos, buffer, count);
}


fssh_ssize_t
fssh_write_pos(int fd, fssh_off_t pos, const void *buffer, fssh_size_t count)
{
	return write_pos(fd, pos, buffer, count);
}


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
