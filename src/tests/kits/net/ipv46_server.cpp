#include <unistd.h>
#include <memory.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

const unsigned short TEST_PORT = 40000;

void usage() {
	printf("server [tcp|udp] [4|6] [local-address]\n");
	exit(1);
}

void recvLoop(int fd) {
	for (;;)	{
		char buffer[1000];
		int ret = recv(fd, buffer, sizeof(buffer) - 1, 0);
		if (ret < 0) {
			perror("recv");
			exit(-1);
		}
		if (ret == 0) {
			printf("received EOF!\n");
			break;
		} else {
			buffer[ret] = 0;
			printf("received %d bytes: \"%s\"\n", ret, buffer);
		}
	}
}

int main(int argc, char *argv[]) {
	int socketType = SOCK_DGRAM;
	int socketFamily = AF_INET;
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
			socketFamily = AF_INET;
			break;
		case 6:
			socketFamily = AF_INET6;
			break;
		default:
			usage();
		}
	}

	sockaddr_storage localAddress;
	memset(&localAddress, 0, sizeof(localAddress));
	localAddress.ss_family = socketFamily;
	((sockaddr_in *) &localAddress)->sin_port = htons(TEST_PORT);

	if (argc > 3) {
		do {
			void *dstBuffer = &((sockaddr_in *) &localAddress)->sin_addr;
			if (inet_pton(AF_INET, argv[3], dstBuffer) == 1) {
				printf("using IPv4 local address\n");
				localAddress.ss_family = AF_INET;
				break;
			}

			dstBuffer = &((sockaddr_in6 *) &localAddress)->sin6_addr;
			if (inet_pton(AF_INET6, argv[3], dstBuffer) == 1) {
				printf("using IPv6 local address\n");
				localAddress.ss_family = AF_INET6;
				break;
			}

			usage();
		} while (false);
	}

	int fd = socket(socketFamily, socketType, 0);
	if (fd < 0) {
		perror("socket");
		return -1;
	}

	if (bind(fd, (sockaddr *)&localAddress, localAddress.ss_family == AF_INET ?
			 sizeof(sockaddr_in) : sizeof(sockaddr_in6)) < 0) {
		perror("bind");
		return -1;
	}

	switch (socketType) {
	case SOCK_DGRAM:
		for (;;) {
			recvLoop(fd);
		}
		break;
	case SOCK_STREAM:
		if (listen(fd, 5) < 0) {
			perror("listen");
			return 1;
		}
		for (;;) {
			int clientfd = accept(fd, NULL, 0);
			if (clientfd < 0) {
				perror("accept");
				return 1;
			}
			printf("TCP server: got some client!\n");
			if (fork() != 0) {
				// parent code
				close(clientfd);
				continue;
			}
			// child code
			close(fd);
			recvLoop(clientfd);
			exit(0);
		}
		break;
	}
}
