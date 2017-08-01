#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include <OS.h>


static status_t
close_fd(void* data)
{
	int fd = *((int*)data);
	snooze(1000000);
	close(fd);
	fprintf(stderr, "fd %d closed\n", fd);
	return B_OK;
}


int
main()
{
	int fd = dup(0);

	thread_id thread = spawn_thread(close_fd, "close fd", B_NORMAL_PRIORITY,
		&fd);
	resume_thread(thread);

	fd_set readSet;
	FD_ZERO(&readSet);
	FD_SET(0, &readSet);
	FD_SET(fd, &readSet);

	fprintf(stderr, "select({0, %d}, NULL, NULL, NULL) ...\n", fd);
	int result = select(fd + 1, &readSet, NULL, NULL, NULL);
	fprintf(stderr, "select(): %d\n", result);

	fprintf(stderr, "fd %d: %s\n", 0, FD_ISSET(0, &readSet) ? "r" : " ");
	fprintf(stderr, "fd %d: %s\n", fd, FD_ISSET(fd, &readSet) ? "r" : " ");

	return 0;
}
