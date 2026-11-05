/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2005-2006 David Gibson & Adam Litke, IBM Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#define _LARGEFILE64_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
//#include <sys/vfs.h>
#include <sys/ipc.h>
//#include <sys/shm.h>
#include <sys/stat.h>
//#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>

//#include "hugetlbfs.h"
#include "hugetlbfs-testutils.h"

#define HUGETLBFS_MAGIC	0x958458f6
#define BUF_SZ 1024
#define MEMINFO_SZ 2048

int verbose_test = 1;
char *test_name;

void check_must_be_root(void)
{
	uid_t uid = getuid();
	if (uid != 0)
		CONFIG("Must be root");
}


#if 0
void check_hugetlb_shm_group(void)
{
	int fd;
	ssize_t ret;
	char gid_buffer[64] = {0};
	gid_t hugetlb_shm_group;
	gid_t gid = getgid();
	uid_t uid = getuid();

	/* root is an exception */
	if (uid == 0)
		return;

	fd = open("/proc/sys/vm/hugetlb_shm_group", O_RDONLY);
	if (fd < 0)
		ERROR("Unable to open /proc/sys/vm/hugetlb_shm_group: %s",
							strerror(errno));
	ret = read(fd, &gid_buffer, sizeof(gid_buffer));
	if (ret < 0)
		ERROR("Unable to read /proc/sys/vm/hugetlb_shm_group: %s",
							strerror(errno));
	hugetlb_shm_group = atoi(gid_buffer);
	close(fd);
	if (hugetlb_shm_group != gid)
		CONFIG("Do not have permission to use SHM_HUGETLB");
}

#define SYSFS_CPU_ONLINE_FMT	"/sys/devices/system/cpu/cpu%d/online"

void check_online_cpus(int online_cpus[], int nr_cpus_needed)
{
	char cpu_state, path_buf[64];
	int total_cpus, cpu_idx, fd, ret, i;

	total_cpus = get_nprocs_conf();
	cpu_idx = 0;

	if (get_nprocs() < nr_cpus_needed)
		CONFIG("Atleast online %d cpus are required", nr_cpus_needed);

	for (i = 0; i < total_cpus && cpu_idx < nr_cpus_needed; i++) {
		errno = 0;
		sprintf(path_buf, SYSFS_CPU_ONLINE_FMT, i);
		fd = open(path_buf, O_RDONLY);
		if (fd < 0) {
			/* If 'online' is absent, the cpu cannot be offlined */
			if (errno == ENOENT) {
				online_cpus[cpu_idx] = i;
				cpu_idx++;
				continue;
			} else {
				FAIL("Unable to open %s: %s", path_buf,
				     strerror(errno));
			}
		}

		ret = read(fd, &cpu_state, 1);
		if (ret < 1)
			FAIL("Unable to read %s: %s", path_buf,
			     strerror(errno));

		if (cpu_state == '1') {
			online_cpus[cpu_idx] = i;
			cpu_idx++;
		}

		close(fd);
	}

	if (cpu_idx < nr_cpus_needed)
		CONFIG("Atleast %d online cpus were not found", nr_cpus_needed);
}
#endif

void  __attribute__((weak)) cleanup(void)
{
}

#if 0
static void segv_handler(int signum, siginfo_t *si, void *uc)
{
	FAIL("Segmentation fault");
}
#endif

static void sigint_handler(int signum, siginfo_t *si, void *uc)
{
	cleanup();
	fprintf(stderr, "%s: %s (pid=%d)\n", test_name,
		strsignal(signum), getpid());
	exit(RC_BUG);
}

void test_init(int argc, char *argv[])
{
	int err;
	struct sigaction sa_int = {
		.sa_sigaction = sigint_handler,
	};

	test_name = argv[0];

	err = sigaction(SIGINT, &sa_int, NULL);
	if (err)
		FAIL("Can't install SIGINT handler: %s", strerror(errno));

	if (getenv("QUIET_TEST"))
		verbose_test = 0;

	verbose_printf("Starting testcase \"%s\", pid %d\n",
		       test_name, getpid());
}


#if 0

#define MAPS_BUF_SZ 4096

static int read_maps(unsigned long addr, char *buf)
{
	FILE *f;
	char line[MAPS_BUF_SZ];
	char *tmp;

	f = fopen("/proc/self/maps", "r");
	if (!f) {
		ERROR("Failed to open /proc/self/maps: %s\n", strerror(errno));
		return -1;
	}

	while (1) {
		unsigned long start, end, off, ino;
		int ret;

		tmp = fgets(line, MAPS_BUF_SZ, f);
		if (!tmp)
			break;

		buf[0] = '\0';
		ret = sscanf(line, "%lx-%lx %*s %lx %*s %ld %255s",
			     &start, &end, &off, &ino,
			     buf);
		if ((ret < 4) || (ret > 5)) {
			ERROR("Couldn't parse /proc/self/maps line: %s\n",
			      line);
			fclose(f);
			return -1;
		}

		if ((start <= addr) && (addr < end)) {
			fclose(f);
			return 1;
		}
	}

	fclose(f);
	return 0;
}

