#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define panic(str) if (ret != 0) { errno = ret; perror(str); return 1; }


void*
threadFunction(void*)
{
	int i = 0;
	printf("variable addr: %p\n", &i);
	pthread_exit(0);
	return 0;
}


int
main(int argc, char **argv)
{
	int ret;
	pthread_attr_t attr;
	ret = pthread_attr_init(&attr);
	panic("init");
	void* stackAddress;
	size_t stackSize;
	ret = pthread_attr_getstack(&attr, &stackAddress, &stackSize);
	panic("getstack");
	printf("stackAddress: %p, stackSize: %lu\n", stackAddress, stackSize);
	stackSize = PTHREAD_STACK_MIN;
	ret = posix_memalign(&stackAddress, sysconf(_SC_PAGE_SIZE),
		stackSize);
	panic("memalign");
	ret = pthread_attr_setstack(&attr, stackAddress, stackSize);
	panic("setstack");
	ret = pthread_attr_getstack(&attr, &stackAddress, &stackSize);
	panic("getstack");
	printf("stackAddress: %p, stackSize: %lu\n", stackAddress, stackSize);

	pthread_t thread;
	ret = pthread_create(&thread, &attr, threadFunction, NULL);
	panic("create");
	ret = pthread_join(thread, NULL);
	panic("join");

	ret = pthread_attr_destroy(&attr);
	panic("destroy");

	return 0;
}
