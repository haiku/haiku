/*
 * Copyright 2021, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/cdefs.h>


extern "C" {

long __stack_chk_guard = 0;


void
__init_stack_protector()
{
	if (__stack_chk_guard != 0)
		return;

	bool done = false;
	int fd = open("/dev/random", O_RDONLY, 0);
	if (fd >= 0) {
		done = read(fd, &__stack_chk_guard, sizeof(__stack_chk_guard))
			== sizeof(__stack_chk_guard);
		close(fd);
	}

	if (!done) {
		((unsigned char *)(void *)__stack_chk_guard)[0] = 0;
		((unsigned char *)(void *)__stack_chk_guard)[1] = 0;
		((unsigned char *)(void *)__stack_chk_guard)[2] = '\n';
		((unsigned char *)(void *)__stack_chk_guard)[3] = 0xff;
	}
}


void
__stack_chk_fail()
{
	sigset_t mask;
	sigfillset(&mask);
	sigdelset(&mask, SIGABRT);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	abort();
}

}


#if __GNUC__ >= 4
extern "C" void __stack_chk_fail_local() __attribute__((visibility("hidden")));
__weak_reference(__stack_chk_fail, __stack_chk_fail_local);
#endif
