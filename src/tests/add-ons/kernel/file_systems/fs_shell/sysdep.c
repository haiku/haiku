/*
  This file contains some routines that are #ifdef'ed based on what
  system you're on.  Currently it supports the BeOS and Unix. It
  could be extended to support Windows NT but their posix support
  is such a joke that it would probably be a real pain in the arse.
  
  THIS CODE COPYRIGHT DOMINIC GIAMPAOLO.  NO WARRANTY IS EXPRESSED 
  OR IMPLIED.  YOU MAY USE THIS CODE AND FREELY DISTRIBUTE IT FOR
  NON-COMMERCIAL USE AS LONG AS THIS NOTICE REMAINS ATTACHED.

  FOR COMMERCIAL USE, CONTACT DOMINIC GIAMPAOLO (dbg@be.com).

  Dominic Giampaolo
  dbg@be.com
*/
#include "compat.h"

#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "stat_util.h"

// Linux support
#ifdef HAIKU_HOST_PLATFORM_LINUX
#	include <linux/hdreg.h>
#endif	// HAIKU_HOST_PLATFORM_LINUX


// Let the FS think, we are the root user.
uid_t
geteuid()
{
	return 0;
}


gid_t
getegid()
{
	return 0;
}


int
device_is_read_only(const char *device)
{
#ifdef unix
    return 0;   /* XXXdbg should do an ioctl or something */
#else
    int             fd;
    device_geometry dg;

    fd = open(device, O_RDONLY);
    if (ioctl(fd, B_GET_GEOMETRY, &dg) < 0)
        return 0;

    close(fd);

    return dg.read_only;
#endif
}

int
get_device_block_size(int fd)
{
#ifdef unix
    return 512;   /* XXXdbg should do an ioctl or something */
#else
    struct stat     st;
    device_geometry dg;

    if (ioctl(fd, B_GET_GEOMETRY, &dg) < 0) {
        if (fstat(fd, &st) < 0 || S_ISDIR(st.st_mode))
            return 0;

        return 512;   /* just assume it's a plain old file or something */
    }

    return dg.bytes_per_sector;
#endif
}

fs_off_t
get_num_device_blocks(int fd)
{
#ifdef unix
    struct stat st;
    
    fstat(fd, &st);    /* XXXdbg should be an ioctl or something */

    return st.st_size / get_device_block_size(fd);
#else
    struct stat st;
    device_geometry dg;

    if (ioctl(fd, B_GET_GEOMETRY, &dg) >= 0) {
        return (fs_off_t)dg.cylinder_count *
               (fs_off_t)dg.sectors_per_track *
               (fs_off_t)dg.head_count;
    }

    /* if the ioctl fails, try just stat'ing in case it's a regular file */
    if (fstat(fd, &st) < 0)
        return 0;

    return st.st_size / get_device_block_size(fd);
#endif
}

int
device_is_removeable(int fd)
{
#ifdef unix
    return 0;   /* XXXdbg should do an ioctl or something */
#else
    device_geometry dg;

    if (ioctl(fd, B_GET_GEOMETRY, &dg) < 0) {
        return 0;
    }

    return dg.removable;
#endif
}

#if (defined(__BEOS__) || defined(__HAIKU__)) && !defined(USER)
#include "scsi.h"
#endif

int
lock_removeable_device(int fd, bool on_or_off)
{
#if defined(unix) || defined(USER)
    return 0;   /* XXXdbg should do an ioctl or something */
#else
    return ioctl(fd, B_SCSI_PREVENT_ALLOW, &on_or_off);
#endif
}




#if (!defined(__BEOS__) && !defined(__HAIKU__))
ssize_t
read_pos(int fd, fs_off_t _pos, void *data,  size_t nbytes)
{
    off_t  pos = (off_t)_pos;
    size_t ret;
    
    if (lseek(fd, pos, SEEK_SET) < 0) {
        perror("read lseek");
        errno = EINVAL;
		return -1;
    }
    
    ret = read(fd, data, nbytes);

    if (ret != nbytes) {
        printf("read_pos: wanted %d, got %d\n", nbytes, ret);
        return -1;
    }

    return ret;
}

ssize_t
write_pos(int fd, fs_off_t _pos, const void *data,  size_t nbytes)
{
    off_t  pos = (off_t)_pos;
    size_t ret;
    
    if (lseek(fd, pos, SEEK_SET) < 0) {
        perror("read lseek");
        errno = EINVAL;
		return -1;
    }
    
    ret = write(fd, data, nbytes);

    if (ret != nbytes) {
        printf("write_pos: wanted %d, got %d\n", nbytes, ret);
        return -1;
    }

    return ret;
}


#ifdef sun                      /* bloody wankers */
#include <sys/stream.h>
#ifdef DEF_IOV_MAX
#define MAX_IOV  DEF_IOV_MAX
#else
#define MAX_IOV  16
#endif
#else                         /* the rest of the world... */
#define MAX_IOV  8192         /* something way bigger than we'll ever use */
#endif

