/* Utility functions for the test apps */

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

void err(int error, char *msg)
{
	printf("Error: %s\n", msg);
	printf("Code: %d\n", error);
	printf("Desc: %s\n", strerror(error));
	exit(-1);
}

void test_banner(char *msg)
{
	int sl = strlen(msg);
	char *buf = (char*)malloc(sl + 5);

	memset(buf, '=', sl + 4);
	printf("%s\n", buf);
	buf[1] = buf[sl + 2] = ' ';
	memcpy(&buf[2], msg, sl);
	printf("%s\n", buf);
	memset(buf, '=', sl + 4);
	printf("%s\n", buf);
}
