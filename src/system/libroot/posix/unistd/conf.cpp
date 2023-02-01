/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <unistd.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/statvfs.h>

#include <SupportDefs.h>

#include <directories.h>
#include <fs_info.h>
#include <posix/realtime_sem_defs.h>
#include <signal_defs.h>
#include <symbol_versioning.h>
#include <syscalls.h>
#include <thread_defs.h>
#include <user_group.h>
#include <user_timer_defs.h>
#include <vfs_defs.h>

#include <errno_private.h>
#include <libroot_private.h>
#include <time_private.h>
#include <unistd_private.h>

#include <OS.h>


int
getdtablesize(void)
{
	struct rlimit rlimit;
	if (getrlimit(RLIMIT_NOFILE, &rlimit) < 0)
		return OPEN_MAX;

	return rlimit.rlim_cur;
}


long
__sysconf_beos(int name)
{
	switch (name) {
		case _SC_CLK_TCK:
			return CLK_TCK_BEOS;
	}

	return __sysconf(name);
}


long
__sysconf(int name)
{
	int err;

	switch (name) {
		case _SC_ARG_MAX:
			return ARG_MAX;
		case _SC_CHILD_MAX:
			return CHILD_MAX;
		case _SC_CLK_TCK:
			return CLK_TCK;
		case _SC_HOST_NAME_MAX:
			return _POSIX_HOST_NAME_MAX;
		case _SC_NGROUPS_MAX:
			return NGROUPS_MAX;
		case _SC_OPEN_MAX:
			return getdtablesize();
		case _SC_STREAM_MAX:
			return STREAM_MAX;
		case _SC_SYMLOOP_MAX:
			return SYMLOOP_MAX;
		case _SC_TTY_NAME_MAX:
			return TTY_NAME_MAX;
		case _SC_TZNAME_MAX:
			return TZNAME_MAX;
		case _SC_VERSION:
			return _POSIX_VERSION;
		case _SC_GETGR_R_SIZE_MAX:
			return MAX_GROUP_BUFFER_SIZE;
		case _SC_GETPW_R_SIZE_MAX:
			return MAX_PASSWD_BUFFER_SIZE;
		case _SC_PAGE_SIZE:
			return B_PAGE_SIZE;
		case _SC_SEM_NSEMS_MAX:
			return _POSIX_SEM_NSEMS_MAX;
		case _SC_SEM_VALUE_MAX:
			return _POSIX_SEM_VALUE_MAX;
		case _SC_IOV_MAX:
			return IOV_MAX;
		case _SC_NPROCESSORS_CONF:
		{
			system_info info;
			err = get_system_info(&info);
			if (err < B_OK) {
				__set_errno(err);
				return -1;
			}
			return info.cpu_count;
		}
		case _SC_NPROCESSORS_ONLN:
		{
			system_info info;
			unsigned int i;
			int count = 0;
			err = get_system_info(&info);
			if (err < B_OK) {
				__set_errno(err);
				return -1;
			}
			for (i = 0; i < info.cpu_count; i++)
				if (_kern_cpu_enabled(i))
					count++;
			return count;
		}
		case _SC_ATEXIT_MAX:
			return ATEXIT_MAX;
		case _SC_PASS_MAX:
			break;
			//XXX:return PASS_MAX;
		case _SC_PHYS_PAGES:
		{
			system_info info;
			err = get_system_info(&info);
			if (err < B_OK) {
				__set_errno(err);
				return -1;
			}
			return info.max_pages;
		}
		case _SC_AVPHYS_PAGES:
		{
			system_info info;
			err = get_system_info(&info);
			if (err < B_OK) {
				__set_errno(err);
				return -1;
			}
			return info.max_pages - info.used_pages;
		}
		case _SC_THREAD_STACK_MIN:
			return MIN_USER_STACK_SIZE;
		case _SC_SIGQUEUE_MAX:
			return MAX_QUEUED_SIGNALS;
		case _SC_RTSIG_MAX:
			return SIGRTMAX - SIGRTMIN + 1;
		case _SC_DELAYTIMER_MAX:
			return MAX_USER_TIMER_OVERRUN_COUNT;
		case _SC_TIMER_MAX:
			return MAX_USER_TIMERS_PER_TEAM;

		/* posix options */
		case _SC_ADVISORY_INFO:
			return _POSIX_ADVISORY_INFO;
		case _SC_BARRIERS:
			return _POSIX_BARRIERS;
		case _SC_CLOCK_SELECTION:
			return _POSIX_CLOCK_SELECTION;
		case _SC_CPUTIME:
			return _POSIX_CPUTIME;
		case _SC_FSYNC:
			return _POSIX_FSYNC;
		case _SC_IPV6:
			return _POSIX_IPV6;
		case _SC_JOB_CONTROL:
			return _POSIX_JOB_CONTROL;
		case _SC_MAPPED_FILES:
			return _POSIX_MAPPED_FILES;
		case _SC_MEMLOCK:
			return _POSIX_MEMLOCK;
		case _SC_MEMORY_PROTECTION:
			return _POSIX_MEMORY_PROTECTION;
		case _SC_MESSAGE_PASSING:
			return _POSIX_MESSAGE_PASSING;
		case _SC_MONOTONIC_CLOCK:
			return _POSIX_MONOTONIC_CLOCK;
		case _SC_PRIORITIZED_IO:
			return _POSIX_PRIORITIZED_IO;
		case _SC_PRIORITY_SCHEDULING:
			return _POSIX_PRIORITY_SCHEDULING;
		case _SC_READER_WRITER_LOCKS:
			return _POSIX_READER_WRITER_LOCKS;
		case _SC_REALTIME_SIGNALS:
			return _POSIX_REALTIME_SIGNALS;
		case _SC_REGEXP:
			return _POSIX_REGEXP;
		case _SC_SAVED_IDS:
			return _POSIX_SAVED_IDS;
		case _SC_SEMAPHORES:
			return _POSIX_SEMAPHORES;
		case _SC_SHARED_MEMORY_OBJECTS:
			return _POSIX_SHARED_MEMORY_OBJECTS;
		case _SC_SHELL:
			return _POSIX_SHELL;
		case _SC_SPAWN:
			return _POSIX_SPAWN;
		case _SC_SPIN_LOCKS:
			return _POSIX_SPIN_LOCKS;
		case _SC_SPORADIC_SERVER:
			return _POSIX_SPORADIC_SERVER;
		case _SC_SYNCHRONIZED_IO:
			return _POSIX_SYNCHRONIZED_IO;
		case _SC_THREAD_ATTR_STACKADDR:
			return _POSIX_THREAD_ATTR_STACKADDR;
		case _SC_THREAD_ATTR_STACKSIZE:
			return _POSIX_THREAD_ATTR_STACKSIZE;
		case _SC_THREAD_CPUTIME:
			return _POSIX_THREAD_CPUTIME;
		case _SC_THREAD_PRIO_INHERIT:
			return _POSIX_THREAD_PRIO_INHERIT;
		case _SC_THREAD_PRIO_PROTECT:
			return _POSIX_THREAD_PRIO_PROTECT;
		case _SC_THREAD_PRIORITY_SCHEDULING:
			return _POSIX_THREAD_PRIORITY_SCHEDULING;
		case _SC_THREAD_PROCESS_SHARED:
			return _POSIX_THREAD_PROCESS_SHARED;
		case _SC_THREAD_ROBUST_PRIO_INHERIT:
			return _POSIX_THREAD_ROBUST_PRIO_INHERIT;
		case _SC_THREAD_ROBUST_PRIO_PROTECT:
			return _POSIX_THREAD_ROBUST_PRIO_PROTECT;
		case _SC_THREAD_SAFE_FUNCTIONS:
			return _POSIX_THREAD_SAFE_FUNCTIONS;
		case _SC_THREADS:
			return _POSIX_THREADS;
		case _SC_TIMEOUTS:
			return _POSIX_TIMEOUTS;
		case _SC_TIMERS:
			return _POSIX_TIMERS;
		case _SC_TRACE:
			return _POSIX_TRACE;
		case _SC_TRACE_EVENT_FILTER:
			return _POSIX_TRACE_EVENT_FILTER;
		case _SC_TRACE_INHERIT:
			return _POSIX_TRACE_INHERIT;
		case _SC_TRACE_LOG:
			return _POSIX_TRACE_LOG;
		case _SC_TYPED_MEMORY_OBJECTS:
			return _POSIX_TYPED_MEMORY_OBJECTS;

		case _SC_V6_ILP32_OFF32:
		case _SC_V7_ILP32_OFF32:
#if _ILP32_OFF32 == 0
			if (sizeof(int) * CHAR_BIT == 32 &&
			    sizeof(long) * CHAR_BIT == 32 &&
				sizeof(void *) * CHAR_BIT == 32 &&
				sizeof(off_t) * CHAR_BIT == 32)
				return 1;
			else
				return -1;
#else
			return _ILP32_OFF32;
#endif
		case _SC_V6_ILP32_OFFBIG:
		case _SC_V7_ILP32_OFFBIG:
#if _ILP32_OFFBIG == 0
			if (sizeof(int) * CHAR_BIT == 32 &&
				sizeof(long) * CHAR_BIT == 32 &&
				sizeof(void *) * CHAR_BIT == 32 &&
				sizeof(off_t) * CHAR_BIT >= 64)
				return 1;
			else
				return -1;
#else
			return _ILP32_OFFBIG;
#endif
		case _SC_V6_LP64_OFF64:
		case _SC_V7_LP64_OFF64:
#if _LP64_OFF64 == 0
			if (sizeof(int) * CHAR_BIT == 64 &&
				sizeof(long) * CHAR_BIT == 64 &&
				sizeof(void *) * CHAR_BIT == 64 &&
				sizeof(off_t) * CHAR_BIT == 64)
				return 1;
			else
				return -1;
#else
			return _LP64_OFF64;
#endif
		case _SC_V6_LPBIG_OFFBIG:
		case _SC_V7_LPBIG_OFFBIG:
#if _LPBIG_OFFBIG == 0
			if (sizeof(int) * CHAR_BIT >= 32 &&
				sizeof(long) * CHAR_BIT >= 64 &&
				sizeof(void *) * CHAR_BIT >= 64 &&
				sizeof(off_t) * CHAR_BIT >= 64)
				return 1;
			else
				return -1;
#else
			return _LPBIG_OFFBIG;
#endif
		case _SC_2_C_BIND:
			return _POSIX2_C_BIND;
		case _SC_2_C_DEV:
			return _POSIX2_C_DEV;
		case _SC_2_CHAR_TERM:
			return _POSIX2_CHAR_TERM;
		case _SC_2_FORT_DEV:
			return _POSIX2_FORT_DEV;
		case _SC_2_FORT_RUN:
			return _POSIX2_FORT_RUN;
		case _SC_2_LOCALEDEF:
			return _POSIX2_LOCALEDEF;
		case _SC_2_PBS:
			return _POSIX2_PBS;
		case _SC_2_PBS_ACCOUNTING:
			return _POSIX2_PBS_ACCOUNTING;
		case _SC_2_PBS_CHECKPOINT:
			return _POSIX2_PBS_CHECKPOINT;
		case _SC_2_PBS_LOCATE:
			return _POSIX2_PBS_LOCATE;
		case _SC_2_PBS_MESSAGE:
			return _POSIX2_PBS_MESSAGE;
		case _SC_2_PBS_TRACK:
			return _POSIX2_PBS_TRACK;
		case _SC_2_VERSION:
			return _POSIX2_VERSION;
		case _SC_XOPEN_CRYPT:
			return _XOPEN_CRYPT;
		case _SC_XOPEN_ENH_I18N:
			return _XOPEN_ENH_I18N;
		case _SC_XOPEN_REALTIME:
			return _XOPEN_REALTIME;
		case _SC_XOPEN_REALTIME_THREADS:
			return _XOPEN_REALTIME_THREADS;
		case _SC_XOPEN_SHM:
			return _XOPEN_SHM;
		case _SC_XOPEN_STREAMS:
			return _XOPEN_STREAMS;
		case _SC_XOPEN_UNIX:
			return _XOPEN_UNIX;
		case _SC_XOPEN_UUCP:
			return _XOPEN_UUCP;
		case _SC_XOPEN_VERSION:
			return _XOPEN_VERSION;

#if _POSIX_ASYNCHRONOUS_IO >= 0
		case _SC_AIO_LISTIO_MAX:
			return AIO_LISTIO_MAX;
		case _SC_AIO_MAX:
			return AIO_MAX;
		case _SC_AIO_PRIO_DELTA_MAX:
			return AIO_PRIO_DELTA_MAX;
#endif

		case _SC_BC_BASE_MAX:
			return BC_BASE_MAX;
		case _SC_BC_DIM_MAX:
			return BC_DIM_MAX;
		case _SC_BC_SCALE_MAX:
			return BC_SCALE_MAX;
		case _SC_BC_STRING_MAX:
			return BC_STRING_MAX;

		case _SC_COLL_WEIGHTS_MAX:
			return COLL_WEIGHTS_MAX;
		case _SC_EXPR_NEST_MAX:
			return EXPR_NEST_MAX;
		case _SC_LINE_MAX:
			return LINE_MAX;
		case _SC_LOGIN_NAME_MAX:
			return LOGIN_NAME_MAX;
		case _SC_MQ_OPEN_MAX:
			return MQ_OPEN_MAX;
		case _SC_MQ_PRIO_MAX:
			return  MQ_PRIO_MAX;
		case _SC_THREAD_DESTRUCTOR_ITERATIONS:
			return PTHREAD_DESTRUCTOR_ITERATIONS;
		case _SC_THREAD_KEYS_MAX:
			return PTHREAD_KEYS_MAX;
		case _SC_THREAD_THREADS_MAX:
			system_info info;
			get_system_info(&info);
			return (int)info.max_threads;
		case _SC_RE_DUP_MAX:
			return RE_DUP_MAX;

		// not POSIX (anymore)
		case _SC_PIPE:
		case _SC_SELECT:
		case _SC_POLL:
			return 1;
	}

	__set_errno(EINVAL);
	return -1;
}


