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

int main(int argc, char **argv)
{
	int s;
	int rv;
	struct fd_set fdr, fdw, fde;
	struct timeval tv;
	int32 rtc;
	
	test_banner("Select test");
			
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		err(s, "Socket creation failed");

	FD_ZERO(&fdr);
	FD_SET(s, &fdr);
	FD_ZERO(&fdw);
	FD_SET(s, &fdw);
	FD_ZERO(&fde);
	FD_SET(s, &fde);
	
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	
	printf("Trying with timeval (5 secs)...\n");
	rtc = real_time_clock();
	rv = select(s + 1, &fdr, NULL, &fde, &tv);
	rtc = real_time_clock() - rtc;
	printf("select gave %d in %ld seconds\n", rv, rtc);
	if (rv < 0)
		printf("errno = %d [%s]\n", errno, strerror(errno));

	printf("resetting select fd's\n");
	FD_ZERO(&fdr);
	FD_SET(s, &fdr);
	FD_ZERO(&fdw);
	FD_SET(s, &fdw);
	FD_ZERO(&fde);
	FD_SET(s, &fde);
	
	printf("Trying without timeval (= NULL)\n");
	rv = select(s +1, &fdr, &fdw, &fde, NULL);
	printf("select gave %d\n", rv);
	
	if (FD_ISSET(s, &fdr))
		printf("Data to read\n");
	if (FD_ISSET(s, &fdw))
		printf("OK to write\n");
	if (FD_ISSET(s, &fde))
		printf("Exception!\n");
		
	closesocket(s);
	
	printf("Test complete.\n");

	return (0);
}

