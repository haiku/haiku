/*
** Small test for signals handling
*/


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <syscalls.h>


void sig_handler(int);
void install_handler(int);
int32 thread(void *);


void
sig_handler(int sig)
{
	int i;
	
	printf("sig_test (sig_handler): Received signal #%d...\n", sig);
	printf("Counting for fun: ");
	for (i=0; i<10; i++)
		printf("%d ", i);
	printf("\n");
}


void
install_handler(int sig)
{
	struct sigaction newa;
	struct sigaction olda;
	
	memset(&newa, 0, sizeof(newa));
	
	newa.sa_handler = sig_handler;
	newa.sa_flags = SA_NOMASK | SA_RESTART;
	
	if (sys_sigaction(sig, &newa, &olda) < 0) {
		printf("Failed installing handler for sig #%d!\n", sig);
	}
}


int32
thread(void *data)
{
	printf("Started snoozing thread...\n");
	while (1) {
		if (snooze(1000000000L) == B_INTERRUPTED)
			printf("sig_test (thread): snooze was interrupted!\n");
	}
	return 0;
}


int
main(int argc, char **argv)
{
	int i;
	
	printf("Installing signal handlers...\n");
	for (i=1; i<=NSIG; i++)
		install_handler(i);
	printf("Spawning some threads...\n");
	for (i=0; i<5; i++)
		resume_thread(spawn_thread(thread, "sig_test aux thread", B_NORMAL_PRIORITY, NULL));
	printf("Done. Entering sleep mode...\n");
	while (1) {
		if (sys_snooze(1000000000L) == B_INTERRUPTED)
			// this never gets called as the syscall is always restarted
			printf("sig_test (main): snooze was interrupted!\n");
	}
	
	return 0;
}