ssize_t
readv_pos(int fd, fs_off_t _pos, const struct iovec *iov, int count)
{
    off_t  pos = (off_t)_pos;
    size_t amt = 0;
    ssize_t ret;
    const struct iovec *tmpiov;
    int i, n;

    if (lseek(fd, pos, SEEK_SET) < 0) {
        perror("read lseek");
        return FS_EINVAL;
    }

    i = 0;
    tmpiov = iov;
    while (i < count) {
        if (i + MAX_IOV < count)
            n = MAX_IOV;
        else
            n = (count - i);

        ret = readv(fd, tmpiov, n);
        amt += ret;

        if (ret < 0)
            break;

        i += n;
        tmpiov += n;
    }

    return amt;
}

ssize_t
writev_pos(int fd, fs_off_t _pos, const struct iovec *iov,  int count)
{
    off_t  pos = (off_t)_pos;
    size_t amt = 0;
    ssize_t ret;
    const struct iovec *tmpiov;
    int i, n;

    if (lseek(fd, pos, SEEK_SET) < 0) {
        perror("read lseek");
        return FS_EINVAL;
    }

    i = 0;
    tmpiov = iov;
    while (i < count) {
        if (i + MAX_IOV < count)
            n = MAX_IOV;
        else
            n = (count - i);

        ret = writev(fd, tmpiov, n);
        amt += ret;

        if (ret < 0)
            break;

        i += n;
        tmpiov += n;
    }

    return amt;
}


int build_platform_open(const char *pathname, int oflags, my_mode_t mode);

int
build_platform_open(const char *pathname, int oflags, my_mode_t mode)
{
	int fd = open(pathname, to_platform_open_mode(oflags),
		to_platform_mode(mode));
	if (fd < 0) {
		errno = from_platform_error(errno);
		return -1;
	}

	return fd;
}


int build_platform_close(int fd);

int
build_platform_close(int fd)
{
	if (close(fd) < 0) {
		errno = from_platform_error(errno);
		return -1;
	}

	return 0;
}


int build_platform_fstat(int fd, struct my_stat *myst);

int
build_platform_fstat(int fd, struct my_stat *myst)
{
	struct stat st;
	
	if (!myst) {
		errno = FS_EINVAL;
		return -1;
	}
	
	if (fstat(fd, &st) < 0) {
		errno = from_platform_error(errno);
		return -1;
	}

	from_platform_stat(&st, myst);

	return 0;
}


ssize_t build_platform_read_pos(int fd, fs_off_t pos, void *buf, size_t count);

ssize_t
build_platform_read_pos(int fd, fs_off_t pos, void *buf, size_t count)
{
	ssize_t result = read_pos(fd, pos, buf, count);
	if (result < 0) {
		errno = from_platform_error(errno);
		return -1;
	}

	return result;
}


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


extern int build_platform_ioctl(int fd, unsigned long op, ...);

int
build_platform_ioctl(int fd, unsigned long op, ...)
{
	status_t error = FS_BAD_VALUE;
	va_list list;

	// count arguments

	va_start(list, op);

	switch (op) {
		case 7:		// B_GET_GEOMETRY
		{
			#ifdef HAIKU_HOST_PLATFORM_LINUX
			{
				device_geometry *geometry = va_arg(list, device_geometry*);
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
					geometry->device_type = B_DISK;
					geometry->removable = false;
					geometry->read_only = false;
					geometry->write_once = false;
					error = FS_OK;
				} else
					error = from_platform_error(errno);
			}
			#endif // HAIKU_HOST_PLATFORM_LINUX

			break;
		}

		case 20:	// B_FLUSH_DRIVE_CACHE
			error = FS_OK;
			break;

		case 10000:	// IOCTL_FILE_UNCACHED_IO
			error = FS_OK;
			break;
	}	

	va_end(list);

	if (error != FS_OK) {
		errno = error;
		return -1;
	}
	return 0;
}


#endif /* ! __BEOS__ */


#include <stdarg.h>


void
panic(const char *format, ...)
{
    va_list     ap;
    
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    while (TRUE)
        ;
}



#include "lock.h"

int
new_lock(lock *l, const char *name)
{
    l->c = 1;
    l->s = create_sem(0, (char *)name);
    if (l->s <= 0)
        return l->s;
    return 0;
}

int
free_lock(lock *l)
{
    delete_sem(l->s);

    return 0;
}

int
new_mlock(mlock *l, long c, const char *name)
{
    l->s = create_sem(c, (char *)name);
    if (l->s <= 0)
        return l->s;
    return 0;
}

int
free_mlock(mlock *l)
{
    delete_sem(l->s);

    return 0;
}


#ifdef unix
#include <sys/time.h>

bigtime_t
system_time(void)
{
    bigtime_t      t;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    
    t = ((bigtime_t)tv.tv_sec * 1000000) + (bigtime_t)tv.tv_usec;
    return t;
}

/*
  If you're compiler/system can't deal with the version of system_time()
  as defined above, use this one instead
bigtime_t
system_time(void)
{
    return (bigtime_t)time(NULL);
}
*/  

#endif  /* unix */

#if (defined(__BEOS__) || defined(__HAIKU__))
#include <KernelExport.h>

void
dprintf(const char *format, ...)
{
	va_list args;
	
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

#endif
