#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdbool.h>

int main() {
	int fds[2];
	int domain;
	domain = AF_UNIX;
        // domain = AF_INET; // works
	printf("Domain: %i\n", domain);
	int ret = socketpair(domain, SOCK_DGRAM, 0, fds); // try also: SOCK_STREAM
	if(ret) {
		perror("Could not get socketpair");
		return 1;
	}

	/*
	struct timeval v = {
		.tv_sec = 1,
		.tv_usec = 0
	};
	ret = setsockopt(fds[0], SOL_SOCKET, SO_RCVTIMEO, &v, sizeof(v));
	if(ret) {
		perror("setsockopt");
	}
	*/

	size_t bufLen = 1024;
	char *buf = calloc(bufLen, 1);
	int ok = 0;
	while(true) {
		printf("recv %i\n", ok);
		ret = recv(fds[0], &buf[0], bufLen, MSG_DONTWAIT);
		// expected: EWOULDBLOCK/EAGAIN (on Linux, macOS, Haiku)
		printf("%i\n", ret);
		if(ret < 0) {
			perror("recv");
			break;
		} else {
			ok++;
		}
	}

	return 0;
}
