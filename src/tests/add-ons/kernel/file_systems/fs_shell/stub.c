/*
  This file contains some stub routines to cover up the differences
  between the BeOS and the rest of the world.
  
  
  THIS CODE COPYRIGHT DOMINIC GIAMPAOLO.  NO WARRANTY IS EXPRESSED 
  OR IMPLIED.  YOU MAY USE THIS CODE AND FREELY DISTRIBUTE IT FOR
  NON-COMMERCIAL USE AS LONG AS THIS NOTICE REMAINS ATTACHED.

  FOR COMMERCIAL USE, CONTACT DOMINIC GIAMPAOLO (dbg@be.com).

  Dominic Giampaolo
  dbg@be.com
*/
#include "compat.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <fs_attr.h>
#include <KernelExport.h>
#include <OS.h>

#include "myfs.h"


#ifndef __BEOS__ 

static void
myfs_die(const char *format,...)
{
	va_list list;
	va_start(list, format);
	vfprintf(stderr, format, list);
	va_end(list);
	
	exit(1);
}


//void
//unload_kernel_addon(aid)
//{
//}

sem_id
create_sem(long count, const char *name)
{
    int *ptr;
    ptr = (int *)malloc(sizeof(int) + strlen(name) + 1);  /* a hack */
    *ptr = count;
    memcpy(ptr+1, name, strlen(name));
    
    return (sem_id)ptr;
}


long
delete_sem(sem_id semid)
{
    int *ptr = (int *)semid;

    free(ptr);
    return 0;
}

long
acquire_sem(sem_id sem)
{
    int *ptr = (int *)sem;

    if (*ptr <= 0) {
        myfs_die("You lose sucka! acquire of sem with count == %d\n", *ptr);
    }

    *ptr -= 1;

    return 0;
}


long
acquire_sem_etc(sem_id sem, int32 count, uint32 j1, bigtime_t j2)
{
    int *ptr = (int *)sem;

    if (*ptr <= 0) {
        myfs_die("You lose sucka! acquire_sem_etc of sem with count == %d\n",
                 *ptr);
    }

    *ptr -= count;

    return 0;
}

long
release_sem(sem_id sem)
{
    int *ptr = (int *)sem;

    *ptr += 1;

    return 0;
}

long
release_sem_etc(sem_id sem, long count, uint32 j1)
{
    int *ptr = (int *)sem;

    *ptr += count;

    return 0;
}


long
atomic_add(vint32 *ptr, long val)
{
    int old = *ptr;

    *ptr += val;

    return old;
}

status_t
snooze(bigtime_t f)
{
    sleep(1);
    return 1;
}


// #pragma mark -

void
debugger(const char *message)
{
	myfs_die("debugger() call: `%s'\n", message);
}


// #pragma mark -

thread_id
find_thread(const char *name)
{
	if (!name)
		return 42;
	
	return FS_EINVAL;
}


thread_id
spawn_thread(thread_func func, const char *name, int32 priority, void *data)
{
	fprintf(stderr, "spawn_thread() not supported on this platform.\n");
	return FS_EINVAL;
}


status_t
resume_thread(thread_id thread)
{
	fprintf(stderr, "resume_thread() not supported on this platform.\n");
	return FS_EINVAL;
}


status_t
wait_for_thread(thread_id thread, status_t *returnValue)
{
	fprintf(stderr, "wait_for_thread() not supported on this platform.\n");
	return FS_EINVAL;
}


// #pragma mark -

port_id
create_port(int32 capacity, const char *name)
{
	fprintf(stderr, "create_port() not supported on this platform.\n");
	return FS_EINVAL;
}


ssize_t
read_port(port_id port, int32 *code, void *buffer, size_t bufferSize)
{
	fprintf(stderr, "read_port() not supported on this platform.\n");
	return FS_EINVAL;
}


status_t
write_port(port_id port, int32 code, const void *buffer, size_t bufferSize)
{
	fprintf(stderr, "write_port() not supported on this platform.\n");
	return FS_EINVAL;
}


status_t
delete_port(port_id port)
{
	fprintf(stderr, "delete_port() not supported on this platform.\n");
	return FS_EINVAL;
}


// #pragma mark -

ssize_t
fs_read_attr(int fd, const char *attribute, uint32 type, off_t pos,
	void *buffer, size_t readBytes)
{
	fprintf(stderr, "fs_read_attr() not supported on this platform.\n");
	errno = FS_EINVAL;
	return -1;
}


ssize_t
fs_write_attr(int fd, const char *attribute, uint32 type, off_t pos,
	const void *buffer, size_t readBytes)
{
	fprintf(stderr, "fs_write_attr() not supported on this platform.\n");
	errno = FS_EINVAL;
	return -1;
}


int
fs_remove_attr(int fd, const char *attribute)
{
	fprintf(stderr, "fs_remove_attr() not supported on this platform.\n");
	errno = FS_EINVAL;
	return -1;
}


int
fs_stat_attr(int fd, const char *attribute, struct attr_info *attrInfo)
{
	fprintf(stderr, "fs_stat_attr() not supported on this platform.\n");
	errno = FS_EINVAL;
	return -1;
}


DIR *
fs_open_attr_dir(const char *path)
{
	fprintf(stderr, "fs_open_attr_dir() not supported on this platform.\n");
	errno = FS_EINVAL;
	return NULL;
}


DIR *
fs_fopen_attr_dir(int fd)
{
	fprintf(stderr, "fs_fopen_attr_dir() not supported on this platform.\n");
	errno = FS_EINVAL;
	return NULL;
}


int
fs_close_attr_dir(DIR *dir)
{
	fprintf(stderr, "fs_close_attr_dir() not supported on this platform.\n");
	errno = FS_EINVAL;
	return -1;
}


struct dirent *
fs_read_attr_dir(DIR *dir)
{
	fprintf(stderr, "fs_read_attr_dir() not supported on this platform.\n");
	errno = FS_EINVAL;
	return NULL;
}


void
fs_rewind_attr_dir(DIR *dir)
{
	fprintf(stderr, "fs_rewind_attr_dir() not supported on this platform.\n");
}

#endif  /* ! __BEOS__ */
