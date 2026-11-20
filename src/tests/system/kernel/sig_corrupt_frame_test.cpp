/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#define _DEFAULT_SOURCE
#include <signal.h>
#include <string.h>
#include <unistd.h>


static void
corrupting_signal_handler(int signal, void *userData, vregs *regs)
{
	memset(regs, 0xAA, sizeof(vregs));
}


static void
exit_signal_handler(int signal, void *userData, vregs *regs)
{
	_exit(0);
}


int
main()
{
	struct sigaction sigusr1Action = {};
	sigusr1Action.sa_handler = (sighandler_t)corrupting_signal_handler;
	sigaction(SIGUSR1, &sigusr1Action, NULL);

	struct sigaction sigsegvAction = {};
	sigsegvAction.sa_handler = (sighandler_t)exit_signal_handler;
	sigaction(SIGSEGV, &sigsegvAction, NULL);
	sigaction(SIGBUS, &sigsegvAction, NULL);

	raise(SIGUSR1);

	// We shouldn't get here, but rather exit via the SIGSEGV handler.
	return 1;
}
