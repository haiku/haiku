#include <stdio.h>
#include <kernel/OS.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "sys/select.h"

#include "ufunc.h"

#define RQST "GET / HTTP/1.0\n\n"
#define BUFFER 1024

int main(int argc, char **argv)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in sin;
	size_t salen = sizeof(sin);
	int rv;
	char buffer[BUFFER];
	
	test_banner("Basic TCP Test");
	
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons((real_time_clock() & 0xffff));
	sin.sin_len = salen;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	rv = bind(sock, (const struct sockaddr*)&sin, salen);
	if (rv < 0)
		err(rv, "bind");
	printf("Bound\n");
	
	sin.sin_port = htons(80);
	sin.sin_addr.s_addr = htonl(0xc0a80001);
	rv = connect(sock, (const struct sockaddr*)&sin, salen);
	if (rv < 0) {
		close(sock);
		err(rv, "connect");
	}
	printf("Connected!\n");

	rv = write(sock, RQST, strlen(RQST));
	if (rv < 0) {
		close(sock);
		err(errno, "write");
	}
	printf("Written request %d\n", rv);
	
	memset(&buffer,0, BUFFER);	
	rv = read(sock, buffer, BUFFER);
	if (rv < 0) {
		close(sock);
		err(errno, "read");
	}
	printf("%s", buffer);
	memset(&buffer, 0, BUFFER);
	while ((rv = read(sock, buffer, BUFFER)) >= 0) {
		printf("%s", buffer);
		if (rv <= 0)
			break;
		memset(&buffer,0, BUFFER);	
	}
	printf("Done.\n");
	close(sock); 

	return (0);
}

	
