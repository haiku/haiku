#include <stdio.h>
#include <kernel/OS.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "sys/select.h"
#include "sys/sockio.h"

#include "ufunc.h"

#define PORT 7772
#define HELLO_MSG "Hello from the server"

int main(int argc, char **argv)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in sin;
	int salen = sizeof(sin);
	int rv, on = 1;
	char buffer[50];
		
	test_banner("Accept Test - Client");

	ioctl(sock, FIONBIO, &on);
		
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = 0;
	sin.sin_len = salen;
	sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	rv = bind(sock, (const struct sockaddr*)&sin, salen);
	if (rv < 0)
		err(rv, "bind");
	printf("Bound\n");
	
	sin.sin_port = htons(PORT);
	
	do {
		rv = connect(sock, (const struct sockaddr*)&sin, salen);
	} while (rv < 0 && errno == EINPROGRESS);
	if (rv < 0 && errno != EISCONN)
		err(errno, "connect");
	printf("connected!\n");
	
	do {
		rv = read(sock, buffer, 50);
	} while (rv < 0 && errno == EWOULDBLOCK);
	if (rv < 0)
		err(rv, "read");
	printf("%s\n", buffer);
	
	close(sock); 

	return (0);
}

	
