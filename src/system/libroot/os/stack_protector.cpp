/*
 * Copyright 2021, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/cdefs.h>
#include <syscalls.h>

#include "private/system/random_defs.h"
#include "private/system/symbol_visibility.h"


extern "C" {

long __stack_chk_guard = 0;


void
__init_stack_protector()
{
	if (__stack_chk_guard != 0)
		return;

	struct random_get_entropy_args args;
	args.buffer = &__stack_chk_guard;
	args.length = sizeof(__stack_chk_guard);

	status_t status = _kern_generic_syscall(RANDOM_SYSCALLS, RANDOM_GET_ENTROPY,
		&args, sizeof(args));

	if (status != B_OK || args.length != sizeof(__stack_chk_guard)) {
		unsigned char* p = (unsigned char *)&__stack_chk_guard;
		p[0] = 0;
		p[1] = 0;
		p[2] = '\n';
		p[3] = 0xff;
	}
}


void
__stack_chk_fail()
{
	HIDDEN_FUNCTION(__stack_chk_fail_local);
	sigset_t mask;
	sigfillset(&mask);
	sigdelset(&mask, SIGABRT);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	abort();
}

}

extern "C" void __stack_chk_fail_local() HIDDEN_FUNCTION_ATTRIBUTE;
__weak_reference(__stack_chk_fail, __stack_chk_fail_local);
