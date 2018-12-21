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
	int s, f;
	int rv;
	struct fd_set fdr, fdw, fde;
	struct timeval tv;
	int32 rtc;
	char path[PATH_MAX];
		
	test_banner("Select Test #2");
			
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		err(s, "Socket creation failed");

	getcwd(path, PATH_MAX);
	sprintf(path, "%s/select_test2.c", path);
	f = open(path, O_RDWR);

	if (f > 0 && s > 0)	{
		printf("\nsocket and fd created.\n");
	} else {
		err(-1, "Failed to create socket or fd\n");
	}
	
	FD_ZERO(&fdr);
	FD_SET(s, &fdr);
	FD_ZERO(&fdw);
	FD_SET(s, &fdw);
	FD_ZERO(&fde);
	FD_SET(s, &fde);
	
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	printf("\nTest1\n=====\n\n");	
	printf("Trying with timeval (5 secs)...\n");
	rtc = real_time_clock();
	rv = select(s + 1, &fdr, NULL, &fde, &tv);
	rtc = real_time_clock() - rtc;
	printf("select gave %d (expecting 0) in %ld seconds\n", rv, rtc);

	FD_ZERO(&fdr);
	FD_SET(s, &fdr);
	FD_SET(f, &fdr);
	FD_ZERO(&fdw);
	FD_SET(s, &fdw);
	FD_ZERO(&fde);
	FD_SET(s, &fde);
	
	printf("\nTest2\n=====\n\n");
	printf("Trying without timeval and both sockets and files...\n");
	rv = select(f +1, &fdr, NULL, NULL, NULL);
	printf("select gave %d (expecting 2)\n", rv);
	
	if (rv > 0) {
		if (FD_ISSET(s, &fdr))
			printf("Data to read\n");
		if (FD_ISSET(s, &fdw))
			printf("OK to write\n");
		if (FD_ISSET(s, &fde))
			printf("Exception!\n");
		if (FD_ISSET(f, &fdr))
			printf("File is readable!\n");
	} else if (rv == 0) {
		printf("Timed out??? huh?!\n");
	} else {
		printf("errno = %d [%s]\n", errno, strerror(errno));
	}
	
	closesocket(s);
	close(f);
	
	printf("Test complete.\n");

	return (0);
}

