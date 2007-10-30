#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>


void
handler(int signal)
{
    printf( "inside handler()\n" );
}


int
main(int argc, char* argv[])
{
	struct sigaction signalAction;
	sigset_t blockedSignalSet;

	sigfillset(&blockedSignalSet);
	sigdelset(&blockedSignalSet, SIGALRM);

	sigemptyset(&signalAction.sa_mask);
	signalAction.sa_flags = 0;
	signalAction.sa_handler = handler;
	sigaction(SIGALRM, &signalAction, NULL);

    fprintf(stdout, "before sigsuspend()\n");
    alarm(2);
    sigsuspend(&blockedSignalSet);
    fprintf(stdout, "after sigsuspend()\n");

    return 0;
}
