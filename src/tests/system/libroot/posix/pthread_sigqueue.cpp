/*
 * Copyright 2024, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, jerome.duval@gmail.com
 * Inspired from Android signal_test.cpp
 */


#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <unistd.h>


#define ASSERT_EQ(x,y) { if (x != y) { fprintf(stderr, "assert failed %s %s\n", #x, #y); exit(1); }}
#define ASSERT_ERRNO(x) ASSERT_EQ(errno, x)


class ScopedSignalHandler
{
public:
	ScopedSignalHandler(int signal, void (*handler)(int, siginfo_t*, void*))
		: signalNumber(signal)
	{
		sigemptyset(&action.sa_mask);
		action.sa_flags = SA_SIGINFO;
		action.sa_sigaction = handler;
		sigaction(signalNumber, &action, &oldAction);
	}
	~ScopedSignalHandler()
	{
		sigaction(signalNumber, &oldAction, NULL);
	}
private:
	struct sigaction action;
	struct sigaction oldAction;
	const int signalNumber;
};

static int gSigqueueSignalHandlerCallCount = 0;

static void
SigqueueSignalHandler(int signum, siginfo_t* info, void*)
{
	ASSERT_EQ(SIGALRM, signum);
	ASSERT_EQ(SIGALRM, info->si_signo);
	ASSERT_EQ(SI_QUEUE, info->si_code);
	ASSERT_EQ(1, info->si_value.sival_int);
	++gSigqueueSignalHandlerCallCount;
}


void
signal_sigqueue()
{
	gSigqueueSignalHandlerCallCount = 0;
	ScopedSignalHandler ssh(SIGALRM, SigqueueSignalHandler);
	sigval sigval = {.sival_int = 1};
	errno = 0;
	ASSERT_EQ(0, sigqueue(getpid(), SIGALRM, sigval));
	ASSERT_ERRNO(0);
	ASSERT_EQ(1, gSigqueueSignalHandlerCallCount);
}


void
signal_pthread_sigqueue_self()
{
	gSigqueueSignalHandlerCallCount = 0;
	ScopedSignalHandler ssh(SIGALRM, SigqueueSignalHandler);
	sigval sigval = {.sival_int = 1};
	errno = 0;
	ASSERT_EQ(0, pthread_sigqueue(pthread_self(), SIGALRM, sigval));
	ASSERT_ERRNO(0);
	ASSERT_EQ(1, gSigqueueSignalHandlerCallCount);
}


void
signal_pthread_sigqueue_other()
{
	gSigqueueSignalHandlerCallCount = 0;
	ScopedSignalHandler ssh(SIGALRM, SigqueueSignalHandler);
	sigval sigval = {.sival_int = 1};
	sigset_t mask;
	sigfillset(&mask);
	pthread_sigmask(SIG_SETMASK, &mask, nullptr);
	pthread_t thread;
	int rc = pthread_create(&thread, nullptr,
		[](void*) -> void* {
			sigset_t mask;
			sigemptyset(&mask);
			sigsuspend(&mask);
			return nullptr;
		}, nullptr);
	ASSERT_EQ(0, rc);
	errno = 0;
	ASSERT_EQ(0, pthread_sigqueue(thread, SIGALRM, sigval));
	ASSERT_ERRNO(0);
	pthread_join(thread, nullptr);
	ASSERT_EQ(1, gSigqueueSignalHandlerCallCount);
}


extern "C" int
main()
{
	printf("signal_sigqueue\n");
	signal_sigqueue();
	printf("signal_pthread_sigqueue_self\n");
	signal_pthread_sigqueue_self();
	printf("signal_pthread_sigqueue_other\n");
	signal_pthread_sigqueue_other();
}