enum {
	FS_BFS,
	FS_FAT,
	FS_EXT,
	FS_UNKNOWN
};


static int
fstype(const char *fsh_name)
{
	if (!strncmp(fsh_name, "bfs", B_OS_NAME_LENGTH))
		return FS_BFS;
	if (!strncmp(fsh_name, "dos", B_OS_NAME_LENGTH))
		return FS_FAT;
	if (!strncmp(fsh_name, "fat", B_OS_NAME_LENGTH))
		return FS_FAT;
	if (!strncmp(fsh_name, "ext2", B_OS_NAME_LENGTH))
		return FS_EXT;
	if (!strncmp(fsh_name, "ext3", B_OS_NAME_LENGTH))
		return FS_EXT;
	return FS_UNKNOWN;
}



static long
__pathconf_common(struct statvfs *fs, struct stat *st,
	int name)
{
	fs_info info;
	int ret;
	ret = fs_stat_dev(fs->f_fsid, &info);
	if (ret < 0) {
		__set_errno(ret);
		return -1;
	}

	// TODO: many cases should check for file type from st.
	switch (name) {
		case _PC_CHOWN_RESTRICTED:
			return _POSIX_CHOWN_RESTRICTED;

		case _PC_MAX_CANON:
			return MAX_CANON;

		case _PC_MAX_INPUT:
			return MAX_INPUT;

		case _PC_NAME_MAX:
			return fs->f_namemax;
			//return NAME_MAX;

		case _PC_NO_TRUNC:
			return _POSIX_NO_TRUNC;

		case _PC_PATH_MAX:
			return PATH_MAX;

		case _PC_PIPE_BUF:
			return VFS_FIFO_ATOMIC_WRITE_SIZE;

		case _PC_LINK_MAX:
			return LINK_MAX;

		case _PC_VDISABLE:
			return _POSIX_VDISABLE;

		case _PC_FILESIZEBITS:
		{
			int type = fstype(info.fsh_name);
			switch (type) {
				case FS_BFS:
				case FS_EXT:
					return 64;
				case FS_FAT:
					return 32;
			}
			// XXX: add fs ? add to statvfs/fs_info ?
			return FILESIZEBITS;
		}

		case _PC_SYMLINK_MAX:
			return SYMLINK_MAX;

		case _PC_2_SYMLINKS:
		{
			int type = fstype(info.fsh_name);
			switch (type) {
				case FS_BFS:
				case FS_EXT:
					return 1;
				case FS_FAT:
					return 0;
			}
			// XXX: there should be an HAS_SYMLINKS flag
			// to fs_info...
			return 1;
		}

		case _PC_XATTR_EXISTS:
		case _PC_XATTR_ENABLED:
		{
#if 0
			/* those seem to be Solaris specific,
			 * else we should return 1 I suppose.
			 * we don't yet map POSIX xattrs
			 * to BFS ones anyway.
			 */
			if (info.flags & B_FS_HAS_ATTR)
				return 1;
			return -1;
#endif
			__set_errno(EINVAL);
			return -1;
		}

		case _PC_SYNC_IO:
		case _PC_ASYNC_IO:
		case _PC_PRIO_IO:
		case _PC_SOCK_MAXBUF:
		case _PC_REC_INCR_XFER_SIZE:
		case _PC_REC_MAX_XFER_SIZE:
		case _PC_REC_MIN_XFER_SIZE:
		case _PC_REC_XFER_ALIGN:
		case _PC_ALLOC_SIZE_MIN:
			/* not yet supported */
			__set_errno(EINVAL);
			return -1;

	}

	__set_errno(EINVAL);
	return -1;
}


