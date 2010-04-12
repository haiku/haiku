#include <unistd.h>
#include <memory.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

const unsigned short TEST_PORT = 40000;

void usage() {
	printf("client [tcp|udp] [4|6] [4|6]\n");
	exit(1);
}

int main(int argc, char *argv[]) {
	int socketType = SOCK_DGRAM;
	int socketFamily1 = AF_INET;
	int socketFamily2 = AF_INET;

	if (argc > 1) {
		if (!strcmp(argv[1], "tcp")) {
			socketType = SOCK_STREAM;
		} else if (!strcmp(argv[1], "udp")) {
			socketType = SOCK_DGRAM;
		} else {
			usage();
		}
	}
	if (argc > 2) {
		switch (atoi(argv[2])) {
		case 4:
			socketFamily1 = AF_INET;
			break;
		case 6:
			socketFamily1 = AF_INET6;
			break;
		default:
			usage();
		}
	}
	if (argc > 3) {
		switch (atoi(argv[3])) {
		case 4:
			socketFamily2 = AF_INET;
			break;
		case 6:
			socketFamily2 = AF_INET6;
			break;
		default:
			usage();
		}
	}

	int fd = socket(socketFamily1, socketType, 0);
	if (fd < 0) {
		perror("socket");
		return -1;
	}

	sockaddr_storage saddr;
	memset(&saddr, 0, sizeof(saddr));
	saddr.ss_family = socketFamily2;
	((sockaddr_in *) &saddr)->sin_port = htons(TEST_PORT);
	if (connect(fd, (sockaddr *) &saddr, socketFamily2 == AF_INET ?
				sizeof(sockaddr_in) : sizeof(sockaddr_in6)) < 0) {
		perror("connect");
		close(fd);
		return -1;
	}

	const char *buffer = "hello world";
	unsigned len = strlen(buffer);
	int ret = send(fd, buffer, len, 0);
	if (ret < len) {
		if (ret < 0) {
			perror("send");
		} else if (ret == 0) {
			printf("no data sent!\n");
		} else {
			printf("not all data sent!\n");
		}
	} else {
		printf("send(): success\n");
	}
	close(fd);
	return 0;
}
