#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdbool.h>

int main() {
	int fds[2];
	int domain;
	domain = AF_UNIX;
 // domain = AF_INET;
	printf("Domain: %i\n", domain);
	int ret = socketpair(domain, SOCK_DGRAM, 0, fds); // try also: SOCK_STREAM
	if(ret) {
		perror("Could not get socketpair");
		return 1;
	}

	struct timeval v = {
		.tv_sec = 1,
		.tv_usec = 0
	};

	// uncomment to allow send to timeout with ETIMEDOUT after 1 second
	//ret = setsockopt(fds[0], SOL_SOCKET, SO_SNDTIMEO, &v, sizeof(v));
	if(ret) {
		perror("setsockopt");
	}

	size_t bufLen = 1024;
	char *buf = calloc(bufLen, 1);
	int ok = 0;
	while(true) {
		printf("send %i\n", ok);
		ret = send(fds[0], &buf[0], bufLen, MSG_DONTWAIT);
		// eventually: EAGAIN (on Linux and Haiku), ENOBUFS (on macOS)
		if(ret < 0) {
			perror("send");
			break;
		} else {
			ok++;
		}
	}

	return 0;
}
