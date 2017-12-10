#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <OS.h>


uint32 gStackPointer;

#define CHECK_STACK_ALIGN(from) \
	asm volatile ("mov %%esp, %0" : "=r" (gStackPointer) :); \
	if (gStackPointer & 0xF) \
		printf("In %s, stack is NOT aligned: %lx\n", from, gStackPointer); \
	else \
		printf("In %s, stack is aligned!\n", from);

int function(void)
{
	CHECK_STACK_ALIGN("function");

	return 0;
}

status_t thread(void* arg)
{
	CHECK_STACK_ALIGN("thread");

	return B_OK;
}

void handler(int param)
{
	CHECK_STACK_ALIGN("signal");
}

int main(void)
{
	CHECK_STACK_ALIGN("main");

	// Test from called function
	function();

	// Test from thread
	{
		status_t rv;
		wait_for_thread(spawn_thread(thread, "test", B_NORMAL_PRIORITY, NULL), &rv);
	}

	// Test from signal handler
	{
		stack_t signalStack;
		struct sigaction action;

		signalStack.ss_sp = malloc(SIGSTKSZ);
		signalStack.ss_flags = 0;
		signalStack.ss_size = SIGSTKSZ;
		sigaltstack(&signalStack, NULL);

		action.sa_handler = handler;
		action.sa_mask = 0;
		action.sa_flags = SA_ONSTACK;
		sigaction(SIGUSR1, &action, NULL);

		kill(getpid(), SIGUSR1);
	}

	return 0;
}

struct Foo {
	Foo()
	{
		CHECK_STACK_ALIGN("init");
	}

	~Foo()
	{
		CHECK_STACK_ALIGN("fini");
	}
};

Foo init;
