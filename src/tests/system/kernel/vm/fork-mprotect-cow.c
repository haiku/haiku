/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2008 David Gibson, IBM Corporation.
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
#define _GNU_SOURCE

#include <sys/types.h>
//#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
//#include <hugetlbfs.h>
#include "hugetlbfs-testutils.h"

/*
 * Test rationale:
 *
 * This checks copy-on-write semantics, specifically the semantics of
 * a MAP_PRIVATE mapping across a fork().  Some versions of the
 * powerpc kernel had a bug in huge_ptep_set_wrprotect() which would
 * fail to flush the hash table after setting the write protect bit in
 * the parent's page tables, thus allowing the parent to pollute the
 * child's mapping.
 */

#define RANDOM_CONSTANT		0x1234ABCD
#define OTHER_CONSTANT		0xfeef5678

/*
 * The parent uses this to check if the child terminated badly.
 */
static void sigchld_handler(int signum, siginfo_t *si, void *uc)
{
	if (WEXITSTATUS(si->si_status) != 0)
		FAIL("Child failed: %d", WEXITSTATUS(si->si_status));
	if (WIFSIGNALED(si->si_status))
		FAIL("Child recived signal %s",
			strsignal(WTERMSIG(si->si_status)));
}

static sigjmp_buf sig_escape;
static void *sig_expected = MAP_FAILED;

static void sig_handler(int signum, siginfo_t *si, void *uc)
{
	if (signum == SIGSEGV) {
		verbose_printf("SIGSEGV at %p (sig_expected=%p)\n", si->si_addr,
			       sig_expected);
		if (si->si_addr == sig_expected) {
			siglongjmp(sig_escape, 1);
		}
		FAIL("SIGSEGV somewhere unexpected");
	}
	FAIL("Unexpected signal %s", strsignal(signum));
}

static int test_read(void *p)
{
	volatile unsigned long *pl = p;
	unsigned long x;

	if (sigsetjmp(sig_escape, 1)) {
		/* We got a SEGV */
		sig_expected = MAP_FAILED;
		return -1;
	}

	sig_expected = p;
	barrier();
	x = *pl;
	verbose_printf("Read back %lu\n", x);
	barrier();
	sig_expected = MAP_FAILED;
	/*
	 * gcc 5 complains about x not ever being used, the following
	 * statement is solely here to shut it up
	 */
	pl = (unsigned long *)x;

	return 0;
}

static int test_write(void *p, unsigned long val)
{
	volatile unsigned long *pl = p;
	unsigned long x;

	if (sigsetjmp(sig_escape, 1)) {
		/* We got a SEGV */
		sig_expected = MAP_FAILED;
		return -1;
	}

	sig_expected = p;
	barrier();
	*pl = val;
	x = *pl;
	barrier();
	sig_expected = MAP_FAILED;

	return (x != val);
}


int main(int argc, char ** argv)
{
	int fd, ret, status;
	void *syncarea;
	volatile unsigned int *p;
	volatile unsigned int *trigger, *child_readback;
	unsigned int parent_readback;
	long hpage_size;
	pid_t pid;
	int prot;
	int err;
	struct sigaction sa = {
		.sa_sigaction = sigchld_handler,
		.sa_flags = SA_SIGINFO,
	};

	test_init(argc, argv);

//	check_free_huge_pages(2);

	if (argc != 1)
		CONFIG("Usage: fork-cow\n");

	/* Get a shared normal page for synchronization */
	verbose_printf("Mapping synchronization area..");
	syncarea = mmap(NULL, getpagesize(), PROT_READ|PROT_WRITE,
			MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	if (syncarea == MAP_FAILED)
		FAIL("mmap() sync area: %s", strerror(errno));
	verbose_printf("done\n");
	verbose_printf("Mprotect synchronization area..");
	err = mprotect(syncarea, getpagesize(), PROT_READ|PROT_WRITE);
	if (err != 0)
		FAIL("mprotect() syncarea: %s", strerror(errno));
	verbose_printf("done\n");

	trigger = syncarea;
	*trigger = 0;

	child_readback = trigger + 1;
	*child_readback = 0;

	hpage_size = check_hugepagesize();

	fd = hugetlbfs_unlinked_fd(hpage_size*8);
	if (fd < 0)
		CONFIG("hugetlbfs_unlinked_fd() failed\n");

	verbose_printf("Mapping hugepage area...");
	p = mmap(NULL, hpage_size*8, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap(): %s", strerror(errno));
	verbose_printf("mapped at %p\n", p);

	/* Touch the page for write in parent */
	verbose_printf("Parent writes pre-fork...");
	*p = RANDOM_CONSTANT;
	verbose_printf("%x\n", RANDOM_CONSTANT);
	
	prot = 0;
	verbose_printf("mprotect()ing to prot=%x\n", prot);
	err = mprotect(p, hpage_size, prot);
	if (err != 0)
		FAIL("mprotect(prot=%x): %s", prot,
		     strerror(errno));


	ret = sigaction(SIGCHLD, &sa, NULL);
	if (ret)
		FAIL("sigaction(): %s", strerror(errno));
		
	struct sigaction sa2 = {
		.sa_sigaction = sig_handler,
		.sa_flags = SA_SIGINFO,
	};

	err = sigaction(SIGSEGV, &sa2, NULL);
	if (err)
		FAIL("Can't install SIGSEGV handler: %s", strerror(errno));


	if ((pid = fork()) < 0)
		FAIL("fork(): %s", strerror(errno));

	if (pid != 0) {
		int r;
		/* Parent */
		verbose_printf("Parent reading..");
		r = test_read(p);
		verbose_printf("%d\n", r);
		
		*trigger = 1;

		while (*trigger != 2)
			;

		if (prot & PROT_READ) {
			if (r != 0)
				FAIL("Parent read failed on mmap(prot=%x)", prot);
		} else {
			if (r != -1)
				FAIL("Parent read succeeded on mmap(prot=%x)", prot);
		}
	} else {
		int r;
		/* Child */
		verbose_printf("Child starts..\n");
		while (*trigger != 1)
			;

		verbose_printf("Child reading..");
		r = test_read(p);
		verbose_printf("%d\n", r);

		*trigger = 2;

		if (prot & PROT_READ) {
			if (r != 0)
				FAIL("Child read failed on mmap(prot=%x)", prot);
		} else {
			if (r != -1)
				FAIL("Child read succeeded on mmap(prot=%x)", prot);
		}

		verbose_printf("Child exits...\n");
		exit(0);
	}

	ret = waitpid(pid, &status, 0);
	if (ret < 0)
		FAIL("waitpid(): %s", strerror(errno));

	PASS();
}
