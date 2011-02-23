#include <pthread.h>
#include <sched.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


static pthread_attr_t attributes;
static int id[4] = {0, 1, 2, 3};
static pthread_t tid[4];
static bool	state[4];
static bool blocking[4];
static pthread_key_t self;


static void 
suspendLoop(int *i) {
	sigjmp_buf env;
	sigset_t mask;

	sigsetjmp(env, false);

	state[*i] = false;

	sigfillset(&mask);
	sigdelset(&mask, SIGUSR1);
	sigdelset(&mask, SIGTERM);

	while (state[*i] == false && blocking[*i] == false)
		sigsuspend(&mask);

	state[*i] = true;
}


static void 
suspendHandler(int sig) {
	int *i = (int*)pthread_getspecific(self);
	suspendLoop(i);
}


static void 
initialiseSignals()
{
	struct sigaction act;
	sigset_t mask;

	act.sa_handler = suspendHandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGUSR1, &act, NULL);

	sigemptyset(&mask);
	sigaddset(&mask, SIGQUIT);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGPIPE);
	sigprocmask(SIG_BLOCK, &mask, NULL);
}


static void 
self_suspend(int* i)
{
	sigset_t mask;

	blocking[*i] = false;

	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
	pthread_sigmask(SIG_BLOCK, &mask, NULL);

	printf("thread %d suspending\n", *i);

	suspendLoop(i);

	printf("thread %d suspend ended\n", *i);

	pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
}


void *
threadStart(void *arg)
{
	int i = *(int*)arg;
	pthread_setspecific(self, &i);

	state[i] = true;
	blocking[i] = true;

	printf("threadStart(%d)\n", i);
	
	for (int j = 0; j < 10; j++) {
		usleep(1000000);
		self_suspend(&i);
	}

	printf("quitting %d\n", i);
	return NULL;
}


/*void 
suspendAllThreads()
{
	for (int i = 0; i < 4; i++) {
		blocking[i] = false;
		if (state[i] == true)
			pthread_kill(tid[i], SIGUSR1);
	}

	for (int i = 0; i < 4; i++) {
		while(state[i] == true) {
			sched_yield();
		}
	}
}*/



void
resumeAllThreads()
{
	for (int i = 0; i < 4; i++) {
		blocking[i] = true;
		if (state[i] == false) {
			printf("thread %d signaled for resume\n", i);
			pthread_kill(tid[i], SIGUSR1);
		}
	}

	int t = 50;
	for (int i = 0; i < 4; i++) {
		while(state[i] == false && t-- > 0) {
			printf("thread %d still suspended, yielding\n", i);
			sched_yield();
		}
	}
}


int
main(int argc, char **argv)
{
	initialiseSignals();

	pthread_key_create(&self, NULL);

	pthread_attr_init(&attributes);
	pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);

	for (int i = 0; i < 4; i++) {
		if (pthread_create(&tid[i], &attributes, threadStart, &id[i]) != 0)
			fprintf(stderr, "couldn't create thread %d\n", i);
		printf("thread %d created\n", i);
	}
	
	/*suspendAllThreads();*/
	printf("snoozing\n");
	usleep(3000000);
	printf("resuming all threads\n");
	resumeAllThreads();
	printf("resuming all threads done\n");
}

