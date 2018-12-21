#include <stdio.h>
#include <unistd.h>
#include <kernel/OS.h>

#include "../driver/net_stack_driver.h"
#include "ufunc.h"

int main(int argc, char **argv)
{
	int fd;
	int rv;
	
	test_banner("Stopping Server");
			
	fd = open("/dev/" NET_STACK_DRIVER_PATH, O_RDWR);
	if (fd < 0)
		err(fd, "can't open the stack driver");

	rv = ioctl(fd, NET_STACK_STOP, NULL, 0);
	printf("ioctl to stop stack gave %d\n", rv);
	
	rv = close(fd);
	printf("close gave %d\n", rv);
	
	printf("Operation complete.\n");

	return (0);
}

