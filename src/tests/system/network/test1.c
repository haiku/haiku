#include <stdio.h>
#include <kernel/OS.h>
#include <string.h>
#include <sys/time.h>

#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "sys/select.h"

#define THREADS	2
#define TIME	10

int32 test_thread(void *data)
{
	int tnum = *(int*)data;
	int sock = 0;
	uint32 num = 0;
	bigtime_t tn;

	printf("Thread %d, starting test...\n", tnum + 1);

	tn = real_time_clock();

	while (real_time_clock() - tn <= TIME) {
		sock = socket(AF_INET, SOCK_DGRAM , 0);
		if (sock < 0) {
			printf("Failed! Socket could not be created.\n");
			printf("Error was %d [%s]\n", sock, strerror(sock));
			printf("This was after I had created %ld socket%s\n",
				num, num == 1 ? "" : "s");
			return -1;
		}
		closesocket(sock);
		num++;
	}

	printf( "Thread %d:\n"
		"       sockets created : %5ld\n"
		"       test time       : %5d seconds\n"
		"       average         : %5ld sockets/sec\n",
		tnum + 1, num, TIME, num / TIME);
}

#define TEST_PHRASE "Hello loopback!"
	
int main(int argc, char **argv)
{
	thread_id t[THREADS];
	int i;
	status_t retval;
		
	for (i=0;i<THREADS;i++) {
		t[i] = spawn_thread(test_thread, "socket test thread", 
			B_NORMAL_PRIORITY, &i);
		if (t[i] >= 0)
			resume_thread(t[i]);
	}

	for (i=0;i<THREADS;i++) {
		wait_for_thread(t[i], &retval);
	}

	printf("Test complete.\n");

	return (0);
}

	
