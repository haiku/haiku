#include <stdio.h>
#include <kernel/OS.h>
#include <string.h>
#include <sys/time.h>
#include <malloc.h>

#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "sys/select.h"

#include "ufunc.h"

#define THREADS	2
#define TIME	10


int32 test_thread(void *data)
{
	int tnum = *(int*)data;
	int sock = 0;
	uint32 num = 0;
	int rv;
	struct sockaddr_in sa;
	bigtime_t tn;

	sa.sin_len = sizeof(sa);
	sa.sin_port = 0;
	sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sa.sin_family = AF_INET;
	memset(&sa.sin_zero, 0, sizeof(sa.sin_zero));
	
	printf("Thread %d, starting test...\n", tnum + 1);

	tn = real_time_clock();

	while (real_time_clock() - tn <= TIME) {
		sock = socket(AF_INET, SOCK_DGRAM , 0);
		if (sock < 0)
			err(sock, "Socket couldn't be created");
		rv = bind(sock, (struct sockaddr *)&sa, sizeof(sa));
		if (rv < 0)
			err(rv, "Socket could not be bound to an ephemereal port");
		closesocket(sock);
		num++;
	}

	printf( "Thread %d:\n"
		"       sockets created : %5ld\n"
		"       test time       : %5d seconds\n"
		"       average         : %5ld sockets/sec\n",
		tnum + 1, num, TIME, num / TIME);
}

int main(int argc, char **argv)
{
	thread_id t[THREADS];
	int i;
	status_t retval;

	test_banner("Socket creation and bind() test");
	
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

	
