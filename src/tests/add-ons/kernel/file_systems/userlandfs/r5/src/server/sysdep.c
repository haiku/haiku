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
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "compat.h"


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
        return EINVAL;
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
        return EINVAL;
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
readv_pos(int fd, fs_off_t _pos, struct iovec *iov, int count)
{
    off_t  pos = (off_t)_pos;
    size_t amt = 0;
    ssize_t ret;
    struct iovec *tmpiov;
    int i, n;

    if (lseek(fd, pos, SEEK_SET) < 0) {
        perror("read lseek");
        return EINVAL;
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
writev_pos(int fd, fs_off_t _pos, struct iovec *iov,  int count)
{
    off_t  pos = (off_t)_pos;
    size_t amt = 0;
    ssize_t ret;
    struct iovec *tmpiov;
    int i, n;

    if (lseek(fd, pos, SEEK_SET) < 0) {
        perror("read lseek");
        return EINVAL;
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



#endif /* __BEOS__ */


#include <stdarg.h>


#if 0	// bonefish
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
#endif


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
