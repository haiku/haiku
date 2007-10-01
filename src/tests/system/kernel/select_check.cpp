#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>


int
main(int argc, const char* const* argv)
{
	const char* config[] = {
		argc >= 2 ? argv[1] : "rwe",
		argc >= 3 ? argv[2] : "rwe",
		argc >= 4 ? argv[3] : "rwe"
	};

	fd_set readSet;
	fd_set writeSet;
	fd_set errorSet;

	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);
	FD_ZERO(&errorSet);

	for (int fd = 0; fd < 3; fd++) {
		if (strchr(config[fd], 'r'))
			FD_SET(fd, &readSet);
		if (strchr(config[fd], 'w'))
			FD_SET(fd, &writeSet);
		if (strchr(config[fd], 'e'))
			FD_SET(fd, &errorSet);
	}

	int result = select(3, &readSet, &writeSet, &errorSet, NULL);
	fprintf(stderr, "select(): %d\n", result);

	for (int fd = 0; fd < 3; fd++) {
		fprintf(stderr, "fd %d: %s%s%s\n", fd,
			FD_ISSET(fd, &readSet) ? "r" : " ",
			FD_ISSET(fd, &writeSet) ? "w" : " ",
			FD_ISSET(fd, &errorSet) ? "e" : " ");
	}

	return 0;
}