int range_is_mapped(unsigned long low, unsigned long high)
{
	FILE *f;
	char line[MAPS_BUF_SZ];
	char *tmp;

	f = fopen("/proc/self/maps", "r");
	if (!f) {
		ERROR("Failed to open /proc/self/maps: %s\n", strerror(errno));
		return -1;
	}

	while (1) {
		unsigned long start, end;
		int ret;

		tmp = fgets(line, MAPS_BUF_SZ, f);
		if (!tmp)
			break;

		ret = sscanf(line, "%lx-%lx", &start, &end);
		if (ret != 2) {
			ERROR("Couldn't parse /proc/self/maps line: %s\n",
			      line);
			fclose(f);
			return -1;
		}

		if ((start >= low) && (start < high)) {
			fclose(f);
			return 1;
		}
		if ((end >= low) && (end < high)) {
			fclose(f);
			return 1;
		}

	}

	fclose(f);
	return 0;
}

/*
 * With the inclusion of MAP_HUGETLB it is now possible to have huge pages
 * without using hugetlbfs, so not all huge page regions will show with the
 * test that reads /proc/self/maps.  Instead we ask /proc/self/smaps for
 * the KernelPageSize.  On success we return the page size (in bytes) for the
 * mapping that contains addr, on failure we return 0
 */
unsigned long long get_mapping_page_size(void *p)
{
	FILE *f;
	char line[MAPS_BUF_SZ];
	char *tmp;
	unsigned long addr = (unsigned long)p;

	f = fopen("/proc/self/smaps", "r");
	if (!f) {
		ERROR("Unable to open /proc/self/smaps\n");
		return 0;
	}

	while ((tmp = fgets(line, MAPS_BUF_SZ, f))) {
		unsigned long start, end, dummy;
		char map_name[256];
		char buf[64];
		int ret;

		ret = sscanf(line, "%lx-%lx %s %lx %s %ld %s", &start, &end,
				buf, &dummy, buf, &dummy, map_name);
		if (ret < 7 || start > addr || end <= addr)
			continue;

		while ((tmp = fgets(line, MAPS_BUF_SZ, f))) {
			unsigned long long page_size;

			ret = sscanf(line, "KernelPageSize: %lld kB",
					&page_size);
			if (ret == 0 )
				continue;
			if (ret < 1 || page_size <= 0) {
				ERROR("Cannot parse /proc/self/smaps\n");
				page_size = 0;
			}

			fclose(f);
			/* page_size is reported in kB, we return B */
			return page_size * 1024;
		}
	}

	/* We couldn't find an entry for this addr in smaps */
	fclose(f);
	return 0;
}

/* We define this function standalone, rather than in terms of
 * hugetlbfs_test_path() so that we can use it without -lhugetlbfs for
 * testing PRELOAD */
int test_addr_huge(void *p)
{
	char name[256];
	char *dirend;
	int ret;
	struct statfs64 sb;

	ret = read_maps((unsigned long)p, name);
	if (ret < 0)
		return ret;
	if (ret == 0) {
		verbose_printf("Couldn't find address %p in /proc/self/maps\n",
			       p);
		return -1;
	}

	/* looks like a filename? */
	if (name[0] != '/')
		return 0;

	/* Truncate the filename portion */

	dirend = strrchr(name, '/');
	if (dirend && dirend > name) {
		*dirend = '\0';
	}

	ret = statfs64(name, &sb);
	if (ret)
		return -1;

	return (sb.f_type == HUGETLBFS_MAGIC);
}

ino_t get_addr_inode(void *p)
{
	char name[256];
	int ret;
	struct stat sb;

	ret = read_maps((unsigned long)p, name);
	if (ret < 0)
		return ret;
	if (ret == 0) {
		ERROR("Couldn't find address %p in /proc/self/maps\n", p);
		return -1;
	}

	/* Don't care about non-filenames */
	if (name[0] != '/')
		return 0;

	/* Truncate the filename portion */

	ret = stat(name, &sb);
	if (ret < 0) {
		/* Don't care about unlinked files */
		if (errno == ENOENT)
			return 0;
		ERROR("stat failed: %s\n", strerror(errno));
		return -1;
	}

	return sb.st_ino;
}


int remove_shmid(int shmid)
{
	if (shmid >= 0) {
		if (shmctl(shmid, IPC_RMID, NULL) != 0) {
			ERROR("shmctl(%x, IPC_RMID) failed (%s)\n",
			      shmid, strerror(errno));
			return -1;
		}
	}
	return 0;
}
#endif

#if 1
int hugetlbfs_unlinked_fd(int size)
{
	const char *path;
	char name[PATH_MAX+1];
	int fd;

	path = "/tmp";
	if (!path)
		return -1;

	name[sizeof(name)-1] = '\0';

	strcpy(name, path);
	strncat(name, "/libhugetlbfs.tmp.XXXXXX", sizeof(name)-1);
	/* FIXME: deal with overflows */

	fd = mkstemp(name);

	if (fd < 0) {
		ERROR("mkstemp() failed: %s\n", strerror(errno));
		return -1;
	}
	
	ftruncate(fd, size);

	unlink(name);

	return fd;
}


#endif
