/*
 * Copyright 2021, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <thread>

#define CYCLES 2


class MyClass
{
public:
	MyClass() { i=0; printf("MyClass() called() %p\n", this); }
	~MyClass() { printf("~MyClass() called() %d %p\n", i, this); }

	void Touch() { i++; printf("Touch() called() %d %p\n", i, this); }

private:
	int j;
	int i;
};

thread_local MyClass myClass1;
thread_local MyClass myClass2;

void* threadFn(void* id_ptr)
{
	int thread_id = *(int*)id_ptr;

	for (int i = 0; i < CYCLES; ++i) {
		int wait_sec = 1;
		sleep(wait_sec);
		myClass1.Touch();
		myClass2.Touch();
	}

	return NULL;
}


int thread_local_test(int count)
{
	pthread_t ids[count];
	int short_ids[count];

	srand(time(NULL));

	for (int i = 0; i < count; i++) {
		short_ids[i] = i;
		pthread_create(&ids[i], NULL, threadFn, &short_ids[i]);
	}

	for (int i = 0; i < count; i++)
		pthread_join(ids[i], NULL);

	return 0;
}


int main()
{
	thread_local_test(1);
}
