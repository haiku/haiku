#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <OS.h>

#include "user_thread.h"

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

	struct user_thread *t = get_user_thread();
	printf("defer_signals: %" B_PRId32 "\n", t->defer_signals);

	return 0;
}
