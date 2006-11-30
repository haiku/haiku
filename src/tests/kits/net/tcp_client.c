/*
 * a stream socket client demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>


#define PORT 1234		// the port client will be connecting to 
#define MAXDATASIZE 100	// max number of bytes we can get at once 


int
main(int argc, char **argv)
{
	int sockfd;
	char buffer[MAXDATASIZE];
	short int port = PORT;
	struct hostent *he;
	struct sockaddr_in their_addr;
		// connector's address information 

	if (argc < 2) {
		fprintf(stderr,"usage: tcp_client <hostname> [port]\n");
		exit(1);
	}

	if (argc == 3)
		port = atoi(argv[2]);

	if ((he = gethostbyname(argv[1])) == NULL) {
		// get the host info 
		perror("gethostbyname");
		exit(1);
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	memset(&their_addr, 0, sizeof(their_addr));
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(port);
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);

	if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
		perror("connect");
		exit(1);
	}

	if (!fork()) {
		int numBytes;

		while (1) {
			// child process
			if ((numBytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) == -1) {
				perror("recv");
				sleep(1);
				// want the read thread to stay alive
				continue;
			}

			buffer[numBytes] = '\0';
			printf("%s", buffer);
		}
	} else {
		while (1) {
			// parent process
			if (fgets(buffer, sizeof(buffer) - 1, stdin) == NULL) {
				perror("fgets");
				exit(1);
			}

			if ((send(sockfd, buffer, strlen(buffer), 0)) == -1) {
				perror("send");
				exit(1);
			}
		}
	}

	close(sockfd);

	return 0;
}

