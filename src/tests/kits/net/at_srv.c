#include <stdio.h>
#include <kernel/OS.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "sys/select.h"

// #include "ufunc.h"

#define PORT 7772
#define HELLO_MSG "Hello from the server\n"
#define RESPONSE_MSG "Hello from the client!\n"

int main(int argc, char **argv)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in sin;
	int salen = sizeof(sin);
	int rv, on = 1;
	int newsock;
	char buffer[50];
		
	// test_banner("Accept Test - Server");
	
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);
	sin.sin_len = salen;
	sin.sin_addr.s_addr = htonl(INADDR_ANY); // LOOPBACK);
	rv = bind(sock, (const struct sockaddr*)&sin, salen);
	if (rv < 0)
		perror("bind");
	printf("Bound\n");
	
	rv = listen(sock, 5);
	if (rv < 0)
		perror("listen");
	printf("Listening\n");

	newsock = accept(sock, (struct sockaddr*)&sin, &salen);
	if (newsock < 0)
		perror("accept");
	printf("Accepted socket %d\n", newsock);
	
	rv = write(newsock, HELLO_MSG, strlen(HELLO_MSG));
	if (rv < 0)
		perror("write");
	printf("Written hello\n");
	
	close(newsock);
	close(sock); 

	return (0);
}

	
