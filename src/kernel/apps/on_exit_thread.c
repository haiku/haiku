#include <OS.h>
#include <stdio.h>


static void
first(void *data)
{
	printf("first on_exit_thread(): %ld\n", (int32)data);
}


static void
second(void *data)
{
	printf("second on_exit_thread(): %ld\n", (int32)data);
}


static int32
thread(void *data)
{
	puts("Nurse Stimpy for the rescue!");
	
	on_exit_thread(first, (void *)11);
	on_exit_thread(second, (void *)22);

	return 0;
}


int
main(int argc, char **argv)
{
	status_t status;
	thread_id id;

	on_exit_thread(first, (void *)666);
	id = spawn_thread(thread, "mythread", B_NORMAL_PRIORITY, (void *)42);
	resume_thread(id);

	wait_for_thread(id, &status);
	return 0;
}
