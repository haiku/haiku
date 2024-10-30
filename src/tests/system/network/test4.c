#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>


int main(int argc, char **argv)
{
	int sock = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
	if (sock < 0) {
		printf("Failed! Socket could not be created.\n");
		return -1;
	}
	int flags = fcntl(sock, F_GETFD);
	int ret = 0;
	if ((flags & FD_CLOEXEC) == 0) {
		printf("Failed! Descriptor flag not found.\n");
		ret = -1;
	}
	close(sock);

	printf("Test complete.\n");

	return ret;
}

