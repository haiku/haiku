#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>


int
main(int argc, const char* const* argv)
{
	// create the listener socket
	int listenerSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenerSocket < 0) {
		fprintf(stderr, "failed to create listener socket: %s\n",
			strerror(errno));
		exit(1);
	}

	// bind it to the port
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = 0;
	socklen_t addrLen = sizeof(addr);
	if (bind(listenerSocket, (sockaddr*)&addr, addrLen) < 0) {
		fprintf(stderr, "failed to bind listener socket: %s\n",
			strerror(errno));
		exit(1);
	}

 	memset(&addr, 0, sizeof(addr));
 	addrLen = sizeof(addr);
 	if (getsockname(listenerSocket, (sockaddr*)&addr, &addrLen) != 0) {
 		fprintf(stderr, "failed to get socket name: %s\n", strerror(errno));
 		exit(1);
 	}

	printf("listener port: %d (%x)\n", addr.sin_port, addr.sin_addr.s_addr);

	// start listening
	if (listen(listenerSocket, 1) < 0) {
		return -1;
	}

	// fork client
	pid_t child = fork();
	if (child < 0) {
		fprintf(stderr, "fork() failed: %s\n", strerror(errno));
		exit(1);
	}

	if (child == 0) {
		// child
		unsigned short previousPort = 0;

		for (int i = 0; ; i++) {
			// create the client socket
			int fd = socket(AF_INET, SOCK_STREAM, 0);
			if (fd < 0) {
				fprintf(stderr, "child: %d: failed to create client socket: "
					"%s\n", i, strerror(errno));
				exit(1);
			}

/*
			// test binding
			if (previousPort != 0) {
				sockaddr_in clientAddr;
				clientAddr.sin_family = AF_INET;
				clientAddr.sin_addr.s_addr = INADDR_ANY;
//				clientAddr.sin_addr.s_addr = INADDR_LOOPBACK;
//				clientAddr.sin_port = htons(previousPort);
//				clientAddr.sin_port = htons(previousPort + 1);
				clientAddr.sin_port = htons(25432);
				if (bind(fd, (sockaddr*)&clientAddr, sizeof(clientAddr)) < 0) {
					fprintf(stderr, "child: %d: failed to bind: %s\n",
						i, strerror(errno));
				}
			}
*/

			// connect to server
			if (connect(fd, (sockaddr*)&addr, addrLen) == 0) {
				sockaddr_in serverAddr;
				socklen_t serverAddrLen = sizeof(serverAddr);
				sockaddr_in clientAddr;
				socklen_t clientAddrLen = sizeof(clientAddr);
				if (getsockname(fd, (sockaddr*)&clientAddr, &clientAddrLen)
						== 0
					&& getpeername(fd, (sockaddr*)&serverAddr, &serverAddrLen)
						== 0) {
					previousPort = ntohs(clientAddr.sin_port);
					printf("child: %d: connected to server: 0x%08x:%u, "
						"local port: %u\n", i,
						(unsigned)ntohl(serverAddr.sin_addr.s_addr),
						ntohs(serverAddr.sin_port),
						previousPort);
				} else {
					fprintf(stderr, "child: %d: failed to get socket name %s\n",
						i, strerror(errno));
				}
			} else {
				fprintf(stderr, "child: %d: connect() failed %s\n", i,
					strerror(errno));
			}

			// wait for the other end to be closed
			char buffer[1];
			if (fd >= 0) {
				read(fd, buffer, sizeof(buffer));
				close(fd);
			}

//			close(fd);
//			sleep(1);
		}
	} else {
		// parent
		while (true) {
			// accept a socket
			int fd = accept(listenerSocket, NULL, 0);
			if (fd < 0) {
				fprintf(stderr, "server: accept() failed: %s\n",
					strerror(errno));
				exit(1);
			}

			// wait for it to be closed
//			char buffer[1];
//			read(fd, buffer, sizeof(buffer));
//			close(fd);

			sleep(1);
			close(fd);
		}
	}

	printf("Everything went fine.\n");
	
	return 0;
}