long
fpathconf(int fd, int name)
{
	struct statvfs fs;
	struct stat st;
	int ret;
	if (fd < 0) {
		__set_errno(EBADF);
		return -1;
	}
	ret = fstat(fd, &st);
	if (ret < 0)
		return ret;
	ret = fstatvfs(fd, &fs);
	if (ret < 0)
		return ret;
	return __pathconf_common(&fs, &st, name);
}


long
pathconf(const char *path, int name)
{
	struct statvfs fs;
	struct stat st;
	int ret;
	if (path == NULL) {
		__set_errno(EFAULT);
		return -1;
	}
	ret = lstat(path, &st);
	if (ret < 0)
		return ret;
	ret = statvfs(path, &fs);
	if (ret < 0)
		return ret;
	return __pathconf_common(&fs, &st, name);
}


size_t
confstr(int name, char *buffer, size_t length)
{
	const char *string = "";

	switch (name) {
		case _CS_PATH:
			string = kSystemNonpackagedBinDirectory ":" kGlobalBinDirectory ":"
				kSystemAppsDirectory ":" kSystemPreferencesDirectory;
			break;
		default:
			__set_errno(EINVAL);
			return 0;
	}

	if (buffer != NULL)
		strlcpy(buffer, string, length);

	return strlen(string) + 1;
}


DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sysconf_beos", "sysconf@", "BASE");

DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sysconf", "sysconf@@", "1_ALPHA4");
