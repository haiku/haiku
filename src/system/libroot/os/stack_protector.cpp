/*
 * Copyright 2021, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/cdefs.h>

#include "private/system/symbol_visibility.h"


extern "C" {

long __stack_chk_guard = 0;


void
__init_stack_protector()
{
	if (__stack_chk_guard != 0)
		return;

	int status = getentropy(&__stack_chk_guard, sizeof(__stack_chk_guard));
	if (status != 0) {
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
