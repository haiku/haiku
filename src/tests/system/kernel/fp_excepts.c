#define _DEFAULT_SOURCE
#include <assert.h>
#include <errno.h>
#include <fenv.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>


static int sExceptions = 0;
static int sLastCode = -1;
static jmp_buf sJmpBuf;


static void
signal_handler(int signal, siginfo_t* siginfo, void*)
{
	sExceptions++;
	sLastCode = siginfo->si_code;

	longjmp(sJmpBuf, 1);
}


int main()
{
	fenv_t exceptenv;
	long double volatile nanl, resultl;

	struct sigaction action = {};
	sigemptyset(&action.sa_mask);
	action.sa_flags = SA_SIGINFO | SA_NODEFER | SA_NOMASK;
	action.sa_sigaction = signal_handler;
	if (sigaction(SIGFPE, &action, NULL) == -1) {
		fprintf(stderr, "failed to install signal handler: %s\n",
			strerror(errno));
		return 1;
	}

	assert(feclearexcept(FE_ALL_EXCEPT) == 0);

	// Test 1: switching to an fenv with an exception raised triggers SIGFPE
	// (but not a kernel panic.)
	if (feenableexcept(FE_INVALID) == -1) {
		fputs("trapping floating-point exceptions are not supported on this machine.\n", stderr);
		return -1;
	}

	assert(fegetenv(&exceptenv) == 0);
	assert(fesetenv(FE_DFL_ENV) == 0);

	assert(feraiseexcept(FE_INVALID) == 0);

	assert(sExceptions == 0);
	if (setjmp(sJmpBuf) == 0) {
		assert(feupdateenv(&exceptenv) == 0);
	} else {
		assert(feclearexcept(FE_INVALID) == 0);
		assert(feupdateenv(&exceptenv) == 0);
	}
	assert(sExceptions == 1);
	assert(sLastCode == FPE_FLTINV);

	// Test 2: long signaling NaNs also trigger SIGFPE
	// (but again no kernel panic.)
	nanl = __builtin_nansl("");
	assert(sExceptions == 1);
	if (setjmp(sJmpBuf) == 0) {
		resultl = nanl + 42.0L;
	} else {
		assert(feclearexcept(FE_ALL_EXCEPT) == 0);
	}
	assert(sExceptions == 2);
	assert(sLastCode == FPE_FLTINV);

	// Test 3: Overflow triggers SIGFPE, with the right code.
	if (feenableexcept(FE_OVERFLOW) == -1) {
		fputs("FE_OVERFLOW is not supported on this machine.\n", stderr);
		return 0;
	}

	if (setjmp(sJmpBuf) == 0) {
		float volatile top = FLT_MAX;
		float volatile result = top * top;
	}
	assert(sExceptions == 3);
	assert(sLastCode == FPE_FLTOVF);

	return 0;
}
