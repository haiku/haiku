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

#define THREADS	2
#define MIN_SOCK	10
#define MAX_SOCK	100
#define TIME	10

int32 test_thread(void *data)
{
	int tnum = *(int*)data;
	int sock[MAX_SOCK];
	int qty = MIN_SOCK;
	struct sockaddr_in sa;
	int i, rv, totsock = 0;

	sa.sin_family = AF_INET;
	sa.sin_port = 0;
	sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sa.sin_len = sizeof(sa);
	memset(&sa.sin_zero, 0, sizeof(sa.sin_zero));
	
	printf("Thread %d, starting test\n", tnum + 1);

	while (qty <= MAX_SOCK) {
		for (i=0;i < qty;i++) {
			if (i >= MAX_SOCK)
				break;
			sock[i] = socket(AF_INET, SOCK_DGRAM , 0);
			totsock++;
			if (sock[i] < 0) {
				printf("Total of %d sockets created\n", totsock);
				err(errno, "Socket creation failed");
			}
		}
		printf("Thread %d: completed creating %d sockets...\n", tnum+1, qty);
		for (i=0;i < qty;i++) {
			if (i >= MAX_SOCK)
				break;
			rv = bind(sock[i],	(struct sockaddr*)&sa, sizeof(sa));
			if (rv < 0)
				err(errno, "Failed to bind!");
		}	
		for (i=0;i < qty;i++) {
			if (i >= MAX_SOCK)
				break;
			rv = close(sock[i]);
			if (rv < 0)
				err(errno, "Failed to close socket!");
		}
		qty += (MAX_SOCK - MIN_SOCK) / 10;
	}

	printf( "Thread %d complete\n", tnum);
}
	
int main(int argc, char **argv)
{
	thread_id t[THREADS];
	int i;
	status_t retval;

	test_banner("Simultaneous socket creation test");
			
	for (i=0;i<THREADS;i++) {
		t[i] = spawn_thread(test_thread, "socket test thread", 
			B_NORMAL_PRIORITY, &i);
		if (t[i] >= 0)
			resume_thread(t[i]);
	}

	for (i=0;i<THREADS;i++) {
		wait_for_thread(t[i], &retval);
	}

	return (0);
}

	
