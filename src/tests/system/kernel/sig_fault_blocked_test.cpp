/*
 * Copyright 2026, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>



int
main(int argc, char *argv[])
{
	int test = argc > 1 ? atoi(argv[1]) : 0;

	const uint PAGE_SIZE = sysconf(_SC_PAGESIZE);

	if (test == 0) {
		struct sigaction signalAction;
		sigemptyset(&signalAction.sa_mask);
		signalAction.sa_flags = 0;
		signalAction.sa_handler = SIG_IGN;
		sigaction(SIGSEGV, &signalAction, NULL);
	} else {
		sigset_t signal_mask;
		sigemptyset (&signal_mask);
		sigaddset(&signal_mask, SIGSEGV);
		pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
	}

	char* addr = (char*)mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
	    MAP_ANON | MAP_PRIVATE, -1, 0);
	memset(addr, 0, PAGE_SIZE);
	mprotect(addr, PAGE_SIZE, 0);

	volatile int x = *(int *)(addr);
	(void)x;

}
