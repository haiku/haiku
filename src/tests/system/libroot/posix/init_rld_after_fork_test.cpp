#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <OS.h>


static void
list_semaphores(const char* process)
{
	printf("%s (%ld) semaphores:\n", process, find_thread(NULL));

	sem_info semInfo;
	int32 cookie = 0;
	while (get_next_sem_info(B_CURRENT_TEAM, &cookie, &semInfo) == B_OK)
		printf("  %9ld  %s\n", semInfo.sem, semInfo.name);
}


int
main()
{
	pid_t child = fork();
	if (child < 0) {
		fprintf(stderr, "Error: fork() failed: %s\n", strerror(errno));
		exit(1);
	}

	if (child > 0) {
		// the parent process -- wait for the child to finish
		status_t result;
		wait_for_thread(child, &result);
	}

	list_semaphores(child == 0 ? "child" : "parent");

	return 0;
}