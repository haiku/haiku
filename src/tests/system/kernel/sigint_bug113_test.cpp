#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define ADD_SIGNAL(signal)	{ signal, #signal }

const struct {
	int			signal;
	const char*	name;
} kSignals[] = {
	ADD_SIGNAL(SIGHUP),
	ADD_SIGNAL(SIGINT),
	ADD_SIGNAL(SIGQUIT),
	ADD_SIGNAL(SIGILL),
	ADD_SIGNAL(SIGCHLD),
	ADD_SIGNAL(SIGABRT),
	ADD_SIGNAL(SIGPIPE),
	ADD_SIGNAL(SIGFPE),
	ADD_SIGNAL(SIGKILL),
	ADD_SIGNAL(SIGSTOP),
	ADD_SIGNAL(SIGSEGV),
	ADD_SIGNAL(SIGCONT),
	ADD_SIGNAL(SIGTSTP),
	ADD_SIGNAL(SIGALRM),
	ADD_SIGNAL(SIGTERM),
	ADD_SIGNAL(SIGTTIN),
	ADD_SIGNAL(SIGTTOU),
	ADD_SIGNAL(SIGUSR1),
	ADD_SIGNAL(SIGUSR2),
	ADD_SIGNAL(SIGWINCH),
	ADD_SIGNAL(SIGKILLTHR),
	ADD_SIGNAL(SIGTRAP),
	{-1, NULL}
};

#define ADD_SA_FLAG(flag)	{ flag, #flag }


const struct {
	int			flag;
	const char*	name;
} kSigActionFlags[] = {
	ADD_SA_FLAG(SA_NOCLDSTOP),
#ifdef SA_NOCLDWAIT
		ADD_SA_FLAG(SA_NOCLDWAIT),
#endif
#ifdef SA_RESETHAND
	ADD_SA_FLAG(SA_RESETHAND),
#endif
#ifdef SA_NODEFER
	ADD_SA_FLAG(SA_NODEFER),
#endif
#ifdef SA_RESTART
	ADD_SA_FLAG(SA_RESTART),
#endif
#ifdef SA_ONSTACK
	ADD_SA_FLAG(SA_ONSTACK),
#endif
#ifdef SA_SIGINFO
	ADD_SA_FLAG(SA_SIGINFO),
#endif
	{0, NULL}
};


int
main(int argc, const char* const* argv)
{
	// print pid, process group
	printf("process id:       %d\n", (int)getpid());
	printf("parent id:        %d\n", (int)getppid());
	printf("process group:    %d\n", (int)getpgrp());
	printf("fg process group: %d\n", (int)tcgetpgrp(STDOUT_FILENO));

	// print signal mask
	sigset_t signalMask;
	sigprocmask(SIG_BLOCK, NULL, &signalMask);
	printf("blocked signals:  ");
	bool printedFirst = false;
	for (int i = 0; kSignals[i].name; i++) {
		if (sigismember(&signalMask, kSignals[i].signal)) {
			if (printedFirst) {
				printf(", ");
			} else
				printedFirst = true;
			printf("%s", kSignals[i].name);
		}
	}
	printf("\n");

	// print signal handlers
	printf("signal handlers:\n");
	for (int i = 0; kSignals[i].name; i++) {
		struct sigaction action;
		sigaction(kSignals[i].signal, NULL, &action);

		// signal name
		int signalNameSpacing = 10 - (int)strlen(kSignals[i].name);
		if (signalNameSpacing < 0)
			signalNameSpacing = 0;
		printf("  %s:%*s ", kSignals[i].name, signalNameSpacing, "");

		// signal handler
		if (action.sa_handler == SIG_DFL)
			printf("SIG_DFL");
		else if (action.sa_handler == SIG_IGN)
			printf("SIG_IGN");
		else if (action.sa_handler == SIG_ERR)
			printf("SIG_ERR");
		else
			printf("%p", action.sa_handler);

		printf(" (");

		// flags
		printedFirst = false;
		for (int i = 0; kSigActionFlags[i].name; i++) {
			if (action.sa_flags & kSigActionFlags[i].flag) {
				if (printedFirst) {
					printf(", ");
				} else
					printedFirst = true;
				printf("%s", kSigActionFlags[i].name);
			}
		}
		printf(")\n");
	}
	

	return 0;
}
