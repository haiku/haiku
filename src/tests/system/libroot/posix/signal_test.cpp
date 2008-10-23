/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#if defined(__BEOS__) && !defined(__HAIKU__)
typedef void (*sighandler_t)(int);
#	define SIGSTKSZ		16384
#	define SA_ONESHOT	0
#	define SA_ONSTACK	0
#	define SA_RESTART	0
#	define ualarm(usec, interval) alarm(1)
#endif

const void* kUserDataMagic = (void *)0x12345678;

static char sAlternateStack[SIGSTKSZ];


bool
is_alternate(void* pointer)
{
	return (char*)pointer > &sAlternateStack[0]
		&& (char*)pointer <= &sAlternateStack[0] + SIGSTKSZ;
}


void
sigHandler(int signal, void *userData, vregs *regs)
{
#if defined(__BEOS__) || defined(__HAIKU__)
	if (userData != kUserDataMagic)
		fprintf(stderr, "FAILED: user data not correct: %p\n", userData);
#endif

	printf("signal handler called with signal %i on %s stack\n",
		signal, is_alternate(&signal) ? "alternate" : "standard");
}


void
wait_for_key()
{
	char buffer[100];
	if (fgets(buffer, sizeof(buffer), stdin) == NULL
		|| buffer[0] == 'q') {
		if (errno == EINTR)
			puts("interrupted");
		else
			exit(0);
	}
}

int
main()
{
	puts("-- 1 (should block) --");

	struct sigaction newAction;
	newAction.sa_handler = (sighandler_t)sigHandler;
	newAction.sa_mask = 0;
	newAction.sa_flags = SA_ONESHOT | SA_ONSTACK | SA_RESTART;
#if defined(__BEOS__) || defined(__HAIKU__)
	newAction.sa_userdata = (void*)kUserDataMagic;
#endif
	sigaction(SIGALRM, &newAction, NULL);

	ualarm(10000, 0);
	wait_for_key();

	puts("-- 2 (does not block, should call handler twice) --");

	newAction.sa_flags = 0;
	sigaction(SIGALRM, &newAction, NULL);

	ualarm(0, 50000);
	wait_for_key();
	wait_for_key();

	ualarm(0, 0);

	puts("-- 3 (alternate stack, should block) --");

#if defined(__BEOS__) && !defined(__HAIKU__)
	set_signal_stack(sAlternateStack, SIGSTKSZ);
#else
	stack_t newStack;
	newStack.ss_sp = sAlternateStack;
	newStack.ss_size = SIGSTKSZ;
	newStack.ss_flags = 0;
	if (sigaltstack(&newStack, NULL) != 0)
		fprintf(stderr, "sigaltstack() failed: %s\n", strerror(errno));
#endif

	newAction.sa_flags = SA_RESTART | SA_ONSTACK;
	sigaction(SIGALRM, &newAction, NULL);

	ualarm(10000, 0);
	wait_for_key();

	puts("-- end --");
	return 0;
}
