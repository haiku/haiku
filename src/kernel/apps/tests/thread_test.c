#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <syscalls.h>
#include <ktypes.h>
#include <sys/resource.h>
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

static int32 counter_thread(void *data)
{
	struct the_test *tt = (struct the_test*)data;
		
	acquire_sem(tt->the_lock);
	
	*(tt->current_val) *= tt->thread_mult;
	
	release_sem(tt->the_lock);
	
	free(data);
	return 0;
}


static int32 priority_test(void *data)
{
	int i;
	
	for (i=0; i<6; i++) {
		printf("%s: %d\n", (char *)data, i);
//		sys_snooze(1000);
	}
	return 0;
}


static int32 communication_test(void *data)
{
	int times = (int)data;
	int i, code;
	char buffer[1024];
	thread_id sender;
	
	for (i=0; i<times; i++) {
		code = receive_data(&sender, (void *)buffer, sizeof(buffer));
		printf(" -> received (%d): \"%s\"\n", code, buffer);
	}
	return 0;
}


int main(int argc, char **argv)
{
	thread_id t[THREADS];
	sem_id lock;
	int i;
	int expected = START_VAL, current_val = START_VAL;
	char *comm_test[] = {	"This is a test",
							"of the send_data",
							"and receive_data",
							"syscalls implementation",
							"for OpenBeOS"	};
		
	printf("OpenBeOS Thread Test - Simple\n"
	       "=============================\n");

	lock = create_sem(1, "test_lock");
	
	for (i=0;i<THREADS;i++) {		
		struct the_test *my_test = (struct the_test*)malloc(sizeof(struct the_test));
		my_test->the_lock = lock;
		my_test->current_val = &current_val;
		my_test->thread_mult = i + 1;

		t[i] = spawn_thread(counter_thread, "counter", B_NORMAL_PRIORITY, my_test);
		if (t[i] > 0) {
			resume_thread(t[i]);
			expected *= (i + 1);
		}
	}
	
	sys_wait_on_thread(t[THREADS - 1], NULL);
	
	printf("Test value = %d vs expected of %d\n", current_val, expected);
	
	delete_sem(lock);
	
	t[0] = spawn_thread(priority_test, "thread0", 11, "thread0");
	t[1] = spawn_thread(priority_test, "thread1", 12, "thread1");
	t[2] = spawn_thread(priority_test, "thread2", 13, "thread2");
	resume_thread(t[0]);
	resume_thread(t[1]);
	resume_thread(t[2]);
	
	printf("Snoozing...\n");
	snooze(100000);
	printf("Waiting for threads...");
	sys_wait_on_thread(t[0], NULL);
	printf("1, ");
	sys_wait_on_thread(t[1], NULL);
	printf("2, ");
	sys_wait_on_thread(t[2], NULL);
	
	printf("3.\nDone. Spawning commthread...\n");
	t[0] = spawn_thread(communication_test, "commthread", B_NORMAL_PRIORITY, (void *)5);
	printf("Spawned. Starting communication...\n");
	resume_thread(t[0]);
	
	for (i=0; i<5; i++) {
		printf("sending");
		send_data(t[0], i, comm_test[i], strlen(comm_test[i]) + 1);
		// Give time to the commthread to display info
		snooze(10000);
	}
	sys_wait_on_thread(t[0], NULL);
	
	return 0;
}
