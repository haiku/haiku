#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <syscalls.h>
#include <ktypes.h>
#include <resource.h>
#include <Errors.h>
#include <OS.h>
#include <stdlib.h>

#define THREADS    5
#define START_VAL  1

struct the_test {
	int   *current_val;
	int    thread_mult;
	sem_id the_lock;	
};

static int counter_thread(void *data)
{
	struct the_test *tt = (struct the_test*)data;
	free(data);
		
	acquire_sem(tt->the_lock);
	
	*(tt->current_val) *= tt->thread_mult;
	
	release_sem(tt->the_lock);
}

int main(int argc, char **argv)
{
	thread_id t[THREADS];
	sem_id lock;
	int i;
	int expected = START_VAL, current_val = START_VAL;
		
	printf("OpenBeOS Thread Test - Simple\n"
	       "=============================\n");

	lock = create_sem(1, "test_lock");
	
	for (i=0;i<THREADS;i++) {		
		struct the_test *my_test = (struct the_test*)malloc(sizeof(struct the_test));
		my_test->the_lock = lock;
		my_test->current_val = &current_val;
		my_test->thread_mult = i + 1;

		t[i] = spawn_thread(counter_thread, "counter", THREAD_MEDIUM_PRIORITY, my_test);
		if (t[i] > 0) {
			resume_thread(t[i]);
			expected *= (i + 1);
		}
	}
	
	sys_thread_wait_on_thread(t[THREADS - 1], NULL);
	
	printf("Test value = %d vs expected of %d\n", current_val, expected);
	
	delete_sem(lock);
	
	return 0;
}
