#include <stdio.h>
#include <poll.h>

int main() {
	FILE* f = popen("/bin/bash -c 'for i in 1 2 3; do { echo $i; sleep 1; }; done'", "r");
	printf("f=%p\n", f);
	int fd = fileno(f);
	printf("fd=%d\n", fd);

	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN | POLLRDBAND;

	char buffer[80];

	while (1) {
		int rv = poll(&pfd, 1, 500);
		printf("rv=%d\n", rv);
		if (rv == 0)
			continue;
		if (rv < 0)
			break;
		printf("events=%08x revents=%08x\n", pfd.events, pfd.revents);
		if ((pfd.events & pfd.revents) == 0)
			break;

		fgets(buffer, 79, f);
		printf("output: %s", buffer);
	}

	return 0;
}
