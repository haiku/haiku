#include <stdio.h>
#include <stdlib.h>
#include <kernel/OS.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "sys/select.h"

#include "ufunc.h"

#define SOCKETS 15

int main(int argc, char **argv)
{
	int * s;
	int rv;
	struct fd_set fdr, fdw, fde;
	struct timeval tv;
	int32 rtc;
	int i, max = 0;
	int nsock = SOCKETS;

	test_banner("Big Select Test");
	
	if (argc >= 2)
		nsock = atoi(argv[1]);
	
	if (nsock < 1 || nsock > 50) {
		printf("Unrealistic value of sockets given. Must be between 1 and 50\n");
		err(-1, "Parameters");
	}

	s = (int *) malloc(nsock * sizeof(int));
	if (!s)
		err(-1, "Not enought memory for sockets array");

	printf("\nTest will be run with %d sockets\n\n", nsock);
		
	for (i=0;i<nsock;i++) {
		s[i] = socket(AF_INET, SOCK_DGRAM, 0);
		if (s[i] < 0) {
			free(s);
			err(errno, "Socket creation failed");
		}
	}
	
	FD_ZERO(&fdr);
	for (i=0;i<nsock;i++) {
		if (s[i] > max)
			max = s[i];
		FD_SET(s[i], &fdr);
	}
	FD_ZERO(&fdw);
//	FD_SET(s, &fdw);
	FD_ZERO(&fde);
//	FD_SET(s, &fde);
	
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	
	printf("Trying with timeval (5 secs)...\n");
	rtc = real_time_clock();
	rv = select(max + 1, &fdr, NULL, &fde, &tv);
	rtc = real_time_clock() - rtc;
	printf("select gave %d in %ld seconds\n", rv, rtc);
	if (rv < 0)
		printf("errno = %d [%s]\n", errno, strerror(errno));

	printf("resetting select fd's\n");
	FD_ZERO(&fdr);
	for (i=0;i<nsock;i++)
		FD_SET(s[i], &fdr);
	FD_ZERO(&fdw);
	for (i=0;i<nsock;i++)
		FD_SET(s[i], &fdw);
	FD_ZERO(&fde);
	for (i=0;i<nsock;i++)
		FD_SET(s[i], &fde);
	
	printf("Trying without timeval (= NULL)\n");
	rv = select(max +1, &fdr, &fdw, &fde, NULL);
	printf("select gave %d\n", rv);
	
	if (rv > 0) {
		if (FD_ISSET(s[0], &fdr))
			printf("Data to read\n");
		if (FD_ISSET(s[0], &fdw))
			printf("OK to write\n");
		if (FD_ISSET(s[0], &fde))
			printf("Exception!\n");
	} else if (rv < 0)
		printf("error was %d [%s]\n", errno, strerror(errno));

	for (i=0;i<nsock;i++)
		closesocket(s[i]);
	
	free(s);
	
	printf("Test complete.\n");

	return (0);
}

