/*
 * a stream socket server demo
 */


#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


#define MYPORT 1234		// the port users will be connecting to
#define BACKLOG 10		// how many pending connections queue will hold
#define MAXDATASIZE	1065537


static void
sigchld_handler(int s)
{
	while (waitpid(-1, NULL, WNOHANG) > 0) {
	}
}


int
main(int argc, char *argv[])
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct sockaddr_in my_addr;	// my address information
	struct sockaddr_in their_addr; // connector's address information
	uint32_t sin_size;
	struct sigaction sa;
	int yes = 1;
	short int port = MYPORT;
	char buf[MAXDATASIZE];

	if (argc >= 2 && atoi(argv[1]) != 0)
		port = atoi(argv[1]);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP

	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
		perror("bind");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	sa.sa_flags = SA_RESTART;
#else
	sa.sa_flags = 0;
#endif
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	while (1) {
		// main accept() loop
		sin_size = sizeof(struct sockaddr_in);
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
			perror("accept");
			continue;
		}
		printf("server: got connection from %s\n", inet_ntoa(their_addr.sin_addr));
		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener

			if (!fork()) {
				while (1) {
					// child's child process
					if (fgets(buf, MAXDATASIZE, stdin) == NULL) {
						perror("fgets");
						exit(1);
					}

					if (!strcmp(buf, "full\n")) {
						int i;
puts("HY");
						for (i = 0; i < MAXDATASIZE - 2; i++) {
							buf[i] = 'a' + (i % 26);
						}
						buf[MAXDATASIZE - 2] = '\n';
						buf[MAXDATASIZE - 1] = '\0';
					}

					if (send(new_fd, buf, strlen(buf), 0) == -1) {
						perror("send");
						exit(1);
					}
				}
			} else {
				ssize_t numBytes;
				while (1) {
					// child process
					if ((numBytes = recv(new_fd, buf, MAXDATASIZE, 0)) == -1) {
						perror("recv");
						exit(1);
					}
					if (numBytes == 0)
						exit(0);

					buf[numBytes] = '\0';

					printf("%s:| %s", inet_ntoa(their_addr.sin_addr), buf);
				}
			}

			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

